#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "nightwave/library_catalog.h"
namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << __LINE__ << ": " << #x << '\n'; } } while(false)
void write(const std::filesystem::path& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary); file.write(data.data(), static_cast<std::streamsize>(data.size()));
}
std::string tag(const std::string& title, const std::string& artist, const std::string& album) {
    std::string body;
    for (const auto& field : {std::make_pair("TIT2", title), std::make_pair("TPE1", artist), std::make_pair("TALB", album)}) {
        body += field.first; const auto size = field.second.size() + 1;
        for (int shift : {24, 16, 8, 0}) body += char(size >> shift);
        body += std::string(2, '\0') + char(3) + field.second;
    }
    std::string result("ID3\3\0\0", 6);
    for (int shift : {21, 14, 7, 0}) result += char((body.size() >> shift) & 127);
    return result + body; // Metadata-only originals; not falsely called playable audio.
}
void finish_build(nightwave::LibraryCatalog& catalog) {
    unsigned ticks = 0;
    while (catalog.building() && ticks++ < 60000) catalog.pump_build(1);
    CHECK(ticks < 60000 && !catalog.failed() && catalog.ready());
}
void finish_query(nightwave::LibraryCatalog& catalog) {
    unsigned ticks = 0;
    while (catalog.querying() && ticks++ < 20000) catalog.pump_query(1);
    CHECK(ticks < 20000);
}
void warm_lookup(nightwave::LibraryCatalog& catalog) {
    unsigned ticks = 0; while (catalog.lookup_loading() && ticks++ < 200) catalog.pump_lookup(1);
    CHECK(ticks < 200);
}
std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary); return std::string((std::istreambuf_iterator<char>(input)), {});
}
std::uint32_t get(const std::string& data, std::size_t at) {
    std::uint32_t value = 0; for (unsigned i = 0; i < 4; ++i) value |= std::uint32_t(static_cast<unsigned char>(data[at + i])) << (i * 8); return value;
}
std::string key(unsigned value) {
    char label[32]; std::snprintf(label, sizeof(label), "Artist %02u", value); return label;
}
std::uint32_t hash(const char* p, std::size_t n) {
    std::uint32_t v = 2166136261u;
    for (std::size_t i = 0; i < n; ++i) v = (v ^ static_cast<unsigned char>(p[i])) * 16777619u;
    return v;
}
void put(std::string& data, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) data[at + i] = char(value >> (i * 8));
}
}
int main(int argc, char** argv) {
    using namespace nightwave; namespace fs = std::filesystem;
    if (argc != 2) return EXIT_FAILURE;
    const auto fixture = fs::path(argv[1]) / "catalog-fixture";
    fs::remove_all(fixture); fs::create_directories(fixture / "nested");
    const auto root = fs::weakly_canonical(fixture).generic_string();
    for (unsigned i = 0; i < 260; ++i) {
        const auto path = fixture / (i % 2 ? "nested" : "") / ("song-" + std::to_string(i) + ".mp3");
        write(path, tag("Original " + std::to_string(i), key(i % 40), "Album " + std::to_string(i % 5)));
    }
    write(fixture / "empty.wav", ""); write(fixture / "broken.mp3", "ID3");
    write(fixture / "ignored.m3u", "song-0.mp3\n");
    fs::create_directory_symlink(fixture, fixture / "loop");
    LibraryCatalog catalog;
    CHECK(!catalog.open(root.c_str())); CHECK(catalog.rebuild()); CHECK(!catalog.ready());
    catalog.pump_build(0); CHECK(!catalog.scanned());
    finish_build(catalog); CHECK(catalog.size() == 262 && !catalog.limited());
    CHECK(catalog.fast_lookup_ready());
    CHECK(catalog.query(CatalogView::kSongs)); finish_query(catalog);
    CHECK(catalog.page().count == 16 && catalog.page().matches == 262 && catalog.page().more);
    std::set<std::string> paths;
    for (unsigned offset = 0; offset < 262; offset += 16) {
        CHECK(catalog.query(CatalogView::kSongs, CatalogFilter::kNone, "", offset)); finish_query(catalog);
        for (std::size_t i = 0; i < catalog.page().count; ++i) paths.insert(catalog.page().rows[i].path.data());
    }
    CHECK(paths.size() == 262);
    CHECK(catalog.seek_song(CatalogFilter::kNone, "", 200)); catalog.pump_query(1);
    CHECK(catalog.page().complete && catalog.page().count == 1); // Direct ordinal access, no whole-card scan.
    CHECK(catalog.query(CatalogView::kArtists)); finish_query(catalog);
    CHECK(catalog.page().count == 16 && catalog.page().more);
    CHECK(catalog.query_reads() == 0);
    CHECK(!std::strcmp(catalog.page().rows[0].metadata.artist.data(), "")); // Unknown sorts first.
    const std::string anchor = catalog.page().rows[15].metadata.artist.data();
    CHECK(anchor == key(14));
    CHECK(catalog.query(CatalogView::kArtists, CatalogFilter::kNone, "", 0, anchor.c_str())); finish_query(catalog);
    CHECK(!std::strcmp(catalog.page().rows[0].metadata.artist.data(), key(15).c_str()));
    CHECK(catalog.query(CatalogView::kArtists, CatalogFilter::kNone, "", 0, key(15).c_str(), true)); finish_query(catalog);
    CHECK(catalog.page().count == 16 && !catalog.page().more);
    CHECK(!std::strcmp(catalog.page().rows[15].metadata.artist.data(), key(14).c_str()));
    CHECK(catalog.query(CatalogView::kSongs, CatalogFilter::kArtist, key(2).c_str())); finish_query(catalog);
    CHECK(catalog.page().count == 7 && catalog.page().matches == 7);
    CHECK(catalog.query_reads() == 7);
    CHECK(catalog.locate_song(CatalogFilter::kArtist, key(2).c_str(), (fs::path(root) / "song-2.mp3").generic_string().c_str())); finish_query(catalog);
    CHECK(catalog.page().count == 1 && catalog.page().matches == 7 && catalog.located_ordinal() < 7);
    CHECK(catalog.locate_song(CatalogFilter::kNone, "", "/missing.mp3")); finish_query(catalog);
    CHECK(!catalog.page().count && catalog.located_ordinal() == UINT32_MAX && catalog.page().matches == 262);
    CHECK(catalog.query(CatalogView::kAlbums)); finish_query(catalog);
    CHECK(catalog.page().count == 6 && !catalog.page().more);
    // Live cache remains usable while a separate, incomplete generation exists.
    CHECK(catalog.rebuild()); catalog.pump_build(8); CHECK(catalog.ready() && catalog.size() == 262);
    CHECK(catalog.query(CatalogView::kSongs)); finish_query(catalog); CHECK(catalog.page().matches == 262);
    finish_build(catalog);
    LibraryCatalog boot; CHECK(boot.open(root.c_str()) && boot.size() == 262 && !boot.building());
    CHECK(boot.lookup_loading()); warm_lookup(boot); CHECK(boot.fast_lookup_ready());
    // Query lifetime/cancellation stress on actual parser/index sources.
    for (unsigned i = 0; i < 1000; ++i) {
        CHECK(boot.seek_song(CatalogFilter::kNone, "", i % 262)); finish_query(boot);
        CHECK(boot.page().complete && boot.page().count == 1 && !boot.page().error);
        CHECK(boot.query(CatalogView::kArtists)); boot.pump_query(1); boot.cancel_query(); CHECK(!boot.querying());
    }
    const auto cache = fixture / ".nightwave" / "catalog-v2.bin";
    std::ifstream input(cache, std::ios::binary); std::string original((std::istreambuf_iterator<char>(input)), {}); input.close();
    fs::remove(cache);
    LibraryCatalog recovery; CHECK(recovery.open(root.c_str()) && recovery.size() == 262);
    write(cache, original); fs::remove(fixture / ".nightwave" / "catalog-v2.bak");
    auto corrupt = original; corrupt[64 + 280] ^= 32; write(cache, corrupt);
    LibraryCatalog bad; CHECK(bad.open(root.c_str())); CHECK(bad.query(CatalogView::kSongs)); finish_query(bad);
    CHECK(bad.page().error && !bad.page().count); // No partial trusted page.
    corrupt = original; std::fill(corrupt.begin() + 64, corrupt.begin() + 64 + 256, '\0');
    const auto escape = root + "/../outside.mp3"; std::copy(escape.begin(), escape.end(), corrupt.begin() + 64);
    put(corrupt, 64 + 484, hash(corrupt.data() + 64, 484)); write(cache, corrupt);
    LibraryCatalog escape_cache; CHECK(escape_cache.open(root.c_str())); CHECK(escape_cache.query(CatalogView::kSongs)); finish_query(escape_cache);
    CHECK(escape_cache.page().error); // Recomputed checksum still cannot escape SD root.
    write(cache, original.substr(0, 33)); LibraryCatalog short_cache; CHECK(!short_cache.open(root.c_str()));
    write(cache, std::string(100000, 'x')); LibraryCatalog garbage; CHECK(!garbage.open(root.c_str()));
    write(cache, original);
    // Footer corruption disables only the accelerator, not bounded song browsing.
    const auto footer_at = 64 + 262 * 488;
    corrupt = original; corrupt[footer_at + 3] ^= 8; write(cache, corrupt);
    LibraryCatalog bad_lookup; CHECK(bad_lookup.open(root.c_str())); warm_lookup(bad_lookup);
    CHECK(!bad_lookup.fast_lookup_ready() && !bad_lookup.lookup_loading());
    CHECK(bad_lookup.query(CatalogView::kSongs, CatalogFilter::kArtist, key(2).c_str())); finish_query(bad_lookup);
    CHECK(bad_lookup.page().matches == 7 && bad_lookup.query_reads() == 262);
    // A recomputed checksum cannot make duplicate/out-of-range track IDs valid.
    corrupt = original;
    const auto ids_at = footer_at + (get(original, 32) + get(original, 36)) * 68;
    corrupt[ids_at + 2] = corrupt[ids_at]; corrupt[ids_at + 3] = corrupt[ids_at + 1];
    put(corrupt, 44, hash(corrupt.data() + footer_at, corrupt.size() - footer_at)); put(corrupt, 60, hash(corrupt.data(), 60)); write(cache, corrupt);
    LibraryCatalog duplicate; CHECK(duplicate.open(root.c_str())); warm_lookup(duplicate); CHECK(!duplicate.fast_lookup_ready());
    const auto reject_footer = [&](std::string bytes) {
        put(bytes, 44, hash(bytes.data() + footer_at, bytes.size() - footer_at)); put(bytes, 60, hash(bytes.data(), 60)); write(cache, bytes);
        LibraryCatalog hostile; CHECK(hostile.open(root.c_str())); warm_lookup(hostile); CHECK(!hostile.fast_lookup_ready());
    };
    corrupt = original; corrupt[ids_at] = char(0xff); corrupt[ids_at + 1] = char(0xff); reject_footer(corrupt);
    corrupt = original; std::fill(corrupt.begin() + footer_at, corrupt.begin() + footer_at + 64, 'x'); reject_footer(corrupt);
    corrupt = original; corrupt[footer_at + 64] = 1; reject_footer(corrupt); // Invalid nonzero first prefix.
    corrupt = original; std::copy_n(corrupt.begin() + footer_at, 64, corrupt.begin() + footer_at + 68); reject_footer(corrupt);
    // A valid permutation with group-mismatched IDs passes structural checks,
    // but every selected record still must match the requested actual tag.
    corrupt = original;
    const auto second_start = static_cast<unsigned char>(corrupt[footer_at + 68 + 64]) |
        (unsigned(static_cast<unsigned char>(corrupt[footer_at + 68 + 65])) << 8);
    for (unsigned i = 0; i < 2; ++i) std::swap(corrupt[ids_at + i], corrupt[ids_at + second_start * 2 + i]);
    put(corrupt, 44, hash(corrupt.data() + footer_at, corrupt.size() - footer_at)); put(corrupt, 60, hash(corrupt.data(), 60)); write(cache, corrupt);
    const std::string first_group(original.data() + footer_at);
    LibraryCatalog wrong_tag; CHECK(wrong_tag.open(root.c_str())); warm_lookup(wrong_tag); CHECK(wrong_tag.fast_lookup_ready());
    CHECK(wrong_tag.query(CatalogView::kSongs, CatalogFilter::kArtist, first_group.empty() ? "(UNKNOWN)" : first_group.c_str())); finish_query(wrong_tag);
    CHECK(wrong_tag.page().error && !wrong_tag.page().count);
    write(cache, original);
    // v1 remains readable without a lookup; rebuild creates v2, preserving v1.
    std::string legacy(32, '\0'); std::copy(original.begin(), original.begin() + 28, legacy.begin());
    legacy.replace(0, 8, "NWCAT001"); put(legacy, 24, get(original, 24) & 1); put(legacy, 28, hash(legacy.data(), 28));
    legacy.append(original, 64, 262 * 488);
    const auto old_cache = fixture / ".nightwave" / "catalog-v1.bin";
    fs::remove(cache); write(old_cache, legacy);
    LibraryCatalog migration; CHECK(migration.open(root.c_str()) && migration.size() == 262 && !migration.fast_lookup_ready());
    CHECK(migration.query(CatalogView::kSongs)); finish_query(migration); CHECK(migration.page().matches == 262);
    CHECK(migration.rebuild()); finish_build(migration); CHECK(migration.fast_lookup_ready() && read_file(old_cache) == legacy);
    CHECK(!garbage.query(CatalogView::kSongs)); CHECK(!garbage.open((root + "/..").c_str())); CHECK(!garbage.rebuild());
    // Explicit depth-limit reporting; root is depth 0 and depth 8 is included.
    auto deep = fixture; for (unsigned i = 0; i < 9; ++i) { deep /= "d"; fs::create_directory(deep); }
    write(deep / "too-deep.wav", "");
    CHECK(catalog.rebuild()); finish_build(catalog); CHECK(catalog.limited() && catalog.size() == 262);
    const auto untrusted = fs::path(argv[1]) / "catalog-symlink-fixture";
    fs::remove_all(untrusted); fs::create_directory(untrusted);
    fs::create_directory_symlink(fixture / ".nightwave", untrusted / ".nightwave");
    LibraryCatalog symlink; const auto unsafe_root = fs::weakly_canonical(untrusted).generic_string();
    CHECK(!symlink.open(unsafe_root.c_str()) && !symlink.rebuild());
    // Full capacity, real indexing and constant record-read budgets; no device
    // latency or acoustic result is inferred from these host operation counts.
    const auto large = fs::path(argv[1]) / "catalog-large-fixture";
    fs::remove_all(large); fs::create_directories(large);
    for (unsigned i = 0; i < 10000; ++i) write(large / ("original-" + std::to_string(i) + ".mp3"), tag("Original fixture", i % 2 ? "Odd" : "Even", "Test album"));
    const auto large_root = fs::weakly_canonical(large).generic_string();
    LibraryCatalog full; CHECK(!full.open(large_root.c_str()) && full.rebuild()); finish_build(full);
    CHECK(full.size() == 10000 && full.fast_lookup_ready());
    CHECK(full.query(CatalogView::kArtists)); finish_query(full); CHECK(full.page().count == 2 && full.query_reads() == 0);
    CHECK(full.query(CatalogView::kSongs, CatalogFilter::kArtist, "Odd", 4990)); finish_query(full);
    CHECK(full.page().count == 10 && full.page().matches == 5000 && full.query_reads() == 10 && !full.page().more);
    CHECK(full.seek_song(CatalogFilter::kArtist, "Odd", 4999)); full.pump_query(1);
    CHECK(full.page().complete && full.query_reads() == 1);
    const std::string last_path = full.page().rows[0].path.data();
    CHECK(full.locate_song(CatalogFilter::kArtist, "Odd", last_path.c_str(), 4999)); full.pump_query(1);
    CHECK(full.page().complete && full.query_reads() == 1 && full.located_ordinal() == 4999);
    CHECK(full.locate_song(CatalogFilter::kArtist, "Odd", last_path.c_str(), 0)); finish_query(full);
    CHECK(full.located_ordinal() == 4999 && full.query_reads() > 1); // Stale hint never selects another track.
    CHECK(full.query(CatalogView::kSongs, CatalogFilter::kAlbum, "Test album", 9984)); finish_query(full);
    CHECK(full.page().count == 16 && full.page().matches == 10000 && full.query_reads() == 16 && !full.page().more);
    CHECK(full.query(CatalogView::kSongs, CatalogFilter::kArtist, "Absent")); full.pump_query(1);
    CHECK(full.page().complete && !full.page().count && !full.query_reads());
    LibraryCatalog large_boot; CHECK(large_boot.open(large_root.c_str())); warm_lookup(large_boot);
    CHECK(large_boot.fast_lookup_ready()); CHECK(large_boot.seek_song(CatalogFilter::kArtist, "Odd", 4999)); large_boot.pump_query(1);
    CHECK(large_boot.page().complete && large_boot.query_reads() == 1);
    const auto overflow = fs::path(argv[1]) / "catalog-group-overflow";
    fs::remove_all(overflow); fs::create_directories(overflow);
    for (unsigned i = 0; i < 1025; ++i) write(overflow / ("original-" + std::to_string(i) + ".mp3"), tag("Original", key(i), "Test album"));
    const auto overflow_root = fs::weakly_canonical(overflow).generic_string();
    LibraryCatalog capped; CHECK(!capped.open(overflow_root.c_str()) && capped.rebuild()); finish_build(capped);
    CHECK(capped.size() == 1025 && !capped.fast_lookup_ready() && !capped.limited());
    CHECK(capped.query(CatalogView::kArtists)); finish_query(capped); CHECK(capped.page().count == 16 && capped.page().more && capped.query_reads() == 1025);
    fs::remove(overflow / "original-1024.mp3"); CHECK(capped.rebuild()); finish_build(capped);
    CHECK(capped.size() == 1024 && capped.fast_lookup_ready());
    CHECK(capped.seek_song(CatalogFilter::kArtist, key(1023).c_str(), 0)); capped.pump_query(1);
    CHECK(capped.page().complete && capped.query_reads() == 1 && capped.page().count == 1);
    LibraryCatalog low_memory(false); CHECK(low_memory.open(large_root.c_str()) && !low_memory.lookup_loading());
    CHECK(low_memory.seek_song(CatalogFilter::kNone, "", 9999)); low_memory.pump_query(1); CHECK(low_memory.page().complete);
    const auto empty = fs::path(argv[1]) / "catalog-empty-fixture";
    fs::remove_all(empty); fs::create_directories(empty);
    const auto empty_root = fs::weakly_canonical(empty).generic_string();
    LibraryCatalog empty_index; CHECK(!empty_index.open(empty_root.c_str()) && empty_index.rebuild()); finish_build(empty_index);
    CHECK(empty_index.fast_lookup_ready() && !empty_index.size());
    LibraryCatalog empty_boot; CHECK(empty_boot.open(empty_root.c_str())); warm_lookup(empty_boot); CHECK(empty_boot.fast_lookup_ready());
    CHECK(empty_boot.query(CatalogView::kArtists)); empty_boot.pump_query(1);
    CHECK(empty_boot.page().complete && !empty_boot.page().count && !empty_boot.query_reads());
    CHECK(CatalogLookup::allocation_bytes() <= 256 * 1024);
    if (failures) return EXIT_FAILURE;
    std::cout << "Catalog tests passed: 10000-track fast lookup, 1025-group fallback, v1 migration, CRC/ID/path rejection, stale resume hints, 1000 query/cancel cycles; lookup bytes " << CatalogLookup::allocation_bytes() << '\n';
}
