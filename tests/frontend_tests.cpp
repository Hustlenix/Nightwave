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
std::string selected;
nightwave::TextFrame displayed;
std::deque<nightwave::ButtonEvent> events;
#define CHECK(value) do { if (!(value)) { ++failures; std::cerr << __LINE__ << ": " << #value << '\n'; } } while(false)
bool shows(const char* value) {
    for (const auto& line : displayed.rows) if (std::strstr(line.data(), value)) return true;
    return false;
}
}
esp_err_t gpio_config(const gpio_config_t*) { return ESP_OK; }
int gpio_get_level(gpio_num_t) { return headphone ? 0 : 1; }
std::int64_t esp_timer_get_time() { return 0; }
namespace nightwave {
bool SdStorage::mount() { mounted_ = card; return card; }
bool ButtonMonitor::poll(ButtonEvent& event) {
    if (events.empty()) return false;
    event = events.front(); events.pop_front(); return true;
}
bool initialize_settings() { return true; }
bool load_settings(Settings& value) { value.volume_percent = saved_volume; return true; }
bool save_settings(const Settings& value) { saved_volume = value.volume_percent; ++saves; return true; }
bool OledDisplay::initialize(OledController controller) { controller_ = controller; return true; }
bool OledDisplay::show(const TextFrame& value) { displayed = value; return true; }
bool OledDisplay::sleep(bool) { return true; }
bool StreamingPlayer::start(const char* path, I2sAudioSink&, OutputPath output, std::uint8_t volume) {
    selected = path; set_volume(volume); output_.store(output); paused_.store(false);
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
    fs::create_directories(root_path / "sub");
    for (const auto* name : {"01.wav", "02.mp3", "z-corrupt.mp3", "ignored.txt"}) {
        std::ofstream file(root_path / name); file << "simulated media";
    }
    std::ofstream(root_path / "sub" / "nested.wav") << "simulated media";
    const auto root = root_path.string();
    SdStorage sd; I2sAudioSink sink; ButtonMonitor buttons;
    auto player = std::make_unique<StreamingPlayer>();
    PlayerFrontend ui(sd, *player, sink, buttons, root.c_str());
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
    card = false;
    PlayerFrontend no_sd(sd, *player, sink, buttons, root.c_str()); no_sd.initialize();
    now = 200; no_sd.tick(now); CHECK(shows("NO SD"));
    card = true; now += 250;
    events.push_back({ButtonId::kPlayPause, ButtonGesture::kPress, now}); no_sd.tick(now);
    CHECK(shows("PLAY: OPEN/PLAY"));
    if (failures) return EXIT_FAILURE;
    std::cout << "Frontend tests passed: real folder scan, five buttons, corrupt recovery, route indicator, settings debounce, diagnostics, sleep/wake, no-SD retry\n";
}
