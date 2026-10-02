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
    // Query lifetime/cancellation stress on actual parser/index sources.
    for (unsigned i = 0; i < 1000; ++i) {
        CHECK(boot.seek_song(CatalogFilter::kNone, "", i % 262)); finish_query(boot);
        CHECK(boot.page().complete && boot.page().count == 1 && !boot.page().error);
        CHECK(boot.query(CatalogView::kArtists)); boot.pump_query(1); boot.cancel_query(); CHECK(!boot.querying());
    }
    const auto cache = fixture / ".nightwave" / "catalog-v1.bin";
    std::ifstream input(cache, std::ios::binary); std::string original((std::istreambuf_iterator<char>(input)), {}); input.close();
    fs::remove(cache);
    LibraryCatalog recovery; CHECK(recovery.open(root.c_str()) && recovery.size() == 262);
    write(cache, original); fs::remove(fixture / ".nightwave" / "catalog-v1.bak");
    auto corrupt = original; corrupt[32 + 280] ^= 32; write(cache, corrupt);
    LibraryCatalog bad; CHECK(bad.open(root.c_str())); CHECK(bad.query(CatalogView::kSongs)); finish_query(bad);
    CHECK(bad.page().error && !bad.page().count); // No partial trusted page.
    corrupt = original; std::fill(corrupt.begin() + 32, corrupt.begin() + 32 + 256, '\0');
    const auto escape = root + "/../outside.mp3"; std::copy(escape.begin(), escape.end(), corrupt.begin() + 32);
    put(corrupt, 32 + 484, hash(corrupt.data() + 32, 484)); write(cache, corrupt);
    LibraryCatalog escape_cache; CHECK(escape_cache.open(root.c_str())); CHECK(escape_cache.query(CatalogView::kSongs)); finish_query(escape_cache);
    CHECK(escape_cache.page().error); // Recomputed checksum still cannot escape SD root.
    write(cache, original.substr(0, 33)); LibraryCatalog short_cache; CHECK(!short_cache.open(root.c_str()));
    write(cache, std::string(100000, 'x')); LibraryCatalog garbage; CHECK(!garbage.open(root.c_str()));
    write(cache, original);
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
    if (failures) return EXIT_FAILURE;
    std::cout << "Catalog tests passed: 262 tracks, nested traversal, song/group paging, cache reload, corruption/escape rejection, depth limit, symlink refusal, 1000 query/cancel cycles\n";
}
