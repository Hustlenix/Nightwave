#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "nightwave/library_catalog_types.h"
namespace nightwave {
// Bounded accelerator, allocated explicitly in PSRAM by LibraryCatalog on ESP.
// Track->group assignments are build scratch; grouped IDs support O(1) ordinal
// lookup. No path/metadata vectors and no SD reads in group-name comparisons.
class CatalogLookup {
 public:
    static constexpr std::size_t kGroups = 1024, kTracks = 10000;
    bool add(const TrackMetadata&, std::uint16_t track);
    void prepare(std::uint32_t tracks);
    bool seal_step(); // At most 64 track assignments, no I/O.
    bool write_step(std::FILE*); // At most 1024 bytes. False on I/O error.
    bool written() const { return wire_cursor_ == bytes(); }
    std::uint32_t bytes() const;
    std::uint32_t checksum() const { return checksum_; }
    bool begin_load(std::uint32_t tracks, std::uint32_t artists, std::uint32_t albums,
                    std::uint32_t length, std::uint32_t checksum);
    bool load_step(std::FILE*, std::uint32_t offset); // At most 1024 bytes.
    bool ready() const { return ready_; }
    bool failed() const { return failed_; }
    std::uint32_t count(CatalogFilter, const char* text) const;
    std::uint32_t record(CatalogFilter, const char* text, std::uint32_t ordinal) const;
    std::uint32_t groups(CatalogView) const;
    const char* name(CatalogView, std::uint32_t sorted_index) const;
    void range(CatalogView, const char* anchor, bool backwards, std::uint32_t& start,
               std::uint32_t& end, bool& more) const;
    static constexpr std::size_t allocation_bytes();
 private:
    struct Group { std::array<char, 64> name{}; std::uint16_t start{0}, count{0}; };
    struct Field {
        std::array<Group, kGroups> groups{};
        std::array<std::uint16_t, kTracks> ids{}, assignments{};
        std::array<std::uint16_t, kGroups> order{}, cursors{};
        std::uint32_t count{0};
    };
    bool add(Field&, const char*, std::uint16_t);
    const Field& field(CatalogFilter) const;
    const Field& field(CatalogView) const;
    static const char* key(const Group&);
    static std::uint32_t find(const Field&, const char*);
    bool validate(Field&);
    void encode(std::uint32_t offset, unsigned char* bytes, std::size_t size) const;
    void decode(std::uint32_t offset, const unsigned char* bytes, std::size_t size);
    Field artists_{}, albums_{};
    std::uint32_t tracks_{0}, seal_cursor_{0}, wire_cursor_{0}, checksum_{2166136261u}, expected_{0};
    bool ready_{false}, failed_{false};
};
constexpr std::size_t CatalogLookup::allocation_bytes() { return sizeof(CatalogLookup); }
static_assert(CatalogLookup::allocation_bytes() <= 256 * 1024, "Lookup generation exceeds PSRAM budget");
}
