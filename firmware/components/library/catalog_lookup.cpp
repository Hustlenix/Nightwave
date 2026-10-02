#include "nightwave/catalog_lookup.h"
#include <algorithm>
#include <cstring>
namespace nightwave {
namespace {
constexpr std::uint32_t kGroupBytes = 68;
std::uint32_t hash_more(std::uint32_t value, const unsigned char* p, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) value = (value ^ p[i]) * 16777619u;
    return value;
}
}
const char* CatalogLookup::key(const Group& g) { return g.name[0] ? g.name.data() : "(UNKNOWN)"; }
bool CatalogLookup::add(Field& f, const char* text, std::uint16_t track) {
    const auto* normalized = *text ? text : "(UNKNOWN)";
    std::uint32_t group = 0;
    while (group < f.count && std::strcmp(key(f.groups[group]), normalized)) ++group;
    if (group == f.count) {
        if (f.count == kGroups) return false;
        std::strcpy(f.groups[group].name.data(), text); ++f.count;
    }
    ++f.groups[group].count; f.assignments[track] = static_cast<std::uint16_t>(group); return true;
}
bool CatalogLookup::add(const TrackMetadata& m, std::uint16_t track) {
    if (ready_ || failed_ || track != tracks_ || track >= kTracks || !std::memchr(m.artist.data(), 0, m.artist.size()) || !std::memchr(m.album.data(), 0, m.album.size())) return false;
    if (!add(artists_, m.artist.data(), track) || !add(albums_, m.album.data(), track)) return false;
    ++tracks_; return true;
}
void CatalogLookup::prepare(std::uint32_t tracks) {
    if (tracks != tracks_ || tracks > kTracks) { failed_ = true; return; }
    tracks_ = tracks; seal_cursor_ = wire_cursor_ = 0; checksum_ = 2166136261u; ready_ = false;
    for (auto* f : {&artists_, &albums_}) {
        std::uint32_t start = 0;
        for (std::uint32_t i = 0; i < f->count; ++i) {
            f->groups[i].start = static_cast<std::uint16_t>(start); f->cursors[i] = static_cast<std::uint16_t>(start);
            start += f->groups[i].count; f->order[i] = static_cast<std::uint16_t>(i);
        }
        std::sort(f->order.begin(), f->order.begin() + f->count, [f](auto a, auto b) { return std::strcmp(key(f->groups[a]), key(f->groups[b])) < 0; });
    }
}
bool CatalogLookup::seal_step() {
    if (failed_) return false;
    const auto end = std::min(tracks_, seal_cursor_ + 64);
    while (seal_cursor_ < end) {
        for (auto* f : {&artists_, &albums_}) {
            auto& cursor = f->cursors[f->assignments[seal_cursor_]];
            f->ids[cursor++] = static_cast<std::uint16_t>(seal_cursor_);
        }
        ++seal_cursor_;
    }
    ready_ = seal_cursor_ == tracks_; return ready_;
}
std::uint32_t CatalogLookup::bytes() const { return (artists_.count + albums_.count) * kGroupBytes + tracks_ * 4; }
// Explicit little-endian wire format, never fwrite a compiler-dependent struct.
void CatalogLookup::encode(std::uint32_t offset, unsigned char* data, std::size_t size) const {
    const auto a = artists_.count * kGroupBytes, b = albums_.count * kGroupBytes;
    for (std::size_t i = 0; i < size; ++i) {
        auto at = offset + static_cast<std::uint32_t>(i);
        if (at < a + b) {
            const auto& f = at < a ? artists_ : albums_; if (at >= a) at -= a;
            const auto& g = f.groups[at / kGroupBytes]; const auto byte = at % kGroupBytes;
            data[i] = byte < 64 ? static_cast<unsigned char>(g.name[byte]) :
                static_cast<unsigned char>((byte < 66 ? g.start : g.count) >> ((byte % 2) * 8));
        } else {
            at -= a + b; const auto& f = at < tracks_ * 2 ? artists_ : albums_; if (at >= tracks_ * 2) at -= tracks_ * 2;
            data[i] = static_cast<unsigned char>(f.ids[at / 2] >> ((at % 2) * 8));
        }
    }
}
void CatalogLookup::decode(std::uint32_t offset, const unsigned char* data, std::size_t size) {
    const auto a = artists_.count * kGroupBytes, b = albums_.count * kGroupBytes;
    for (std::size_t i = 0; i < size; ++i) {
        auto at = offset + static_cast<std::uint32_t>(i);
        if (at < a + b) {
            auto& f = at < a ? artists_ : albums_; if (at >= a) at -= a;
            auto& g = f.groups[at / kGroupBytes]; const auto byte = at % kGroupBytes;
            if (byte < 64) g.name[byte] = static_cast<char>(data[i]);
            else {
                auto& value = byte < 66 ? g.start : g.count;
                value = byte % 2 ? static_cast<std::uint16_t>(value | (std::uint16_t(data[i]) << 8)) : data[i];
            }
        } else {
            at -= a + b; auto& f = at < tracks_ * 2 ? artists_ : albums_; if (at >= tracks_ * 2) at -= tracks_ * 2;
            auto& value = f.ids[at / 2]; value = at % 2 ? static_cast<std::uint16_t>(value | (std::uint16_t(data[i]) << 8)) : data[i];
        }
    }
}
bool CatalogLookup::write_step(std::FILE* file) {
    if (!ready_ || !file) return false;
    std::array<unsigned char, 1024> data{};
    const auto size = std::min<std::uint32_t>(data.size(), bytes() - wire_cursor_);
    encode(wire_cursor_, data.data(), size);
    if (std::fwrite(data.data(), 1, size, file) != size) return false;
    checksum_ = hash_more(checksum_, data.data(), size); wire_cursor_ += size; return true;
}
bool CatalogLookup::begin_load(std::uint32_t tracks, std::uint32_t artists, std::uint32_t albums, std::uint32_t length, std::uint32_t checksum) {
    if (tracks > kTracks || artists > kGroups || albums > kGroups || (!tracks && (artists || albums)) ||
        (tracks && (!artists || !albums)) || length != (artists + albums) * kGroupBytes + tracks * 4) return false;
    tracks_ = tracks; artists_.count = artists; albums_.count = albums;
    expected_ = checksum; wire_cursor_ = 0; checksum_ = 2166136261u; ready_ = failed_ = false; return true;
}
bool CatalogLookup::validate(Field& f) {
    std::uint32_t start = 0;
    for (std::uint32_t i = 0; i < f.count; ++i) {
        const auto& g = f.groups[i];
        if (!std::memchr(g.name.data(), 0, g.name.size()) || !g.count || g.start != start) return false;
        start += g.count; if (start > tracks_) return false;
        f.order[i] = static_cast<std::uint16_t>(i);
    }
    if (start != tracks_) return false;
    std::sort(f.order.begin(), f.order.begin() + f.count, [&f](auto a, auto b) { return std::strcmp(key(f.groups[a]), key(f.groups[b])) < 0; });
    for (std::uint32_t i = 1; i < f.count; ++i) if (!std::strcmp(key(f.groups[f.order[i - 1]]), key(f.groups[f.order[i]]))) return false;
    std::fill(f.assignments.begin(), f.assignments.begin() + tracks_, UINT16_MAX);
    for (std::uint32_t i = 0; i < tracks_; ++i) {
        const auto id = f.ids[i]; if (id >= tracks_ || f.assignments[id] != UINT16_MAX) return false;
        f.assignments[id] = static_cast<std::uint16_t>(i);
    }
    return true;
}
bool CatalogLookup::load_step(std::FILE* file, std::uint32_t offset) {
    if (!file || failed_) return false;
    if (ready_) return true;
    std::array<unsigned char, 1024> data{}; const auto size = std::min<std::uint32_t>(data.size(), bytes() - wire_cursor_);
    if (std::fseek(file, static_cast<long>(offset + wire_cursor_), SEEK_SET) || std::fread(data.data(), 1, size, file) != size) { failed_ = true; return false; }
    decode(wire_cursor_, data.data(), size); checksum_ = hash_more(checksum_, data.data(), size); wire_cursor_ += size;
    if (wire_cursor_ == bytes()) {
        ready_ = checksum_ == expected_ && validate(artists_) && validate(albums_); failed_ = !ready_;
    }
    return !failed_;
}
const CatalogLookup::Field& CatalogLookup::field(CatalogFilter filter) const { return filter == CatalogFilter::kArtist ? artists_ : albums_; }
const CatalogLookup::Field& CatalogLookup::field(CatalogView view) const { return view == CatalogView::kArtists ? artists_ : albums_; }
std::uint32_t CatalogLookup::find(const Field& f, const char* text) {
    std::uint32_t low = 0, high = f.count;
    while (low < high) { const auto middle = (low + high) / 2; if (std::strcmp(key(f.groups[f.order[middle]]), text) < 0) low = middle + 1; else high = middle; }
    return low < f.count && !std::strcmp(key(f.groups[f.order[low]]), text) ? f.order[low] : UINT32_MAX;
}
std::uint32_t CatalogLookup::count(CatalogFilter filter, const char* text) const {
    if (filter == CatalogFilter::kNone) return tracks_;
    const auto& f = field(filter); const auto index = find(f, text); return index == UINT32_MAX ? 0 : f.groups[index].count;
}
std::uint32_t CatalogLookup::record(CatalogFilter filter, const char* text, std::uint32_t ordinal) const {
    if (filter == CatalogFilter::kNone) return ordinal < tracks_ ? ordinal : UINT32_MAX;
    const auto& f = field(filter); const auto index = find(f, text);
    return index == UINT32_MAX || ordinal >= f.groups[index].count ? UINT32_MAX : f.ids[f.groups[index].start + ordinal];
}
std::uint32_t CatalogLookup::groups(CatalogView view) const { return field(view).count; }
const char* CatalogLookup::name(CatalogView view, std::uint32_t sorted_index) const {
    const auto& f = field(view); return sorted_index < f.count ? f.groups[f.order[sorted_index]].name.data() : "";
}
void CatalogLookup::range(CatalogView view, const char* anchor, bool backwards, std::uint32_t& start, std::uint32_t& end, bool& more) const {
    const auto& f = field(view); std::uint32_t low = 0, high = f.count;
    if (*anchor) {
        while (low < high) {
            const auto middle = (low + high) / 2; const int relative = std::strcmp(key(f.groups[f.order[middle]]), anchor);
            if (relative < 0 || (!backwards && !relative)) low = middle + 1; else high = middle;
        }
    } else low = backwards ? f.count : 0;
    if (backwards) { end = low; start = end > 16 ? end - 16 : 0; more = start != 0; }
    else { start = low; end = std::min(f.count, start + 16); more = end < f.count; }
}
}
