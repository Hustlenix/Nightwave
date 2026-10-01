#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "nightwave/media_documents.h"
#include "nightwave/playback_policy.h"
namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { ++failures; std::cerr << __LINE__ << ": " << #x << '\n'; } } while(false)
nightwave::DocumentStatus parse(nightwave::Lyrics& lyrics, const std::string& source) {
    auto* file = std::tmpfile(); if (!file) std::abort();
    std::fwrite(source.data(), 1, source.size(), file); std::rewind(file);
    const auto result = lyrics.parse(file); std::fclose(file); return result;
}
void write(const std::filesystem::path& path, const std::string& data) { std::ofstream file(path, std::ios::binary); file.write(data.data(), static_cast<std::streamsize>(data.size())); }
std::string tag(unsigned version, const std::string& id, const std::string& text) {
    const unsigned n = static_cast<unsigned>(text.size());
    std::string frame = id;
    for (int shift : {24,16,8,0}) frame += char(n >> shift);
    frame += std::string(2, '\0'); frame += text;
    const unsigned size = static_cast<unsigned>(frame.size());
    std::string result = "ID3"; result += char(version); result += std::string(2, '\0');
    for (int shift : {21,14,7,0}) result += char((size >> shift) & 127);
    return result + frame;
}
}
int main(int argc, char** argv) {
    using namespace nightwave; namespace fs = std::filesystem;
    if (argc != 2) return EXIT_FAILURE;
    auto lyrics = std::make_unique<Lyrics>();
    CHECK(parse(*lyrics, "\xef\xbb\xbf[ar:Nightwave test]\r\n[00:03.250]three\n[00:01][00:02.5]one\n[offset:-500]\n") == DocumentStatus::kOk);
    CHECK(lyrics->size() == 3 && !lyrics->current(499));
    CHECK(lyrics->current(500)->time_ms == 500 && !std::strcmp(lyrics->current(500)->text.data(), "one"));
    CHECK(lyrics->next(500)->time_ms == 2000);
    CHECK(lyrics->current(5000)->time_ms == 2750 && !lyrics->next(5000));
    CHECK(parse(*lyrics, "[00:01]first\n[00:01]last\n") == DocumentStatus::kOk);
    CHECK(!std::strcmp(lyrics->current(1000)->text.data(), "last"));
    CHECK(parse(*lyrics, "[00:01]caf\xc3\xa9\n") == DocumentStatus::kOk);
    CHECK(!std::strcmp(lyrics->current(1000)->text.data(), "caf\xc3\xa9"));
    for (const auto* malformed : {"[", "[0", "[00:]x", "[00:60]x", "[00:01.1234]x", "[offset:-999999]", "[00:01x", "[00:01]"}) {
        const auto result = parse(*lyrics, malformed);
        CHECK(result == DocumentStatus::kMalformed || !std::strcmp(malformed, "[00:01]"));
    }
    CHECK(parse(*lyrics, std::string(1000, 'x')) == DocumentStatus::kLimit && !lyrics->size());
    CHECK(parse(*lyrics, std::string("[00:01]bad") + std::string(1, '\0') + "text") == DocumentStatus::kMalformed);
    std::string many; for (unsigned i = 0; i < 257; ++i) many += "[00:01]synthetic line\n";
    CHECK(parse(*lyrics, many) == DocumentStatus::kLimit && !lyrics->size());
    // Repeated lifetime/parser operations plus deterministic malformed fuzz.
    std::uint32_t random = 12;
    for (unsigned cycle = 0; cycle < 1000; ++cycle) {
        CHECK(parse(*lyrics, "[00:00]original test words\n[00:02]next test line\n") == DocumentStatus::kOk);
        CHECK(lyrics->current(0) && lyrics->current(2000) && lyrics->next(0));
        std::string fuzz;
        for (unsigned i = 0; i < 128; ++i) { random = random * 1664525U + 1013904223U; fuzz += char(random >> 24); }
        parse(*lyrics, fuzz); CHECK(lyrics->size() <= Lyrics::kCapacity);
    }
    std::array<char, 256> joined{};
    CHECK(local_media_path("/sdcard/lists", "../music/song.mp3", "/sdcard", joined.data(), joined.size()));
    CHECK(!std::strcmp(joined.data(), "/sdcard/music/song.mp3"));
    for (const auto* bad : {"../../escape.wav", "http://host/a.mp3", "/etc/a.wav", "C:\\a.mp3", "../x.txt"})
        CHECK(!local_media_path("/sdcard/lists", bad, "/sdcard", joined.data(), joined.size()));
    const auto fixture = fs::path(argv[1]) / "media-documents"; fs::create_directories(fixture / "lists");
    const auto root = fixture.generic_string(); const auto list_path = (fixture / "lists" / "test.m3u").generic_string();
    auto list = std::make_unique<Playlist>();
    write(list_path, "\xef\xbb\xbf#EXTM3U\n../one.wav\n../two.mp3\n");
    CHECK(list->load(list_path.c_str(), root.c_str()) == DocumentStatus::kOk && list->size() == 2);
    write(list_path, "../../../outside.mp3\n"); CHECK(list->load(list_path.c_str(), root.c_str()) == DocumentStatus::kMalformed && !list->size());
    TrackMetadata metadata;
    const auto mp3 = (fixture / "synthetic.mp3").generic_string();
    write(mp3, tag(3, "TIT2", std::string(1, '\3') + "Original test title"));
    CHECK(read_metadata(mp3.c_str(), metadata) == DocumentStatus::kOk && !std::strcmp(metadata.title.data(), "Original test title"));
    write(mp3, tag(4, "TPE1", std::string(1, '\3') + "Test artist"));
    CHECK(read_metadata(mp3.c_str(), metadata) == DocumentStatus::kOk && !std::strcmp(metadata.artist.data(), "Test artist"));
    const char utf16[] = {1, char(0xff), char(0xfe), 'A', 0, char(0xe9), 0};
    write(mp3, tag(3, "TIT2", std::string(utf16, sizeof(utf16))));
    CHECK(read_metadata(mp3.c_str(), metadata) == DocumentStatus::kOk && !std::strcmp(metadata.title.data(), "A\xc3\xa9"));
    write(mp3, std::string("ID3\3\0\0\x7f\x7f\x7f\x7f", 10)); CHECK(read_metadata(mp3.c_str(), metadata) != DocumentStatus::kOk);
    const auto wav = (fs::path(argv[1]) / "02_left_right_stereo_44100.wav").generic_string();
    CHECK(read_metadata(wav.c_str(), metadata) == DocumentStatus::kOk && metadata.duration_ms > 0);
    PlaybackPolicy policy;
    CHECK(policy.next(2, 3, true) == 3 && policy.next(2, 3, false) == 0);
    policy.mode = PlaybackMode::kRepeatAll; CHECK(policy.next(2, 3, true) == 0);
    policy.mode = PlaybackMode::kRepeatTrack; CHECK(policy.next(2, 3, true) == 2);
    policy.mode = PlaybackMode::kShuffle;
    std::size_t current = 0;
    for (unsigned i = 0; i < 1000; ++i) { const auto next = policy.next(current, 4, true); CHECK(next < 4 && next != current); current = next; }
    SleepTimer timer; CHECK(!timer.expired(99999999, true));
    timer.set(SleepMode::k15, UINT32_MAX - 100); CHECK(!timer.expired(899898, false)); CHECK(timer.expired(899899, false));
    timer.set(SleepMode::kEndTrack, 0); CHECK(!timer.expired(900000, false) && timer.expired(900000, true));
    if (failures) return EXIT_FAILURE;
    std::cout << "Media tests passed: bounded LRC/ID3/WAV/M3U parsing, 1000 parser/shuffle cycles, sleep rollover\n";
}
