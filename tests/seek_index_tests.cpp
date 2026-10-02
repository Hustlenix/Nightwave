#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include "nightwave/mp3_seek_index.h"
namespace fs = std::filesystem;
namespace {
int failures = 0;
#define CHECK(v) do { if (!(v)) { ++failures; std::cerr << __LINE__ << ": " << #v << '\n'; } } while (false)
void write(const fs::path& path, const std::vector<unsigned char>& bytes) {
    std::ofstream f(path, std::ios::binary); f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
std::vector<unsigned char> frames(unsigned count) {
    // Structurally valid MPEG1 Layer III frames, not claimed audible/decodable.
    std::vector<unsigned char> bytes(count * 417, 0);
    for (unsigned i = 0; i < count; ++i) { auto* p = bytes.data() + i * 417; p[0] = 255; p[1] = 251; p[2] = 144; }
    return bytes;
}
void scan(nightwave::Mp3SeekIndex& index) {
    for (unsigned i = 0; i < 132000 && index.status() == nightwave::SeekIndexStatus::kBuilding; ++i) index.step(8);
}
}
int main(int argc, char** argv) {
    using namespace nightwave;
    if (argc != 2) return EXIT_FAILURE;
    std::array<char, 40> temporary{}; std::strcpy(temporary.data(), "/tmp/nightwave-seek-XXXXXX");
    const auto* created = mkdtemp(temporary.data()); if (!created) return EXIT_FAILURE;
    const fs::path root(created);
    fs::create_directories(root / ".nightwave");
    const auto media = root / "frames.mp3";
    auto bytes = frames(2000); write(media, bytes);
    auto index = std::make_unique<Mp3SeekIndex>();
    CHECK(index->begin(media.c_str())); index->step(1); CHECK(index->frame_count() == 1);
    scan(*index); CHECK(index->ready() && index->frame_count() == 2000);
    CHECK(index->duration_ms() == 52244); CHECK(index->save(root.c_str()));
    Mp3SeekPoint point;
    CHECK(Mp3SeekIndex::cached_seek(media.c_str(), root.c_str(), 30000, point));
    CHECK(point.samples > 0 && point.offset == point.samples / 1152 * 417);
    const auto target_frame = 30000ULL * 44100 / 1000 / 1152;
    CHECK(target_frame - point.samples / 1152 >= 128 && target_frame - point.samples / 1152 < 256);
    CHECK(!Mp3SeekIndex::cached_seek(media.c_str(), root.c_str(), 53000, point));
    for (const auto& entry : fs::directory_iterator(root / ".nightwave")) {
        std::fstream f(entry.path(), std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(304); f.put('X');
    }
    CHECK(!Mp3SeekIndex::cached_seek(media.c_str(), root.c_str(), 30000, point));
    CHECK(index->save(root.c_str())); bytes[10] = 42; write(media, bytes);
    CHECK(!Mp3SeekIndex::cached_seek(media.c_str(), root.c_str(), 30000, point));
    CHECK(!index->save(root.c_str())); // Changed while/after scanning must not publish old identity.
    bytes = frames(2000); bytes.pop_back(); write(media, bytes);
    CHECK(index->begin(media.c_str())); scan(*index); CHECK(index->status() == SeekIndexStatus::kInvalid);
    bytes = frames(2000); bytes[417 + 2] = 148; write(media, bytes);
    CHECK(index->begin(media.c_str())); scan(*index); CHECK(index->status() == SeekIndexStatus::kInvalid);
    bytes = frames(131073); write(media, bytes); bytes.clear(); bytes.shrink_to_fit();
    CHECK(index->begin(media.c_str())); scan(*index); CHECK(index->status() == SeekIndexStatus::kLimit);
    CHECK(!index->save(root.c_str()));
    bytes = frames(20);
    std::memcpy(bytes.data() + 36, "Xing", 4); bytes[43] = 1; bytes[47] = 20; write(media, bytes);
    auto* f = std::fopen(media.c_str(), "rb"); CHECK(f != nullptr);
    CHECK(Mp3SeekIndex::probe_duration(f, static_cast<std::uint32_t>(bytes.size())) == 522); std::fclose(f);
    bytes[44] = 127; write(media, bytes); f = std::fopen(media.c_str(), "rb");
    CHECK(Mp3SeekIndex::probe_duration(f, static_cast<std::uint32_t>(bytes.size())) == 0); std::fclose(f);
    bytes = {'I','D','3',4,0,0,128,0,0,0}; write(media, bytes); CHECK(!index->begin(media.c_str()));
    bytes = frames(1000); write(media, bytes);
    for (unsigned i = 0; i < 1000; ++i) { CHECK(index->begin(media.c_str())); index->step(1); index->cancel(); CHECK(index->status() == SeekIndexStatus::kIdle); }
    fs::remove_all(root / ".nightwave");
    fs::create_directory_symlink(fs::temp_directory_path(), root / ".nightwave");
    CHECK(index->begin(media.c_str())); scan(*index); CHECK(!index->save(root.c_str()));
    fs::remove(root / ".nightwave"); fs::create_directory(root / ".nightwave");
    const auto original = fs::path(argv[1]);
    CHECK(index->begin(original.c_str())); scan(*index); CHECK(index->ready());
    CHECK(index->duration_ms() >= 30000 && index->duration_ms() < 30200);
    std::cout << "Seek index checks: bounded scanning, duration, preroll, malformed/stale/corrupt caches, capacity, cancellation\n";
    // Only known test-owned files/directories, never an arbitrary supplied root.
    fs::remove_all(root);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
