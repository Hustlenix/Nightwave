#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include "driver/gpio.h"
#include "nightwave/player_frontend.h"
namespace {
int failures = 0;
bool card = true, headphone = false;
std::uint8_t saved_volume = 8;
unsigned saves = 0;
nightwave::Settings persisted{};
bool restore_full_settings = false;
std::string selected;
nightwave::TextFrame displayed;
std::deque<nightwave::ButtonEvent> events;
#define CHECK(value) do { if (!(value)) { ++failures; std::cerr << __LINE__ << ": " << #value << '\n'; } } while(false)
bool shows(const char* value) {
    for (const auto& line : displayed.rows) if (std::strstr(line.data(), value)) return true;
    return false;
}
struct FakeDisplay final : nightwave::DisplaySink {
    std::string lyric, title; bool asleep{false}, fail{false};
    bool begin() override { return true; }
    nightwave::DisplayCapabilities capabilities() const override { return {240, 280, true, false, true}; }
    bool present(const nightwave::DisplayFrame& frame) override {
        displayed = frame.fallback; lyric = frame.lyric_current; title = frame.title; return !fail;
    }
    bool sleep(bool value) override { asleep = value; return !fail; }
};
}
esp_err_t gpio_config(const gpio_config_t*) { return ESP_OK; }
int gpio_get_level(gpio_num_t) { return headphone ? 0 : 1; }
std::int64_t esp_timer_get_time() { return 0; }
namespace nightwave {
// This suite fakes StreamingPlayer completely; its decoder is never allocated.
// Real Helix/streaming behavior is exercised by the separate decoder suites.
Mp3Decoder::~Mp3Decoder() = default;
DecodeResult Mp3Decoder::decode(EncodedBytes, bool) { return {}; }
void Mp3Decoder::reset() {}
bool SdStorage::mount() { mounted_ = card; return card; }
bool ButtonMonitor::poll(ButtonEvent& event) {
    if (events.empty()) return false;
    event = events.front(); events.pop_front(); return true;
}
bool initialize_settings() { return true; }
bool load_settings(Settings& value) { if (restore_full_settings) value = persisted; else value.volume_percent = saved_volume; return true; }
bool save_settings(const Settings& value) { persisted = value; saved_volume = value.volume_percent; ++saves; return true; }
bool OledDisplay::initialize(OledController controller) { controller_ = controller; return true; }
bool OledDisplay::show(const TextFrame& value) { displayed = value; return true; }
bool OledDisplay::sleep(bool) { return true; }
bool StreamingPlayer::start(const char* path, I2sAudioSink&, OutputPath output, std::uint8_t volume, std::uint32_t seek_ms, const char*) {
    selected = path; set_volume(volume); output_.store(output); paused_.store(false);
    position_ms_.store(seek_ms);
    const bool ok = !std::strstr(path, "corrupt");
    errors_.store(ok ? 0 : 1); running_.store(ok); return ok;
}
bool StreamingPlayer::stop() { running_.store(false); return true; }
StreamTelemetry StreamingPlayer::telemetry() const { return {}; }
AudioSinkStatus I2sAudioSink::configure(const AudioFormat&) { return AudioSinkStatus::kAccepted; }
AudioSinkStatus I2sAudioSink::write(const PcmBlock&) { return AudioSinkStatus::kAccepted; }
void I2sAudioSink::stop() {}
}
int main(int argc, char** argv) {
    using namespace nightwave;
    namespace fs = std::filesystem;
    if (argc != 2) return EXIT_FAILURE;
    const auto root_path = fs::path(argv[1]) / "frontend-fixture";
    fs::remove(root_path / "a.m3u"); // Remove only this suite's prior generated playlist.
    fs::create_directories(root_path / "sub");
    for (const auto* name : {"01.wav", "02.mp3", "z-corrupt.mp3", "ignored.txt"}) {
        std::ofstream file(root_path / name); file << "simulated media";
    }
    std::ofstream(root_path / "sub" / "nested.wav") << "simulated media";
    std::ofstream(root_path / "01.lrc") << "[00:00]original lyric test with more than twenty two characters\n[00:01]second test line\n";
    const auto root = fs::weakly_canonical(root_path).string(); // SD roots are canonical; CTest uses tests/../ paths.
    SdStorage sd; I2sAudioSink sink; ButtonMonitor buttons;
    auto player = std::make_unique<StreamingPlayer>();
    FakeDisplay display;
    PlayerFrontend ui(sd, *player, sink, buttons, root.c_str(), &display);
    ui.initialize(); CHECK(shows("STARTING"));
    std::uint32_t now = 200;
    ui.tick(now); CHECK(shows("PLAY: OPEN/PLAY")); CHECK(shows("/ sub"));
    const auto press = [&](ButtonId button, ButtonGesture gesture = ButtonGesture::kPress) {
        now += 250; events.push_back({button, gesture, now}); ui.tick(now);
    };
    press(ButtonId::kPlayPause); CHECK(shows("nested.wav")); // Enter directory.
    press(ButtonId::kPrevious, ButtonGesture::kLongPress); CHECK(shows("/ sub"));
    press(ButtonId::kNext); press(ButtonId::kPlayPause);
    CHECK(selected.find("01.wav") != std::string::npos && player->playing()); CHECK(shows("PLAYING"));
    press(ButtonId::kNext, ButtonGesture::kLongPress); CHECK(shows("original lyric test"));
    CHECK(display.lyric == "original lyric test with more than twenty two characters");
    press(ButtonId::kPlayPause); CHECK(player->paused() && shows("PAUSED"));
    press(ButtonId::kPlayPause); CHECK(!player->paused());
    press(ButtonId::kNext); CHECK(selected.find("02.mp3") != std::string::npos);
    press(ButtonId::kPrevious); CHECK(selected.find("01.wav") != std::string::npos);
    press(ButtonId::kVolumeUp); CHECK(shows("VOL 10"));
    now += 2100; ui.tick(now); CHECK(saves == 1 && saved_volume == 10);
    headphone = true; now += 250; ui.tick(now); now += 250; ui.tick(now); CHECK(shows("HP"));
    press(ButtonId::kVolumeUp, ButtonGesture::kLongPress); CHECK(shows("BATTERY UNMEASURED"));
    press(ButtonId::kPlayPause, ButtonGesture::kLongPress); CHECK(shows("PLAY: OPEN/PLAY"));
    press(ButtonId::kPlayPause, ButtonGesture::kLongPress); CHECK(shows("PLAYING"));
    press(ButtonId::kNext); press(ButtonId::kNext); CHECK(shows("FILE / OUTPUT ERROR")); CHECK(shows("z-corrupt.mp3"));
    press(ButtonId::kNext); CHECK(player->playing() && shows("PLAYING"));
    now += 31000; ui.tick(now);
    const auto before_wake = selected;
    press(ButtonId::kNext); CHECK(selected == before_wake); // Wake only.
    press(ButtonId::kNext); CHECK(selected != before_wake);
    press(ButtonId::kVolumeDown, ButtonGesture::kLongPress); CHECK(shows("BT BACKEND NOT READY"));
    press(ButtonId::kPlayPause); CHECK(shows("SHUFFLE"));
    press(ButtonId::kNext); press(ButtonId::kPlayPause); CHECK(shows("15 MIN"));
    now += 900000; ui.tick(now); now += 250; ui.tick(now); CHECK(!player->playing());
    // Playlist browser path and real .m3u parser integration (created after
    // old browser-order assertions, so those assertions remain deterministic).
    std::ofstream(root_path / "a.m3u") << "02.mp3\n01.wav\n";
    PlayerFrontend playlist_ui(sd, *player, sink, buttons, root.c_str()); playlist_ui.initialize();
    now = 200; playlist_ui.tick(now); now += 250;
    events.push_back({ButtonId::kNext, ButtonGesture::kPress, now}); playlist_ui.tick(now); // 01.wav
    now += 250; events.push_back({ButtonId::kNext, ButtonGesture::kPress, now}); playlist_ui.tick(now); // 02.mp3
    now += 250; events.push_back({ButtonId::kNext, ButtonGesture::kPress, now}); playlist_ui.tick(now); // a.m3u
    now += 250; events.push_back({ButtonId::kPlayPause, ButtonGesture::kPress, now}); playlist_ui.tick(now);
    CHECK(selected.find("02.mp3") != std::string::npos && player->playing());
    CHECK(!playlist_ui.output_preference(OutputPreference::kBluetooth, now));
    CHECK(playlist_ui.output_preference(OutputPreference::kWired, now));
    CHECK(playlist_ui.resume_saved() && selected.find("02.mp3") != std::string::npos);
    CHECK(playlist_ui.queue_size() == 2);
    now += 250; events.push_back({ButtonId::kNext, ButtonGesture::kPress, now}); playlist_ui.tick(now);
    CHECK(selected.find("01.wav") != std::string::npos);
    card = false;
    PlayerFrontend no_sd(sd, *player, sink, buttons, root.c_str()); no_sd.initialize();
    now = 200; no_sd.tick(now); CHECK(shows("NO SD"));
    card = true; now += 250;
    events.push_back({ButtonId::kPlayPause, ButtonGesture::kPress, now}); no_sd.tick(now);
    CHECK(shows("PLAY: OPEN/PLAY"));
    // Real catalog + UI integration, beyond the old 128-entry browser limit.
    const auto indexed_path = fs::path(argv[1]) / "frontend-indexed-fixture";
    fs::create_directories(indexed_path);
    for (unsigned i = 0; i < 260; ++i) {
        char name[32]; std::snprintf(name, sizeof(name), "original-%03u.mp3", i);
        std::ofstream(indexed_path / name) << "original fake decoder fixture";
    }
    const auto indexed_root = fs::weakly_canonical(indexed_path).generic_string();
    player->stop();
    PlayerFrontend indexed_ui(sd, *player, sink, buttons, indexed_root.c_str()); indexed_ui.initialize();
    now = 0;
    const auto idle_ticks = [&](unsigned count) { for (unsigned i = 0; i < count; ++i) { now += 25; indexed_ui.tick(now); } };
    const auto catalog_press = [&](ButtonId button, ButtonGesture gesture = ButtonGesture::kPress) {
        now += 250; events.push_back({button, gesture, now}); indexed_ui.tick(now);
    };
    idle_ticks(400); CHECK(shows("LIBRARY LIMIT"));
    CHECK(indexed_ui.catalog_fast_lookup());
    catalog_press(ButtonId::kVolumeDown, ButtonGesture::kLongPress);
    for (unsigned i = 0; i < 4; ++i) catalog_press(ButtonId::kNext);
    CHECK(shows(">LIBRARY"));
    catalog_press(ButtonId::kNext); CHECK(shows(">REBUILD INDEX"));
    catalog_press(ButtonId::kPlayPause); CHECK(shows(">SONGS"));
    catalog_press(ButtonId::kPlayPause); idle_ticks(12);
    CHECK(shows("HOLD PREV: UP"));
    // Browse a second page then come back; these presses do not play tracks.
    for (unsigned i = 0; i < 16; ++i) catalog_press(ButtonId::kNext);
    idle_ticks(12); catalog_press(ButtonId::kPrevious); idle_ticks(12);
    catalog_press(ButtonId::kPlayPause);
    CHECK(player->playing() && indexed_ui.queue_size() == 260);
    const auto paused_scan = indexed_ui.catalog_scanned(); idle_ticks(80);
    CHECK(indexed_ui.catalog_building() && indexed_ui.catalog_scanned() == paused_scan);
    const auto indexed_first = selected;
    for (unsigned i = 0; i < 20; ++i) catalog_press(ButtonId::kNext);
    CHECK(selected != indexed_first && indexed_ui.queue_size() == 260);
    CHECK(indexed_ui.seek_current(1234)); idle_ticks(12); CHECK(player->position_ms() == 1234);
    indexed_ui.mode(PlaybackMode::kRepeatTrack, now);
    const auto repeating = selected; player->stop(); idle_ticks(12);
    CHECK(player->playing() && selected == repeating);
    // Re-enter the library while playing: new index writes remain paused, but
    // bounded reads of the sealed generation still permit group navigation.
    catalog_press(ButtonId::kVolumeDown, ButtonGesture::kLongPress);
    catalog_press(ButtonId::kPrevious); // Move back from rebuild to LIBRARY.
    catalog_press(ButtonId::kPlayPause); // Retained LIBRARY settings row.
    CHECK(shows("INDEX PAUSED"));
    catalog_press(ButtonId::kNext); catalog_press(ButtonId::kPlayPause); idle_ticks(80);
    CHECK(shows("UNKNOWN"));
    catalog_press(ButtonId::kPlayPause); idle_ticks(80); catalog_press(ButtonId::kPlayPause);
    CHECK(player->playing() && indexed_ui.queue_size() == 260);
    const auto filtered_first = selected; catalog_press(ButtonId::kNext); idle_ticks(80);
    CHECK(selected != filtered_first && player->playing());
    CHECK(indexed_ui.resume_saved()); idle_ticks(80);
    CHECK(indexed_ui.queue_size() == 260 && player->playing());
    const auto resumed = selected; catalog_press(ButtonId::kNext); idle_ticks(80);
    CHECK(selected != resumed && indexed_ui.queue_size() == 260);
    idle_ticks(100); // Flush the actual new catalog context through the fake NVS.
    CHECK(!std::strncmp(persisted.library_folder.data(), "@catalog2:1:", 12));
    CHECK(!std::strcmp(persisted.resume_path.data(), selected.c_str()));
    const auto checkpoint = persisted;
    const auto checkpoint_path = selected;
    restore_full_settings = true;
    player->stop();
    PlayerFrontend cold(sd, *player, sink, buttons, indexed_root.c_str()); cold.initialize();
    CHECK(!player->playing()); // Loading settings/cache is never boot autoplay.
    cold.tick(200); CHECK(cold.catalog_fast_lookup());
    CHECK(cold.resume_saved() && selected == checkpoint_path);
    cold.tick(225); CHECK(cold.queue_size() == 260 && player->playing());
    events.push_back({ButtonId::kNext, ButtonGesture::kPress, 450}); cold.tick(450);
    cold.tick(475); CHECK(selected != checkpoint_path && cold.queue_size() == 260);
    // Legacy saved context still restores by checked path, without an ordinal.
    persisted = checkpoint;
    std::snprintf(persisted.library_folder.data(), persisted.library_folder.size(), "@catalog:1:(UNKNOWN)");
    player->stop();
    PlayerFrontend legacy_resume(sd, *player, sink, buttons, indexed_root.c_str()); legacy_resume.initialize();
    CHECK(!player->playing()); legacy_resume.tick(200);
    CHECK(legacy_resume.resume_saved());
    for (unsigned i = 0; i < 100; ++i) legacy_resume.tick(225 + i * 25);
    CHECK(legacy_resume.queue_size() == 260 && selected == checkpoint_path);
    // Reject malformed/oversized hints safely; the canonical saved track can
    // still play explicitly as a one-track queue, not an unchecked catalog jump.
    for (const auto* context : {"@catalog2:1:10000:(UNKNOWN)", "@catalog2:1:-1:(UNKNOWN)", "@catalog2:3:1:(UNKNOWN)", "@catalog2:1:1"}) {
        persisted = checkpoint;
        std::snprintf(persisted.library_folder.data(), persisted.library_folder.size(), "%s", context);
        player->stop();
        PlayerFrontend invalid_resume(sd, *player, sink, buttons, indexed_root.c_str()); invalid_resume.initialize();
        CHECK(!player->playing()); invalid_resume.tick(200);
        CHECK(invalid_resume.resume_saved()); invalid_resume.tick(225);
        CHECK(invalid_resume.queue_size() == 1 && selected == checkpoint_path);
    }
    catalog_press(ButtonId::kPrevious, ButtonGesture::kLongPress); // Parent from now-playing returns folder browser.
    if (failures) return EXIT_FAILURE;
    std::cout << "Frontend tests passed: folder/M3U controls, recovery, settings, sleep, 260-track fast catalog paging/queue/seek/repeat/group playback, cold/legacy/malformed resume without boot autoplay\n";
}
