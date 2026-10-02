#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <dirent.h>
#include <memory>
#include "nightwave/media_documents.h"
#include "nightwave/catalog_lookup.h"
#include "nightwave/mp3_seek_index.h"
namespace nightwave {
struct CatalogRecord {
    std::array<char, 256> path{};
    TrackMetadata metadata{};
};
struct CatalogPage {
    static constexpr std::size_t kSize = 16;
    std::array<CatalogRecord, kSize> rows{};
    std::size_t count{0};
    std::uint32_t matches{0}; // Song count, not a distinct-group count.
    bool complete{false}, more{false}, error{false};
};
// Single UI-task owner. Metadata/index writes run only when the caller permits
// idle work. Query reads are bounded per pump; no unbounded vectors or sorting.
// All cache files are inside the reserved root/.nightwave namespace.
class LibraryCatalog {
 public:
    static constexpr std::uint32_t kMaxTracks = 10000, kMaxDirectories = 4096,
        kMaxEntries = 100000;
    explicit LibraryCatalog(bool accelerate = true) : accelerate_(accelerate) {}
    ~LibraryCatalog();
    LibraryCatalog(const LibraryCatalog&) = delete;
    LibraryCatalog& operator=(const LibraryCatalog&) = delete;
    bool open(const char* root); // Header/length check only; records validated on use.
    bool rebuild();
    void pump_build(unsigned budget = 2); // At most 8 entries/operations per call.
    bool building() const { return writer_ != nullptr; }
    bool ready() const { return reader_ != nullptr; }
    bool limited() const { return limited_; }
    bool failed() const { return failed_; }
    std::uint32_t size() const { return count_; }
    std::uint32_t scanned() const { return walked_; }
    bool fast_lookup_ready() const { return reader_ && lookup_ && lookup_->ready(); }
    bool lookup_loading() const { return lookup_ && !lookup_->ready() && !lookup_->failed(); }
    void pump_lookup(unsigned chunks = 4); // At most four 1024-byte reads.
    std::uint32_t query_reads() const { return query_reads_; }
    bool query(CatalogView, CatalogFilter = CatalogFilter::kNone,
               const char* filter = "", std::uint32_t offset = 0,
               const char* group_anchor = "", bool backwards = false);
    bool seek_song(CatalogFilter filter, const char* text, std::uint32_t ordinal);
    bool locate_song(CatalogFilter filter, const char* text, const char* path, std::uint32_t hint = UINT32_MAX);
    std::uint32_t located_ordinal() const { return located_ordinal_; }
    void pump_query(unsigned budget = 4);
    void cancel_query() { querying_ = false; }
    bool querying() const { return querying_; }
    const CatalogPage& page() const { return page_; }
 private:
    bool read(std::uint32_t, CatalogRecord&);
    bool load(const char* suffix = "catalog-v2.bin", bool allocate_lookup = true);
    bool load_available();
    struct LookupDeleter { void operator()(CatalogLookup*) const; };
    using LookupPtr = std::unique_ptr<CatalogLookup, LookupDeleter>;
    static LookupPtr allocate_lookup();
    void finish_walk();
    void fast_query_step();
    bool append_record(const CatalogRecord&);
    void close_build();
    void fail_build();
    void publish();
    void clear_page();
    bool cache_path(const char* suffix, char* out, std::size_t capacity) const;
    const char* group(const CatalogRecord&) const;
    std::array<char, 256> root_{}, active_path_{};
    std::FILE *reader_{nullptr}, *writer_{nullptr}, *directories_{nullptr};
    DIR* directory_{nullptr};
    std::uint32_t count_{0}, new_count_{0}, walked_{0}, skipped_{0},
        directory_count_{0}, directory_cursor_{0}, query_cursor_{0}, offset_{0};
    std::uint8_t depth_{0};
    std::array<char, 256> locate_path_{};
    std::uint32_t located_ordinal_{UINT32_MAX};
    std::uint32_t header_bytes_{32}, query_end_{0}, locate_hint_{UINT32_MAX}, query_reads_{0};
    unsigned build_phase_{0};
    LookupPtr lookup_{}, build_lookup_{};
    Mp3SeekIndex seek_index_{};
    CatalogRecord pending_record_{};
    bool seek_pending_{false};
    bool limited_{false}, new_limited_{false}, failed_{false}, querying_{false}, backwards_{false}, first_only_{false}, locating_{false};
    bool accelerate_{true}, query_fast_{false}, hint_checked_{false};
    CatalogView view_{CatalogView::kSongs};
    CatalogFilter filter_{CatalogFilter::kNone};
    std::array<char, 64> filter_text_{}, anchor_{};
    CatalogPage page_{};
};
}
