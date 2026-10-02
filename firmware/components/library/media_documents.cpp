#include "nightwave/media_documents.h"
#include "nightwave/mp3_seek_index.h"
#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <strings.h>
namespace nightwave {
namespace {
constexpr std::size_t kDocumentLimit = 65536;
bool media(const char* p) {
    const auto* dot = std::strrchr(p, '.');
    return dot && (!strcasecmp(dot, ".mp3") || !strcasecmp(dot, ".wav"));
}
std::uint32_t be32(const unsigned char* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3];
}
std::uint32_t le32(const unsigned char* p) {
    return p[0] | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
bool synchsafe(const unsigned char* p, std::uint32_t& n) {
    if ((p[0] | p[1] | p[2] | p[3]) & 128) return false;
    n = (std::uint32_t(p[0]) << 21) | (std::uint32_t(p[1]) << 14) | (std::uint32_t(p[2]) << 7) | p[3]; return true;
}
enum class LineResult { kLine, kEnd, kBad, kLimit, kIo };
LineResult line(std::FILE* file, char* dst, std::size_t cap, std::size_t& total) {
    std::size_t used = 0; bool bad = false, overlong = false; int c;
    while ((c = std::fgetc(file)) != EOF) {
        if (++total > kDocumentLimit) return LineResult::kLimit;
        if (c == '\n') break;
        if (c == 0 || (c < 32 && c != '\r' && c != '\t')) bad = true;
        if (used + 1 < cap) dst[used++] = static_cast<char>(c); else overlong = true;
    }
    dst[used] = 0;
    if (std::ferror(file)) return LineResult::kIo;
    if (bad) return LineResult::kBad;
    if (overlong) return LineResult::kLimit;
    while (used && (dst[used - 1] == '\r' || dst[used - 1] == ' ' || dst[used - 1] == '\t')) dst[--used] = 0;
    return c == EOF && !used ? LineResult::kEnd : LineResult::kLine;
}
DocumentStatus status(LineResult r) {
    return r == LineResult::kIo ? DocumentStatus::kIoError :
        (r == LineResult::kLimit ? DocumentStatus::kLimit : DocumentStatus::kMalformed);
}
bool number(const char*& p, unsigned& n, unsigned max_digits) {
    n = 0; unsigned digits = 0;
    while (*p >= '0' && *p <= '9') {
        if (++digits > max_digits) return false;
        n = n * 10 + unsigned(*p++ - '0');
    }
    return digits != 0;
}
bool timestamp(const char*& p, std::uint32_t& ms) {
    const auto* start = p; unsigned minute, second, fraction = 0, digits = 0;
    if (*p++ != '[' || !number(p, minute, 3) || *p++ != ':' || !number(p, second, 2) || second > 59) { p = start; return false; }
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') { if (++digits > 3) { p = start; return false; } fraction = fraction * 10 + unsigned(*p++ - '0'); }
        if (!digits) { p = start; return false; }
        if (digits == 1) fraction *= 100; else if (digits == 2) fraction *= 10;
    }
    if (*p != ']') { p = start; return false; }
    ++p; ms = (minute * 60 + second) * 1000 + fraction; return true;
}
// Decode bounded Latin-1, UTF-8 or UTF-16 without splitting UTF-8 characters.
// Invalid encodings truncate at the invalid character, never read beyond size.
void copy_text(char* dst, std::size_t cap, const unsigned char* src, std::size_t size, unsigned encoding) {
    std::size_t written = 0, i = 0; bool little = encoding == 1;
    if (encoding == 1) {
        if (size < 2) { dst[0] = 0; return; }
        if (src[0] == 0xfe && src[1] == 0xff) little = false;
        else if (!(src[0] == 0xff && src[1] == 0xfe)) { dst[0] = 0; return; }
        i = 2;
    }
    auto emit = [&](std::uint32_t cp) {
        unsigned char bytes[4]; std::size_t n;
        if (cp < 128) { bytes[0] = static_cast<unsigned char>(cp); n = 1; }
        else if (cp < 2048) { bytes[0] = 0xc0 | (cp >> 6); bytes[1] = 0x80 | (cp & 63); n = 2; }
        else if (cp < 65536) { bytes[0] = 0xe0 | (cp >> 12); bytes[1] = 0x80 | ((cp >> 6) & 63); bytes[2] = 0x80 | (cp & 63); n = 3; }
        else { bytes[0] = 0xf0 | (cp >> 18); bytes[1] = 0x80 | ((cp >> 12) & 63); bytes[2] = 0x80 | ((cp >> 6) & 63); bytes[3] = 0x80 | (cp & 63); n = 4; }
        if (written + n >= cap) return false;
        for (std::size_t j = 0; j < n; ++j) dst[written++] = static_cast<char>(bytes[j]);
        return true;
    };
    while (i < size && written + 1 < cap) {
        std::uint32_t cp = src[i++];
        if (encoding == 1 || encoding == 2) {
            if (i >= size) break;
            cp = little ? cp | (std::uint32_t(src[i++]) << 8) : (cp << 8) | src[i++];
            if (cp >= 0xd800 && cp <= 0xdbff) {
                if (i + 1 >= size) break;
                const auto low = little ? src[i] | (std::uint32_t(src[i + 1]) << 8) : (std::uint32_t(src[i]) << 8) | src[i + 1];
                i += 2; if (low < 0xdc00 || low > 0xdfff) break;
                cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
            } else if (cp >= 0xdc00 && cp <= 0xdfff) break;
        } else if (encoding == 3 && cp >= 128) {
            unsigned continuation = cp >= 0xf0 && cp <= 0xf4 ? 3 : cp >= 0xe0 && cp <= 0xef ? 2 : cp >= 0xc2 && cp <= 0xdf ? 1 : 0;
            if (!continuation || i + continuation > size) break;
            const auto minimum = continuation == 1 ? 128U : continuation == 2 ? 2048U : 65536U;
            cp &= (1U << (6 - continuation)) - 1; bool valid = true;
            while (continuation--) { const auto b = src[i++]; if ((b & 0xc0) != 0x80) valid = false; cp = (cp << 6) | (b & 63); }
            if (!valid || cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) break;
        }
        if (!cp) break;
        if (cp < 32) cp = ' ';
        if (!emit(cp)) break;
    }
    dst[written] = 0;
}
}
bool local_media_path(const char* base, const char* relative, const char* root, char* dst, std::size_t cap) {
    if (!base || !relative || !root || !dst || !cap || !*relative || *relative == '/' || *relative == '\\' || std::strchr(relative, ':')) return false;
    const auto root_size = std::strlen(root), base_size = std::strlen(base);
    if (!root_size || root_size > base_size || std::strncmp(base, root, root_size) || (base_size > root_size && base[root_size] != '/')) return false;
    if (base_size >= cap) return false;
    std::memcpy(dst, base, base_size + 1); std::size_t used = base_size; const char* p = relative;
    while (*p) {
        const auto* begin = p; while (*p && *p != '/' && *p != '\\') ++p;
        const auto n = static_cast<std::size_t>(p - begin);
        if (n == 2 && begin[0] == '.' && begin[1] == '.') {
            if (used <= root_size) return false;
            while (used > root_size && dst[used - 1] != '/') --used;
            if (used > root_size) --used;
            dst[used] = 0;
        } else if (n && !(n == 1 && begin[0] == '.')) {
            if (used + n + 2 > cap) return false;
            dst[used++] = '/'; std::memcpy(dst + used, begin, n); used += n; dst[used] = 0;
        }
        if (*p) ++p;
    }
    return media(dst);
}
DocumentStatus Lyrics::load(const char* media_path) {
    clear(); if (!media_path) return DocumentStatus::kMalformed;
    std::array<char, 256> path{}; const auto size = std::strlen(media_path);
    if (size >= path.size()) return DocumentStatus::kLimit;
    std::memcpy(path.data(), media_path, size + 1); auto* dot = std::strrchr(path.data(), '.');
    if (!dot || static_cast<std::size_t>(dot - path.data()) + 5 > path.size()) return DocumentStatus::kMalformed;
    std::strcpy(dot, ".lrc"); auto* file = std::fopen(path.data(), "rb");
    if (!file) return DocumentStatus::kMissing;
    const auto result = parse(file); std::fclose(file); return result;
}
DocumentStatus Lyrics::parse(std::FILE* file) {
    clear(); if (!file) return DocumentStatus::kMalformed;
    std::array<char, 512> buffer{}; std::size_t total = 0; std::int32_t offset = 0; bool first = true;
    auto reject = [this](DocumentStatus s) { clear(); return s; };
    while (true) {
        const auto r = line(file, buffer.data(), buffer.size(), total);
        if (r == LineResult::kEnd) break;
        if (r != LineResult::kLine) return reject(status(r));
        const char* p = buffer.data();
        if (first && std::strlen(p) >= 3 && !std::memcmp(p, "\xef\xbb\xbf", 3)) p += 3;
        first = false;
        if (!std::strncmp(p, "[offset:", 8)) {
            p += 8; const bool negative = *p == '-'; if (*p == '-' || *p == '+') ++p;
            unsigned n; if (!number(p, n, 6) || *p++ != ']' || *p || n > 600000) return reject(DocumentStatus::kMalformed);
            offset = negative ? -static_cast<std::int32_t>(n) : static_cast<std::int32_t>(n); continue;
        }
        if (*p != '[' || p[1] < '0' || p[1] > '9') continue;
        std::array<std::uint32_t, 16> times{}; std::size_t tags = 0; std::uint32_t ms;
        while (*p == '[' && p[1] >= '0' && p[1] <= '9') {
            if (tags == times.size() || !timestamp(p, ms)) return reject(DocumentStatus::kMalformed);
            times[tags++] = ms;
        }
        if (count_ + tags > lines_.size()) return reject(DocumentStatus::kLimit);
        for (std::size_t i = 0; i < tags; ++i) {
            auto& target = lines_[count_++]; target.time_ms = times[i];
            copy_text(target.text.data(), target.text.size(), reinterpret_cast<const unsigned char*>(p), std::strlen(p), 3);
        }
    }
    for (std::size_t i = 0; i < count_; ++i) lines_[i].time_ms = static_cast<std::uint32_t>(std::max<std::int64_t>(0, std::int64_t(lines_[i].time_ms) + offset));
    std::stable_sort(lines_.begin(), lines_.begin() + count_, [](const LyricLine& a, const LyricLine& b) { return a.time_ms < b.time_ms; });
    return count_ ? DocumentStatus::kOk : DocumentStatus::kMalformed;
}
const LyricLine* Lyrics::current(std::uint32_t position) const {
    std::size_t lo = 0, hi = count_;
    while (lo < hi) { const auto mid = lo + (hi - lo) / 2; if (lines_[mid].time_ms <= position) lo = mid + 1; else hi = mid; }
    return lo ? &lines_[lo - 1] : nullptr;
}
const LyricLine* Lyrics::next(std::uint32_t position) const {
    const auto* now = current(position); const auto index = now ? static_cast<std::size_t>(now - lines_.data()) + 1 : 0;
    return index < count_ ? &lines_[index] : nullptr;
}
DocumentStatus Playlist::load(const char* path, const char* root) {
    count_ = 0; if (!path || !root) return DocumentStatus::kMalformed;
    std::array<char, 256> base{}; if (std::strlen(path) >= base.size()) return DocumentStatus::kLimit;
    std::strcpy(base.data(), path); auto* slash = std::strrchr(base.data(), '/');
    if (!slash) return DocumentStatus::kMalformed;
    *slash = 0; auto* file = std::fopen(path, "rb"); if (!file) return DocumentStatus::kMissing;
    std::size_t total = 0; std::array<char, 512> buffer{}; DocumentStatus result = DocumentStatus::kOk; bool first = true;
    while (true) {
        const auto r = line(file, buffer.data(), buffer.size(), total);
        if (r == LineResult::kEnd) break;
        if (r != LineResult::kLine) { result = status(r); break; }
        const char* p = buffer.data();
        if (first && std::strlen(p) >= 3 && !std::memcmp(p, "\xef\xbb\xbf", 3)) p += 3;
        first = false;
        if (!*p || *p == '#') continue;
        if (count_ == paths_.size()) { result = DocumentStatus::kLimit; break; }
        if (!local_media_path(base.data(), p, root, paths_[count_].data(), paths_[count_].size())) { result = DocumentStatus::kMalformed; break; }
        ++count_;
    }
    std::fclose(file); if (result != DocumentStatus::kOk) count_ = 0; return result;
}
DocumentStatus read_metadata(const char* path, TrackMetadata& metadata) {
    metadata = {}; if (!path) return DocumentStatus::kMalformed;
    const auto* slash = std::strrchr(path, '/'); const auto* fallback = slash ? slash + 1 : path;
    copy_text(metadata.title.data(), metadata.title.size(), reinterpret_cast<const unsigned char*>(fallback), std::strlen(fallback), 3);
    auto* file = std::fopen(path, "rb"); if (!file) return DocumentStatus::kMissing;
    auto finish = [file](DocumentStatus result) { std::fclose(file); return result; };
    if (std::fseek(file, 0, SEEK_END)) return finish(DocumentStatus::kIoError);
    const auto end = std::ftell(file); if (end < 10) return finish(DocumentStatus::kMalformed);
    const auto* extension = std::strrchr(path, '.');
    if (extension && !strcasecmp(extension, ".mp3") && std::uint64_t(end) <= UINT32_MAX)
        metadata.duration_ms = Mp3SeekIndex::probe_duration(file, static_cast<std::uint32_t>(end));
    std::rewind(file); std::array<unsigned char, 12> header{};
    if (std::fread(header.data(), 1, 10, file) != 10) return finish(DocumentStatus::kIoError);
    if (!std::memcmp(header.data(), "ID3", 3)) {
        std::uint32_t size; const auto version = header[3];
        if (version != 3 && version != 4) return finish(DocumentStatus::kMalformed);
        if (!synchsafe(header.data() + 6, size) || size > kDocumentLimit || std::uint64_t(size) + 10 > std::uint64_t(end)) return finish(DocumentStatus::kLimit);
        if (header[5] & 0xe0) return finish(DocumentStatus::kMalformed);
        std::uint32_t left = size; unsigned frames = 0; std::array<unsigned char, 512> text{};
        while (left >= 10 && frames++ < 256) {
            if (std::fread(header.data(), 1, 10, file) != 10) return finish(DocumentStatus::kIoError);
            if (!header[0]) break;
            std::uint32_t n = be32(header.data() + 4);
            if (version == 4 && !synchsafe(header.data() + 4, n)) return finish(DocumentStatus::kMalformed);
            left -= 10; if (!n || n > left) return finish(DocumentStatus::kMalformed);
            char* target = nullptr; std::size_t capacity = 0;
            if (!std::memcmp(header.data(), "TIT2", 4)) { target = metadata.title.data(); capacity = metadata.title.size(); }
            else if (!std::memcmp(header.data(), "TPE1", 4)) { target = metadata.artist.data(); capacity = metadata.artist.size(); }
            else if (!std::memcmp(header.data(), "TALB", 4)) { target = metadata.album.data(); capacity = metadata.album.size(); }
            if (target && !header[8] && !header[9]) {
                const auto take = std::min<std::size_t>(n, text.size());
                if (std::fread(text.data(), 1, take, file) != take) return finish(DocumentStatus::kIoError);
                if (text[0] <= 3) copy_text(target, capacity, text.data() + 1, take - 1, text[0]);
                if (std::fseek(file, static_cast<long>(n - take), SEEK_CUR)) return finish(DocumentStatus::kIoError);
            } else if (std::fseek(file, static_cast<long>(n), SEEK_CUR)) return finish(DocumentStatus::kIoError);
            left -= n;
        }
        return finish(frames >= 256 && left ? DocumentStatus::kLimit : DocumentStatus::kOk);
    }
    if (!std::memcmp(header.data(), "RIFF", 4)) {
        if (std::fread(header.data() + 10, 1, 2, file) != 2 || std::memcmp(header.data() + 8, "WAVE", 4)) return finish(DocumentStatus::kMalformed);
        std::uint32_t byte_rate = 0, data_size = 0; unsigned chunks = 0;
        while (std::ftell(file) + 8 <= end && chunks++ < 256) {
            if (std::fread(header.data(), 1, 8, file) != 8) return finish(DocumentStatus::kIoError);
            const auto n = le32(header.data() + 4); const auto payload = std::ftell(file);
            if (payload < 0 || std::uint64_t(payload) + n + (n & 1) > std::uint64_t(end)) return finish(DocumentStatus::kMalformed);
            if (!std::memcmp(header.data(), "fmt ", 4) && n >= 16) {
                std::array<unsigned char, 16> fmt{}; if (std::fread(fmt.data(), 1, 16, file) != 16) return finish(DocumentStatus::kIoError);
                byte_rate = le32(fmt.data() + 8);
            } else if (!std::memcmp(header.data(), "data", 4)) data_size = n;
            else if (!std::memcmp(header.data(), "LIST", 4) && n >= 4) {
                if (n > kDocumentLimit) return finish(DocumentStatus::kLimit);
                if (std::fread(header.data() + 8, 1, 4, file) != 4) return finish(DocumentStatus::kIoError);
                if (!std::memcmp(header.data() + 8, "INFO", 4)) {
                    std::uint32_t left = n - 4;
                    while (left >= 8) {
                        if (std::fread(header.data(), 1, 8, file) != 8) return finish(DocumentStatus::kIoError);
                        const auto bytes = le32(header.data() + 4); const auto padded = std::uint64_t(bytes) + (bytes & 1);
                        if (padded > left - 8) return finish(DocumentStatus::kMalformed);
                        std::array<unsigned char, 256> text{}; const auto take = std::min<std::size_t>(bytes, text.size());
                        if (std::fread(text.data(), 1, take, file) != take) return finish(DocumentStatus::kIoError);
                        if (!std::memcmp(header.data(), "INAM", 4)) copy_text(metadata.title.data(), metadata.title.size(), text.data(), take, 0);
                        else if (!std::memcmp(header.data(), "IART", 4)) copy_text(metadata.artist.data(), metadata.artist.size(), text.data(), take, 0);
                        else if (!std::memcmp(header.data(), "IPRD", 4)) copy_text(metadata.album.data(), metadata.album.size(), text.data(), take, 0);
                        if (std::fseek(file, static_cast<long>(padded - take), SEEK_CUR)) return finish(DocumentStatus::kIoError);
                        left -= static_cast<std::uint32_t>(8 + padded);
                    }
                }
            }
            const auto next = std::uint64_t(payload) + n + (n & 1);
            if (next > LONG_MAX || std::fseek(file, static_cast<long>(next), SEEK_SET)) return finish(DocumentStatus::kIoError);
        }
        if (byte_rate && data_size) metadata.duration_ms = static_cast<std::uint32_t>(std::min<std::uint64_t>(UINT32_MAX, data_size * 1000ULL / byte_rate));
        return finish(chunks >= 256 ? DocumentStatus::kLimit : DocumentStatus::kOk);
    }
    if (end >= 128 && !std::fseek(file, end - 128, SEEK_SET)) {
        std::array<unsigned char, 128> tail{};
        if (std::fread(tail.data(), 1, tail.size(), file) != tail.size()) return finish(DocumentStatus::kIoError);
        if (!std::memcmp(tail.data(), "TAG", 3)) {
            copy_text(metadata.title.data(), metadata.title.size(), tail.data() + 3, 30, 0);
            copy_text(metadata.artist.data(), metadata.artist.size(), tail.data() + 33, 30, 0);
            copy_text(metadata.album.data(), metadata.album.size(), tail.data() + 63, 30, 0);
        }
    }
    return finish(DocumentStatus::kOk);
}
}
