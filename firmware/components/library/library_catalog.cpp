#include "nightwave/library_catalog.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <new>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
namespace nightwave {
namespace {
constexpr std::size_t kHeader = 64, kRecord = 488, kDirectory = 257;
std::uint32_t hash(const void* data, std::size_t length) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::uint32_t value = 2166136261u;
    for (std::size_t i = 0; i < length; ++i) value = (value ^ bytes[i]) * 16777619u;
    return value;
}
void put(unsigned char* p, std::uint32_t v) { for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<unsigned char>(v >> (i * 8)); }
std::uint32_t get(const unsigned char* p) {
    std::uint32_t v = 0; for (unsigned i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(p[i]) << (i * 8); return v;
}
bool info(const char* path, struct stat& value) {
#ifdef ESP_PLATFORM
    return stat(path, &value) == 0; // FAT has no symbolic links.
#else
    return lstat(path, &value) == 0; // Do not follow host-fixture symlinks.
#endif
}
bool terminated(const unsigned char* p, std::size_t n) { return std::memchr(p, 0, n) != nullptr; }
bool audio(const char* name) {
    const auto* dot = std::strrchr(name, '.');
    return dot && (!strcasecmp(dot, ".wav") || !strcasecmp(dot, ".mp3"));
}
bool regular_or_missing(const char* path) {
    struct stat value{};
    return info(path, value) ? S_ISREG(value.st_mode) : errno == ENOENT;
}
}
LibraryCatalog::~LibraryCatalog() { close_build(); if (reader_) std::fclose(reader_); }
void LibraryCatalog::LookupDeleter::operator()(CatalogLookup* p) const {
    if (!p) return;
#ifdef ESP_PLATFORM
    p->~CatalogLookup(); heap_caps_free(p);
#else
    delete p;
#endif
}
LibraryCatalog::LookupPtr LibraryCatalog::allocate_lookup() {
#ifdef ESP_PLATFORM
    auto* memory = heap_caps_malloc(sizeof(CatalogLookup), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return LookupPtr(memory ? new (memory) CatalogLookup : nullptr);
#else
    return LookupPtr(new (std::nothrow) CatalogLookup);
#endif
}
bool LibraryCatalog::cache_path(const char* suffix, char* out, std::size_t capacity) const {
    constexpr char prefix[] = "/.nightwave/";
    const auto root_size = std::strlen(root_.data()), suffix_size = std::strlen(suffix);
    if (root_size >= capacity || sizeof(prefix) > capacity - root_size ||
        suffix_size >= capacity - root_size - (sizeof(prefix) - 1)) return false;
    std::memcpy(out, root_.data(), root_size);
    std::memcpy(out + root_size, prefix, sizeof(prefix) - 1);
    std::memcpy(out + root_size + sizeof(prefix) - 1, suffix, suffix_size + 1);
    return true;
}
bool LibraryCatalog::open(const char* root) {
    close_build(); if (reader_) { std::fclose(reader_); reader_ = nullptr; }
    lookup_.reset();
    root_[0] = 0; querying_ = false; count_ = 0; failed_ = false; limited_ = false;
    if (!root || !*root || std::strlen(root) > 218 || root[0] != '/' || std::strstr(root, "/../") ||
        std::strstr(root, "/./") || std::strstr(root, "//") || std::strchr(root, '\\') ||
        root[std::strlen(root) - 1] == '/' || !std::strcmp(root + std::strlen(root) - std::min<std::size_t>(2, std::strlen(root)), "/.") ||
        !std::strcmp(root + std::strlen(root) - std::min<std::size_t>(3, std::strlen(root)), "/..")) return false;
    struct stat value{};
    if (!info(root, value) || !S_ISDIR(value.st_mode)) return false;
    std::strcpy(root_.data(), root);
    return load_available();
}
bool LibraryCatalog::load_available() {
    return load() || load("catalog-v2.bak") || load("catalog-v1.bin") || load("catalog-v1.bak");
}
bool LibraryCatalog::load(const char* suffix, bool allocate) {
    std::array<char, 256> path{};
    cache_path("", path.data(), path.size()); path[std::strlen(path.data()) - 1] = 0;
    struct stat directory{};
    if (!info(path.data(), directory) || !S_ISDIR(directory.st_mode)) return false;
    cache_path(suffix, path.data(), path.size());
    struct stat value{};
    if (!info(path.data(), value) || !S_ISREG(value.st_mode)) return false;
    auto* file = std::fopen(path.data(), "rb");
    if (!file) return false;
    std::array<unsigned char, kHeader> bytes{};
    bool valid = std::fread(bytes.data(), 1, 32, file) == 32;
    const bool legacy = valid && !std::memcmp(bytes.data(), "NWCAT001", 8);
    const auto header = legacy ? 32u : 64u;
    if (!legacy) valid = valid && !std::memcmp(bytes.data(), "NWCAT002", 8) && std::fread(bytes.data() + 32, 1, 32, file) == 32;
    const auto flags = get(bytes.data() + 24), tracks = get(bytes.data() + 12);
    const auto footer = legacy ? 0u : get(bytes.data() + 40);
    valid = valid && get(bytes.data() + 8) == hash(root_.data(), std::strlen(root_.data())) && tracks <= kMaxTracks &&
        flags <= (legacy ? 1u : 3u) && get(bytes.data() + (legacy ? 28 : 60)) == hash(bytes.data(), legacy ? 28 : 60) &&
        value.st_size == static_cast<off_t>(header + kRecord * tracks + footer);
    if (!legacy) {
        const auto a = get(bytes.data() + 32), b = get(bytes.data() + 36);
        valid = valid && a <= CatalogLookup::kGroups && b <= CatalogLookup::kGroups &&
            (flags & 2 ? footer == (a + b) * 68 + tracks * 4 && (!tracks || (a && b)) : !footer && !a && !b);
    }
    if (!valid) { std::fclose(file); return false; }
    lookup_.reset(); reader_ = file; count_ = tracks; limited_ = (flags & 1) != 0; header_bytes_ = header;
    if (!legacy && (flags & 2) && accelerate_ && allocate) {
        lookup_ = allocate_lookup();
        if (lookup_ && !lookup_->begin_load(count_, get(bytes.data() + 32), get(bytes.data() + 36), footer, get(bytes.data() + 44))) lookup_.reset();
    }
    return true;
}
void LibraryCatalog::close_build() {
    if (directory_) { closedir(directory_); directory_ = nullptr; }
    if (directories_) { std::fclose(directories_); directories_ = nullptr; }
    if (writer_) { std::fclose(writer_); writer_ = nullptr; }
    build_lookup_.reset(); build_phase_ = 0;
    seek_index_.cancel(); seek_pending_ = false;
}
void LibraryCatalog::fail_build() { failed_ = true; close_build(); }
bool LibraryCatalog::rebuild() {
    close_build(); failed_ = false; walked_ = skipped_ = new_count_ = directory_cursor_ = 0;
    directory_count_ = 1; depth_ = 0; new_limited_ = false;
    if (!root_[0]) { failed_ = true; return false; }
    std::array<char, 256> path{};
    cache_path("", path.data(), path.size()); path[std::strlen(path.data()) - 1] = 0;
    struct stat value{};
    if (info(path.data(), value)) { if (!S_ISDIR(value.st_mode)) { failed_ = true; return false; } }
    else if (errno != ENOENT || mkdir(path.data(), 0700)) { failed_ = true; return false; }
    for (const auto* suffix : {"catalog-v2.tmp", "directories-v2.tmp", "catalog-v2.bin", "catalog-v2.bak"}) {
        cache_path(suffix, path.data(), path.size());
        if (!regular_or_missing(path.data())) { failed_ = true; return false; }
    }
    cache_path("catalog-v2.tmp", path.data(), path.size()); writer_ = std::fopen(path.data(), "w+b");
    cache_path("directories-v2.tmp", path.data(), path.size()); directories_ = std::fopen(path.data(), "w+b");
    std::array<unsigned char, kHeader> header{};
    std::array<unsigned char, kDirectory> first{}; std::strcpy(reinterpret_cast<char*>(first.data()), root_.data());
    if (!writer_ || !directories_ || std::fwrite(header.data(), 1, header.size(), writer_) != header.size() ||
        std::fwrite(first.data(), 1, first.size(), directories_) != first.size()) { fail_build(); return false; }
    if (accelerate_) build_lookup_ = allocate_lookup();
    return true;
}
void LibraryCatalog::pump_build(unsigned budget) {
    budget = std::min(budget, 8u);
    while (writer_ && budget--) {
        if (seek_pending_) {
            seek_index_.step(1);
            if (seek_index_.status() != SeekIndexStatus::kBuilding) {
                if (seek_index_.ready()) {
                    pending_record_.metadata.duration_ms = seek_index_.duration_ms();
                    seek_index_.save(root_.data()); // Optional cache failure never drops a track.
                }
                seek_pending_ = false;
                if (!append_record(pending_record_)) return;
            }
            continue;
        }
        if (build_phase_ == 1) { if (build_lookup_->seal_step()) build_phase_ = 2; continue; }
        if (build_phase_ == 2) {
            if (!build_lookup_->write_step(writer_)) { fail_build(); return; }
            if (build_lookup_->written()) { publish(); return; } continue;
        }
        if (new_count_ == kMaxTracks || walked_ == kMaxEntries) { new_limited_ = true; finish_walk(); continue; }
        if (!directory_) {
            if (directory_cursor_ == directory_count_) { finish_walk(); continue; }
            std::array<unsigned char, kDirectory> row{};
            if (std::fseek(directories_, static_cast<long>(directory_cursor_++ * kDirectory), SEEK_SET) ||
                std::fread(row.data(), 1, row.size(), directories_) != row.size()) { fail_build(); return; }
            std::memcpy(active_path_.data(), row.data(), 256); depth_ = row[256];
            directory_ = opendir(active_path_.data());
            if (!directory_) { fail_build(); return; }
            continue;
        }
        errno = 0;
        const auto* entry = readdir(directory_);
        if (!entry) {
            const bool error = errno != 0; closedir(directory_); directory_ = nullptr;
            if (error) { fail_build(); return; } continue;
        }
        ++walked_;
        if (entry->d_name[0] == '.') continue;
        CatalogRecord record{};
        const int length = std::snprintf(record.path.data(), record.path.size(), "%s/%s", active_path_.data(), entry->d_name);
        if (length <= 0 || static_cast<std::size_t>(length) >= record.path.size()) { ++skipped_; new_limited_ = true; continue; }
        struct stat value{};
        if (!info(record.path.data(), value)) { ++skipped_; continue; }
        if (S_ISDIR(value.st_mode)) {
            if (depth_ == 8 || directory_count_ == kMaxDirectories) { ++skipped_; new_limited_ = true; continue; }
            std::array<unsigned char, kDirectory> row{}; std::memcpy(row.data(), record.path.data(), 256); row[256] = depth_ + 1;
            if (std::fseek(directories_, 0, SEEK_END) || std::fwrite(row.data(), 1, row.size(), directories_) != row.size()) { fail_build(); return; }
            ++directory_count_; continue;
        }
        if (!S_ISREG(value.st_mode) || !audio(entry->d_name)) continue;
        // Invalid/empty audio remains browsable with a filename fallback. The
        // decoder, not an index or metadata tag, determines actual playability.
        read_metadata(record.path.data(), record.metadata);
        const auto* extension = std::strrchr(record.path.data(), '.');
        if (extension && !strcasecmp(extension, ".mp3")) {
            Mp3SeekPoint cached{};
            if (Mp3SeekIndex::cached_seek(record.path.data(), root_.data(), 0, cached)) record.metadata.duration_ms = cached.duration_ms;
            else if (seek_index_.begin(record.path.data())) { pending_record_ = record; seek_pending_ = true; continue; }
        }
        if (!append_record(record)) return;
    }
}
bool LibraryCatalog::append_record(const CatalogRecord& record) {
    if (build_lookup_ && !build_lookup_->add(record.metadata, static_cast<std::uint16_t>(new_count_))) build_lookup_.reset();
    std::array<unsigned char, kRecord> bytes{};
    std::memcpy(bytes.data(), record.path.data(), 256); std::memcpy(bytes.data() + 256, record.metadata.title.data(), 96);
    std::memcpy(bytes.data() + 352, record.metadata.artist.data(), 64); std::memcpy(bytes.data() + 416, record.metadata.album.data(), 64);
    put(bytes.data() + 480, record.metadata.duration_ms); put(bytes.data() + 484, hash(bytes.data(), 484));
    if (std::fwrite(bytes.data(), 1, bytes.size(), writer_) != bytes.size()) { fail_build(); return false; }
    ++new_count_; return true;
}
void LibraryCatalog::finish_walk() {
    if (directory_) { closedir(directory_); directory_ = nullptr; }
    if (directories_) { std::fclose(directories_); directories_ = nullptr; }
    if (!build_lookup_) { publish(); return; }
    build_lookup_->prepare(new_count_);
    if (build_lookup_->failed()) { fail_build(); return; }
    build_phase_ = 1;
}
void LibraryCatalog::publish() {
    std::array<unsigned char, kHeader> bytes{}; std::memcpy(bytes.data(), "NWCAT002", 8);
    put(bytes.data() + 8, hash(root_.data(), std::strlen(root_.data()))); put(bytes.data() + 12, new_count_);
    put(bytes.data() + 16, skipped_); put(bytes.data() + 20, directory_count_);
    put(bytes.data() + 24, (new_limited_ ? 1u : 0u) | (build_lookup_ ? 2u : 0u));
    if (build_lookup_) {
        put(bytes.data() + 32, build_lookup_->groups(CatalogView::kArtists)); put(bytes.data() + 36, build_lookup_->groups(CatalogView::kAlbums));
        put(bytes.data() + 40, build_lookup_->bytes()); put(bytes.data() + 44, build_lookup_->checksum());
    }
    put(bytes.data() + 60, hash(bytes.data(), 60));
    if (std::fseek(writer_, 0, SEEK_SET) || std::fwrite(bytes.data(), 1, bytes.size(), writer_) != bytes.size() ||
        std::fflush(writer_) || fsync(fileno(writer_))) { fail_build(); return; }
    auto* finished = writer_; writer_ = nullptr;
    auto completed_lookup = std::move(build_lookup_);
    const bool close_error = std::fclose(finished) != 0; close_build();
    if (close_error) { failed_ = true; return; }
    std::array<char, 256> final{}, temporary{}, backup{};
    cache_path("catalog-v2.bin", final.data(), final.size()); cache_path("catalog-v2.tmp", temporary.data(), temporary.size());
    cache_path("catalog-v2.bak", backup.data(), backup.size());
    // Only these reserved cache files are replaced. Keep the prior generation
    // until replacement succeeds. Rename durability after power loss is a bench gate.
    if (reader_) { std::fclose(reader_); reader_ = nullptr; }
    querying_ = false;
    struct stat value{}; const bool had_old = info(final.data(), value);
    if (had_old && ((unlink(backup.data()) && errno != ENOENT) || rename(final.data(), backup.data()))) {
        failed_ = true; load_available(); return;
    }
    if (rename(temporary.data(), final.data())) {
        if (had_old) rename(backup.data(), final.data());
        failed_ = true; load_available(); return;
    }
    if (!load("catalog-v2.bin", false)) failed_ = true;
    else lookup_ = std::move(completed_lookup);
}
void LibraryCatalog::pump_lookup(unsigned chunks) {
    chunks = std::min(chunks, 4u);
    while (lookup_loading() && chunks--) if (!lookup_->load_step(reader_, header_bytes_ + count_ * kRecord)) { lookup_.reset(); return; }
}
bool LibraryCatalog::read(std::uint32_t index, CatalogRecord& result) {
    if (!reader_ || index >= count_) return false;
    std::array<unsigned char, kRecord> bytes{};
    if (query_reads_ != UINT32_MAX) ++query_reads_;
    if (std::fseek(reader_, static_cast<long>(header_bytes_ + index * kRecord), SEEK_SET) ||
        std::fread(bytes.data(), 1, bytes.size(), reader_) != bytes.size() ||
        get(bytes.data() + 484) != hash(bytes.data(), 484) || !terminated(bytes.data(), 256) ||
        !terminated(bytes.data() + 256, 96) || !terminated(bytes.data() + 352, 64) || !terminated(bytes.data() + 416, 64)) return false;
    const auto* path = reinterpret_cast<const char*>(bytes.data()); const auto root_size = std::strlen(root_.data());
    std::array<char, 256> checked{};
    if (std::strncmp(path, root_.data(), root_size) || path[root_size] != '/' ||
        !local_media_path(root_.data(), path + root_size + 1, root_.data(), checked.data(), checked.size()) || std::strcmp(path, checked.data())) return false;
    std::memcpy(result.path.data(), bytes.data(), 256); std::memcpy(result.metadata.title.data(), bytes.data() + 256, 96);
    std::memcpy(result.metadata.artist.data(), bytes.data() + 352, 64); std::memcpy(result.metadata.album.data(), bytes.data() + 416, 64);
    result.metadata.duration_ms = get(bytes.data() + 480); return true;
}
const char* LibraryCatalog::group(const CatalogRecord& row) const {
    const auto* value = view_ == CatalogView::kArtists ? row.metadata.artist.data() : row.metadata.album.data();
    return *value ? value : "(UNKNOWN)";
}
bool LibraryCatalog::query(CatalogView view, CatalogFilter filter, const char* text, std::uint32_t offset,
                           const char* anchor, bool backwards) {
    querying_ = false; clear_page();
    if (!ready() || !text || !anchor || std::strlen(text) >= 64 || std::strlen(anchor) >= 64 ||
        static_cast<unsigned>(view) > 2 || static_cast<unsigned>(filter) > 2 || offset > kMaxTracks) { page_.error = true; return false; }
    view_ = view; filter_ = filter; offset_ = offset; backwards_ = backwards;
    std::strcpy(filter_text_.data(), text); std::strcpy(anchor_.data(), anchor);
    first_only_ = locating_ = false; located_ordinal_ = UINT32_MAX;
    query_reads_ = 0; locate_hint_ = UINT32_MAX; hint_checked_ = false;
    query_fast_ = fast_lookup_ready() && (view == CatalogView::kSongs || filter == CatalogFilter::kNone);
    query_cursor_ = view == CatalogView::kSongs && filter == CatalogFilter::kNone ? std::min(offset, count_) : 0;
    page_.matches = query_cursor_;
    if (query_fast_) {
        page_.matches = lookup_->count(filter, text);
        if (view == CatalogView::kSongs) { query_cursor_ = std::min(offset, page_.matches); query_end_ = page_.matches; }
        else lookup_->range(view, anchor, backwards, query_cursor_, query_end_, page_.more);
    }
    querying_ = true; return true;
}
bool LibraryCatalog::seek_song(CatalogFilter filter, const char* text, std::uint32_t ordinal) {
    if (!query(CatalogView::kSongs, filter, text, ordinal)) return false;
    first_only_ = true; return true;
}
bool LibraryCatalog::locate_song(CatalogFilter filter, const char* text, const char* path, std::uint32_t hint) {
    if (!path || std::strlen(path) >= locate_path_.size() || !query(CatalogView::kSongs, filter, text)) return false;
    std::strcpy(locate_path_.data(), path); locating_ = true; locate_hint_ = hint; return true;
}
void LibraryCatalog::fast_query_step() {
    if (view_ != CatalogView::kSongs) {
        if (query_cursor_ == query_end_) { page_.complete = true; querying_ = false; return; }
        CatalogRecord row{};
        auto& label = view_ == CatalogView::kArtists ? row.metadata.artist : row.metadata.album;
        std::strcpy(label.data(), lookup_->name(view_, query_cursor_++));
        page_.rows[page_.count++] = row;
        if (query_cursor_ == query_end_) { page_.complete = true; querying_ = false; } return;
    }
    // A resume hint is only an acceleration hint: path and tag are checked,
    // never trust a saved ordinal after a card/index generation changes.
    if (locating_ && !hint_checked_) {
        hint_checked_ = true;
        if (locate_hint_ < query_end_) {
            CatalogRecord hinted{};
            if (read(lookup_->record(filter_, filter_text_.data(), locate_hint_), hinted) &&
                !std::strcmp(hinted.path.data(), locate_path_.data())) {
                const auto* text = filter_ == CatalogFilter::kArtist ? hinted.metadata.artist.data() : hinted.metadata.album.data();
                if (filter_ == CatalogFilter::kNone || !std::strcmp(*text ? text : "(UNKNOWN)", filter_text_.data())) {
                    page_.rows[0] = hinted; page_.count = 1; located_ordinal_ = locate_hint_;
                    page_.complete = true; querying_ = false; return;
                }
            }
            return; // This slice performed one record read; search next slice.
        }
    }
    if (query_cursor_ == query_end_) { page_.complete = true; querying_ = false; return; }
    const auto ordinal = query_cursor_++;
    CatalogRecord row{};
    if (!read(lookup_->record(filter_, filter_text_.data(), ordinal), row)) { clear_page(); page_.error = true; querying_ = false; return; }
    const auto* tag = filter_ == CatalogFilter::kArtist ? row.metadata.artist.data() : row.metadata.album.data();
    if (filter_ != CatalogFilter::kNone && std::strcmp(*tag ? tag : "(UNKNOWN)", filter_text_.data())) {
        clear_page(); page_.error = true; querying_ = false; return;
    }
    if (locating_) {
        if (!std::strcmp(row.path.data(), locate_path_.data())) {
            page_.rows[0] = row; page_.count = 1; located_ordinal_ = ordinal; page_.complete = true; querying_ = false;
        }
        return;
    }
    page_.rows[page_.count++] = row; page_.more = offset_ + page_.count < page_.matches;
    if (first_only_ || page_.count == page_.rows.size() || query_cursor_ == query_end_) { page_.complete = true; querying_ = false; }
}
void LibraryCatalog::pump_query(unsigned budget) {
    budget = std::min(budget, 8u);
    while (querying_ && budget--) {
        if (query_fast_) { fast_query_step(); continue; }
        if (query_cursor_ == count_) { page_.complete = true; querying_ = false; return; }
        CatalogRecord row{};
        if (!read(query_cursor_++, row)) { clear_page(); page_.error = true; querying_ = false; return; }
        const char* tag = filter_ == CatalogFilter::kArtist ? row.metadata.artist.data() : row.metadata.album.data();
        if (!*tag) tag = "(UNKNOWN)";
        if (filter_ != CatalogFilter::kNone && std::strcmp(tag, filter_text_.data())) continue;
        const auto match = page_.matches++;
        if (view_ == CatalogView::kSongs) {
            if (locating_) {
                if (!std::strcmp(row.path.data(), locate_path_.data())) { page_.rows[0] = row; page_.count = 1; located_ordinal_ = match; }
                continue;
            }
            if (match < offset_) continue;
            if (page_.count < page_.rows.size()) page_.rows[page_.count++] = row;
            else page_.more = true;
            if (first_only_ || (filter_ == CatalogFilter::kNone && page_.count == page_.rows.size())) {
                if (filter_ == CatalogFilter::kNone) { page_.matches = count_; page_.more = offset_ + page_.count < count_; }
                page_.complete = true; querying_ = false; return;
            }
            continue;
        }
        const auto* key = group(row);
        const int relative = std::strcmp(key, anchor_.data());
        if (*anchor_.data() && (backwards_ ? relative >= 0 : relative <= 0)) continue;
        std::size_t at = 0;
        while (at < page_.count && std::strcmp(group(page_.rows[at]), key) < 0) ++at;
        if (at < page_.count && !std::strcmp(group(page_.rows[at]), key)) continue;
        if (page_.count < page_.rows.size()) {
            for (auto i = page_.count; i > at; --i) page_.rows[i] = page_.rows[i - 1];
            page_.rows[at] = row; ++page_.count;
        } else {
            page_.more = true;
            if (backwards_) {
                if (!at) continue;
                for (std::size_t i = 1; i < at; ++i) page_.rows[i - 1] = page_.rows[i];
                page_.rows[at - 1] = row;
            } else if (at < page_.count) {
                for (auto i = page_.count - 1; i > at; --i) page_.rows[i] = page_.rows[i - 1];
                page_.rows[at] = row;
            }
        }
    }
}
void LibraryCatalog::clear_page() {
    page_.count = 0; page_.matches = 0; page_.complete = page_.more = page_.error = false;
}
}
