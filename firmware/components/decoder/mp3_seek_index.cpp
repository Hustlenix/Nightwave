#include "nightwave/mp3_seek_index.h"
#include <algorithm>
#include <climits>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
namespace nightwave {
namespace {
std::uint32_t hash(std::uint32_t h, const void* data, std::size_t n) {
    const auto* p = static_cast<const unsigned char*>(data);
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}
void put(unsigned char* p, std::uint32_t n) { for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<unsigned char>(n >> (8 * i)); }
std::uint32_t get(const unsigned char* p) { return p[0] | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24); }
std::uint32_t be(const unsigned char* p) { return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3]; }
bool identity(std::FILE* f, std::uint32_t& length, std::uint64_t& modified, std::uint32_t& fingerprint) {
    struct stat s{};
    if (fstat(fileno(f), &s) || !S_ISREG(s.st_mode) || s.st_size <= 0 || std::uint64_t(s.st_size) > LONG_MAX || std::uint64_t(s.st_size) > UINT32_MAX) return false;
    length = static_cast<std::uint32_t>(s.st_size); modified = static_cast<std::uint64_t>(s.st_mtime);
    std::array<unsigned char, 512> b{}; fingerprint = 2166136261u;
    // Size/mtime plus first/last-window freshness, not hostile-edit authentication.
    const std::array<std::uint32_t, 2> windows{0, length > b.size() ? length - static_cast<std::uint32_t>(b.size()) : 0};
    for (const auto at : windows) {
        const auto n = std::min<std::size_t>(b.size(), length - at);
        if (std::fseek(f, static_cast<long>(at), SEEK_SET) || std::fread(b.data(), 1, n, f) != n) return false;
        fingerprint = hash(fingerprint, b.data(), n);
    }
    return true;
}
bool cache_path(const char* root, const char* media, char* out, std::size_t cap) {
    if (!root || !media || !*root || std::strlen(media) >= 256 || std::strlen(root) > 218) return false;
    const auto n = std::strlen(root);
    if (root[0] != '/' || root[n - 1] == '/' || std::strstr(root, "/../") || std::strstr(root, "/./") || std::strchr(root, '\\') ||
        std::strncmp(root, media, n) || media[n] != '/' || std::strstr(media, "/../") || std::strstr(media, "/./") || std::strchr(media, '\\')) return false;
    std::array<char, 256> directory{};
    std::snprintf(directory.data(), directory.size(), "%s/.nightwave", root);
    struct stat s{};
#ifdef ESP_PLATFORM
    if (stat(directory.data(), &s) || !S_ISDIR(s.st_mode)) return false;
#else
    if (lstat(directory.data(), &s) || !S_ISDIR(s.st_mode)) return false;
#endif
    const auto id = hash(2166136261u, media, std::strlen(media));
    const int written = std::snprintf(out, cap, "%s/.nightwave/seek-%08lx.bin", root, static_cast<unsigned long>(id));
    return written > 0 && static_cast<std::size_t>(written) < cap;
}
bool regular(const char* p, bool missing = false) {
    struct stat s{};
#ifdef ESP_PLATFORM
    const auto ok = stat(p, &s) == 0;
#else
    const auto ok = lstat(p, &s) == 0;
#endif
    return ok ? S_ISREG(s.st_mode) : missing && errno == ENOENT;
}
bool audio_start(std::FILE* f, std::uint32_t length, std::uint32_t& at) {
    std::array<unsigned char, 10> h{}; at = 0;
    if (length < 10 || std::fseek(f, 0, SEEK_SET) || std::fread(h.data(), 1, h.size(), f) != h.size()) return false;
    if (!std::memcmp(h.data(), "ID3", 3)) {
        if (h[3] < 2 || h[3] > 4 || h[4] == 255 || ((h[6] | h[7] | h[8] | h[9]) & 128)) return false;
        const auto reserved = h[3] == 2 ? 0x3fu : h[3] == 3 ? 0x1fu : 0x0fu;
        if (h[5] & reserved) return false;
        const auto bytes = (std::uint32_t(h[6]) << 21) | (std::uint32_t(h[7]) << 14) | (std::uint32_t(h[8]) << 7) | h[9];
        if (bytes > Mp3Framer::kMaxTagBytes) return false;
        at = bytes + 10 + (h[3] == 4 && (h[5] & 16) ? 10 : 0);
    }
    return at < length;
}
}
void Mp3SeekIndex::cancel() { if (file_) std::fclose(file_); file_ = nullptr; if (status_ == SeekIndexStatus::kBuilding) status_ = SeekIndexStatus::kIdle; }
bool Mp3SeekIndex::begin(const char* path) {
    cancel(); status_ = SeekIndexStatus::kInvalid; frames_ = samples_ = rate_ = count_ = cursor_ = 0; buffered_ = 0; framer_.reset();
    if (!path || std::strlen(path) >= path_.size()) return false;
    file_ = std::fopen(path, "rb");
    if (!file_ || !identity(file_, length_, modified_, fingerprint_)) { cancel(); return false; }
    if (!audio_start(file_, length_, cursor_)) { cancel(); return false; }
    const auto n = std::min<std::size_t>(buffer_.size(), length_ - cursor_);
    if (std::fseek(file_, static_cast<long>(cursor_), SEEK_SET) || std::fread(buffer_.data(), 1, n, file_) != n) { cancel(); return false; }
    Mp3Framer check;
    if (check.scan(buffer_.data(), n, cursor_ + n == length_).status != Mp3ScanStatus::kFrame) { cancel(); return false; }
    buffered_ = n;
    std::strcpy(path_.data(), path); status_ = SeekIndexStatus::kBuilding; return true;
}
void Mp3SeekIndex::step(unsigned budget) {
    budget = std::min(budget, 8u);
    const auto fail = [this](SeekIndexStatus status) { status_ = status; cancel(); };
    while (file_ && status_ == SeekIndexStatus::kBuilding && budget--) {
        if (cursor_ == length_) { fail(frames_ ? SeekIndexStatus::kReady : SeekIndexStatus::kInvalid); return; }
        auto r = framer_.scan(buffer_.data(), buffered_, cursor_ + buffered_ == length_);
        if (r.status == Mp3ScanStatus::kNeedInput) {
            const auto n = std::min<std::size_t>(buffer_.size() - buffered_, length_ - cursor_ - buffered_);
            if (!n || std::fseek(file_, static_cast<long>(cursor_ + buffered_), SEEK_SET) || std::fread(buffer_.data() + buffered_, 1, n, file_) != n) { fail(SeekIndexStatus::kIo); return; }
            buffered_ += n;
            r = framer_.scan(buffer_.data(), buffered_, cursor_ + buffered_ == length_);
        }
        if (!r.bytes || r.bytes > buffered_ || (r.status != Mp3ScanStatus::kSkip && r.status != Mp3ScanStatus::kFrame)) { fail(SeekIndexStatus::kInvalid); return; }
        if (r.status == Mp3ScanStatus::kFrame) {
            if (rate_ && r.rate != rate_) { fail(SeekIndexStatus::kInvalid); return; }
            rate_ = r.rate;
            if (frames_ == kPoints * kStride) { fail(SeekIndexStatus::kLimit); return; }
            if (frames_ % kStride == 0) points_[count_++] = {cursor_, samples_};
            samples_ += ((buffer_[1] >> 3) & 3) == 3 ? 1152u : 576u; ++frames_;
        } else if (frames_ && framer_.recovery_bytes()) { fail(SeekIndexStatus::kInvalid); return; }
        cursor_ += static_cast<std::uint32_t>(r.bytes);
        buffered_ -= r.bytes;
        std::memmove(buffer_.data(), buffer_.data() + r.bytes, buffered_);
    }
}
std::uint32_t Mp3SeekIndex::duration_ms() const { return ready() && rate_ ? static_cast<std::uint32_t>(std::uint64_t(samples_) * 1000 / rate_) : 0; }
bool Mp3SeekIndex::save(const char* root) {
    std::array<char, 256> path{}, temporary{};
    if (!ready() || !cache_path(root, path_.data(), path.data(), path.size()) || !regular(path.data(), true)) return false;
    auto* source = std::fopen(path_.data(), "rb");
    if (!source) return false;
    std::uint32_t length = 0, fingerprint = 0; std::uint64_t modified = 0;
    const bool fresh = identity(source, length, modified, fingerprint) && length == length_ && modified == modified_ && fingerprint == fingerprint_;
    std::fclose(source);
    if (!fresh) return false;
    std::strcpy(temporary.data(), path.data()); auto* dot = std::strrchr(temporary.data(), '.'); std::strcpy(dot, ".tmp");
    if (!regular(temporary.data(), true)) return false;
    auto* f = std::fopen(temporary.data(), "wb"); if (!f) return false;
    std::array<unsigned char, 304> h{}; std::memcpy(h.data(), "NWSEEK01", 8);
    put(h.data() + 8, length_); put(h.data() + 12, static_cast<std::uint32_t>(modified_)); put(h.data() + 16, static_cast<std::uint32_t>(modified_ >> 32));
    put(h.data() + 20, fingerprint_); put(h.data() + 24, rate_); put(h.data() + 28, frames_); put(h.data() + 32, samples_); put(h.data() + 36, count_);
    std::memcpy(h.data() + 40, path_.data(), 256); put(h.data() + 300, hash(2166136261u, h.data(), 300));
    bool ok = std::fwrite(h.data(), 1, h.size(), f) == h.size(); std::uint32_t crc = 2166136261u;
    for (std::uint32_t i = 0; ok && i < count_; ++i) {
        std::array<unsigned char, 8> b{}; put(b.data(), points_[i].offset); put(b.data() + 4, points_[i].samples);
        crc = hash(crc, b.data(), b.size()); ok = std::fwrite(b.data(), 1, b.size(), f) == b.size();
    }
    std::array<unsigned char, 4> tail{}; put(tail.data(), crc);
    ok = ok && std::fwrite(tail.data(), 1, tail.size(), f) == tail.size() && !std::fflush(f) && !fsync(fileno(f));
    if (std::fclose(f)) ok = false;
    if (ok && rename(temporary.data(), path.data())) ok = false;
    return ok; // Old cache survives write failure; FAT rename durability unproven.
}
bool Mp3SeekIndex::cached_seek(const char* media, const char* root, std::uint32_t ms, Mp3SeekPoint& point) {
    point = {}; std::array<char, 256> path{};
    if (!cache_path(root, media, path.data(), path.size()) || !regular(path.data())) return false;
    auto* source = std::fopen(media, "rb"); if (!source) return false;
    std::uint32_t length = 0, fingerprint = 0; std::uint64_t modified = 0;
    const bool matches = identity(source, length, modified, fingerprint); std::fclose(source); if (!matches) return false;
    auto* f = std::fopen(path.data(), "rb"); if (!f) return false;
    const auto finish = [f](bool ok) { std::fclose(f); return ok; };
    std::array<unsigned char, 304> h{}; struct stat s{};
    if (fstat(fileno(f), &s) || std::fread(h.data(), 1, h.size(), f) != h.size() || std::memcmp(h.data(), "NWSEEK01", 8) ||
        get(h.data() + 300) != hash(2166136261u, h.data(), 300) || !std::memchr(h.data() + 40, 0, 256) || std::strcmp(media, reinterpret_cast<const char*>(h.data() + 40)) ||
        get(h.data() + 8) != length || (get(h.data() + 12) | (std::uint64_t(get(h.data() + 16)) << 32)) != modified || get(h.data() + 20) != fingerprint) return finish(false);
    const auto rate = get(h.data() + 24), frames = get(h.data() + 28), samples = get(h.data() + 32), count = get(h.data() + 36);
    if (!count || count > kPoints || !frames || frames > kPoints * kStride || count != (frames + kStride - 1) / kStride ||
        (rate != 22050 && rate != 32000 && rate != 44100 && rate != 48000) || samples != frames * (rate == 22050 ? 576u : 1152u) ||
        s.st_size != static_cast<off_t>(308 + count * 8) || std::uint64_t(ms) * rate / 1000 >= samples) return finish(false);
    const auto wanted = std::uint64_t(ms) * rate / 1000;
    const auto frame_samples = rate == 22050 ? 576u : 1152u;
    const auto group = static_cast<std::uint32_t>(wanted / frame_samples / kStride);
    const auto anchor = group ? group - 1 : 0; // >=128-frame reservoir/synthesis preroll for late seeks.
    std::uint32_t crc = 2166136261u, previous = 0; Mp3SeekPoint candidate{};
    for (std::uint32_t i = 0; i < count; ++i) {
        std::array<unsigned char, 8> b{}; if (std::fread(b.data(), 1, b.size(), f) != b.size()) return finish(false);
        crc = hash(crc, b.data(), b.size()); const auto offset = get(b.data()), at = get(b.data() + 4);
        if (offset >= length || (i && offset <= previous) || at != i * kStride * frame_samples) return finish(false);
        if (i == anchor) candidate = {offset, at, rate, static_cast<std::uint32_t>(std::uint64_t(samples) * 1000 / rate)};
        previous = offset;
    }
    std::array<unsigned char, 4> tail{};
    if (std::fread(tail.data(), 1, tail.size(), f) != tail.size() || get(tail.data()) != crc) return finish(false);
    source = std::fopen(media, "rb");
    if (!source) return finish(false);
    std::array<unsigned char, 1536> frame{};
    const auto n = std::min<std::size_t>(frame.size(), length - candidate.offset);
    const bool read = !std::fseek(source, static_cast<long>(candidate.offset), SEEK_SET) && std::fread(frame.data(), 1, n, source) == n;
    std::fclose(source);
    Mp3Framer framer;
    const auto result = read ? framer.scan(frame.data(), n, candidate.offset + n == length) : Mp3ScanResult{};
    if (result.status != Mp3ScanStatus::kFrame || result.rate != rate) return finish(false);
    point = candidate; return finish(true);
}
std::uint32_t Mp3SeekIndex::probe_duration(std::FILE* f, std::uint32_t length) {
    if (!f || length < 4) return 0;
    std::array<unsigned char, 1536> b{}; std::uint32_t at = 0;
    if (!audio_start(f, length, at)) return 0;
    const auto n = std::min<std::size_t>(b.size(), length - at);
    if (std::fseek(f, static_cast<long>(at), SEEK_SET) || std::fread(b.data(), 1, n, f) != n) return 0;
    Mp3Framer framer; const auto r = framer.scan(b.data(), n, at + n == length);
    if (r.status != Mp3ScanStatus::kFrame) return 0;
    const bool v1 = ((b[1] >> 3) & 3) == 3;
    const auto offset = 4u + (v1 ? (r.channels == 1 ? 17u : 32u) : (r.channels == 1 ? 9u : 17u));
    if (offset + 12 > r.bytes || (std::memcmp(b.data() + offset, "Xing", 4) && std::memcmp(b.data() + offset, "Info", 4))) return 0;
    const auto flags = be(b.data() + offset + 4), frames = be(b.data() + offset + 8);
    if (!(flags & 1) || flags > 15 || !frames || frames > kPoints * kStride) return 0;
    const auto required = offset + 8 + 4 + ((flags & 2) ? 4u : 0u) + ((flags & 4) ? 100u : 0u) + ((flags & 8) ? 4u : 0u);
    if (required > r.bytes || frames > (length - at) / (v1 ? 96u : 24u)) return 0;
    if ((flags & 2) && (be(b.data() + offset + 12) > length - at || be(b.data() + offset + 12) < r.bytes)) return 0;
    return static_cast<std::uint32_t>(std::uint64_t(frames) * (v1 ? 1152 : 576) * 1000 / r.rate);
}
}
