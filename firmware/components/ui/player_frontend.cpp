#include "nightwave/player_frontend.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "nightwave/hardware_config.h"
namespace nightwave {
namespace {
bool media(const char* name) {
    const char* dot = std::strrchr(name, '.');
    return dot && (!strcasecmp(dot, ".wav") || !strcasecmp(dot, ".mp3"));
}
}
void PlayerFrontend::initialize() {
    nvs_ready_ = initialize_settings();
    if (nvs_ready_) load_settings(settings_);
    if (!nvs_ready_) ESP_LOGW("ui", "NVS unavailable; settings are volatile, partition not erased");
    oled_ready_ = oled_.initialize();
    if (!oled_ready_) ESP_LOGW("ui", "OLED unavailable (provisional SSD1306 128x64 @0x3c)");
    TextFrame boot; boot.line(2, "NIGHTWAVE"); boot.line(4, "STARTING PLAYER");
    if (oled_ready_) oled_.show(boot);
    gpio_config_t detect{};
    detect.pin_bit_mask = 1ULL << hardware::kHeadphoneDetect;
    detect.mode = GPIO_MODE_INPUT; detect.pull_up_en = GPIO_PULLUP_ENABLE;
    if (gpio_config(&detect) != ESP_OK) ESP_LOGW("ui", "headphone detect unavailable");
    nav_.last_input_ms = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);
    nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd;
}
bool PlayerFrontend::scan() {
    nav_.populate(0); truncated_ = false;
    if (!sd_.mount()) return false;
    auto* directory = opendir(folder_.data());
    if (!directory) return false;
    if (std::strcmp(folder_.data(), "/sdcard")) {
        std::strcpy(entries_[0].name.data(), ".."); entries_[0].directory = true; nav_.count = 1;
    }
    while (const auto* item = readdir(directory)) {
        if (item->d_name[0] == '.') continue;
        std::array<char, 512> path{};
        const int size = std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), item->d_name);
        if (size <= 0 || static_cast<std::size_t>(size) >= 256 || std::strlen(item->d_name) >= 256) continue;
        struct stat info{};
        if (stat(path.data(), &info) != 0) continue;
        const bool is_directory = S_ISDIR(info.st_mode);
        if (!is_directory && (!S_ISREG(info.st_mode) || !media(item->d_name))) continue;
        if (nav_.count == entries_.size()) { truncated_ = true; continue; }
        auto& entry = entries_[nav_.count++];
        std::strcpy(entry.name.data(), item->d_name); entry.directory = is_directory;
    }
    closedir(directory);
    std::sort(entries_.begin(), entries_.begin() + nav_.count, [](const Entry& a, const Entry& b) {
        if (a.directory != b.directory) return a.directory;
        return strcasecmp(a.name.data(), b.name.data()) < 0;
    });
    return true;
}
void PlayerFrontend::track_started(const char* path) {
    const auto* slash = path ? std::strrchr(path, '/') : nullptr;
    std::snprintf(title_.data(), title_.size(), "%s", slash ? slash + 1 : (path ? path : ""));
    nav_.screen = PlayerScreen::kNowPlaying; was_running_ = true;
}
void PlayerFrontend::volume(std::uint8_t value, std::uint32_t now) {
    settings_.volume_percent = std::min<std::uint8_t>(100, value);
    player_.set_volume(settings_.volume_percent); dirty_ = true; changed_at_ = now;
}
bool PlayerFrontend::play(std::size_t index) {
    if (index >= nav_.count || entries_[index].directory) return false;
    if (!player_.stop()) { nav_.screen = PlayerScreen::kCorrupt; return false; }
    std::array<char, 512> path{};
    const int size = std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), entries_[index].name.data());
    if (size <= 0 || static_cast<std::size_t>(size) >= 256 ||
        !player_.start(path.data(), audio_, headphone_ ? OutputPath::kLine : OutputPath::kSpeaker,
                       settings_.volume_percent)) {
        nav_.screen = PlayerScreen::kCorrupt; return false;
    }
    playing_index_ = index; track_started(path.data()); return true;
}
void PlayerFrontend::next(int direction) {
    if (!nav_.count) return;
    std::size_t index = playing_index_ < nav_.count ? playing_index_ : 0;
    for (std::size_t i = 0; i < nav_.count; ++i) {
        index = direction > 0 ? (index + 1) % nav_.count : (index + nav_.count - 1) % nav_.count;
        if (!entries_[index].directory) { play(index); return; }
    }
}
void PlayerFrontend::parent() {
    if (std::strcmp(folder_.data(), "/sdcard") == 0) return;
    auto* slash = std::strrchr(folder_.data(), '/');
    if (slash && slash - folder_.data() >= 7) *slash = 0;
    nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd;
}
void PlayerFrontend::event(const ButtonEvent& e) {
    if (!nav_.interact(e.monotonic_ms)) return;
    if (e.gesture == ButtonGesture::kLongPress) {
        if (e.button == ButtonId::kPlayPause) {
            nav_.screen = nav_.screen == PlayerScreen::kBrowser && player_.playing()
                ? PlayerScreen::kNowPlaying : PlayerScreen::kBrowser;
            if (nav_.screen == PlayerScreen::kBrowser && !scan()) nav_.screen = PlayerScreen::kNoSd;
        } else if (e.button == ButtonId::kPrevious) parent();
        else if (e.button == ButtonId::kVolumeUp) nav_.screen = PlayerScreen::kDiagnostics;
        return;
    }
    if (e.button == ButtonId::kVolumeUp || e.button == ButtonId::kVolumeDown) {
        const int value = settings_.volume_percent + (e.button == ButtonId::kVolumeUp ? 2 : -2);
        volume(static_cast<std::uint8_t>(std::clamp(value, 0, 100)), e.monotonic_ms); return;
    }
    if (nav_.screen == PlayerScreen::kNoSd) {
        if (e.button == ButtonId::kPlayPause) nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd;
        return;
    }
    if (nav_.screen == PlayerScreen::kBrowser) {
        if (e.button == ButtonId::kPrevious) nav_.move(-1);
        else if (e.button == ButtonId::kNext) nav_.move(1);
        else if (e.button == ButtonId::kPlayPause && nav_.count) {
            const auto& entry = entries_[nav_.cursor];
            if (!entry.directory) play(nav_.cursor);
            else if (!std::strcmp(entry.name.data(), "..")) parent();
            else {
                std::array<char, 512> path{};
                const int size = std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), entry.name.data());
                if (size > 0 && static_cast<std::size_t>(size) < folder_.size()) {
                    std::strcpy(folder_.data(), path.data());
                    nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd;
                }
            }
        }
    } else if (e.button == ButtonId::kPrevious) next(-1);
    else if (e.button == ButtonId::kNext) next(1);
    else if (e.button == ButtonId::kPlayPause) {
        if (player_.playing()) { player_.pause(!player_.paused()); nav_.screen = PlayerScreen::kNowPlaying; }
        else { nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd; }
    }
}
TextFrame PlayerFrontend::frame() const {
    TextFrame text; std::array<char, 64> line{};
    text.line(0, "NIGHTWAVE");
    std::snprintf(line.data(), line.size(), "VOL %u  %s  BAT ?", settings_.volume_percent, headphone_ ? "HP" : "SPK");
    text.line(1, line.data());
    switch (nav_.screen) {
        case PlayerScreen::kBoot: text.line(3, "STARTING"); break;
        case PlayerScreen::kNoSd: text.line(3, "NO SD / MOUNT ERROR"); text.line(5, "PLAY: RETRY SD"); break;
        case PlayerScreen::kCorrupt: text.line(3, "FILE / OUTPUT ERROR"); text.line(4, title_.data()); text.line(6, "NEXT OR HOLD PLAY"); break;
        case PlayerScreen::kNowPlaying:
            text.line(3, title_.data());
            text.line(5, !player_.playing() ? "STOPPED / END" : (player_.paused() ? "PAUSED" : "PLAYING"));
            text.line(7, "HOLD PLAY: BROWSER"); break;
        case PlayerScreen::kBrowser: {
            text.line(2, folder_.data());
            if (!nav_.count) text.line(4, "NO WAV/MP3 FILES");
            const auto start = nav_.cursor / 4 * 4;
            for (std::size_t i = 0; i < 4 && start + i < nav_.count; ++i) {
                const auto& entry = entries_[start + i];
                std::snprintf(line.data(), line.size(), "%c%c %.18s", start + i == nav_.cursor ? '>' : ' ',
                    entry.directory ? '/' : ' ', entry.name.data());
                text.line(3 + i, line.data());
            }
            text.line(7, truncated_ ? "128 ITEM LIMIT" : "PLAY: OPEN/PLAY"); break;
        }
        case PlayerScreen::kDiagnostics: {
            const auto t = player_.telemetry();
            std::snprintf(line.data(), line.size(), "PCM %luMS ENC %lu", static_cast<unsigned long>(t.pcm_ms), static_cast<unsigned long>(t.compressed_bytes)); text.line(2, line.data());
            std::snprintf(line.data(), line.size(), "UNDER %lu ERR %lu", static_cast<unsigned long>(t.underruns), static_cast<unsigned long>(t.errors)); text.line(3, line.data());
            std::snprintf(line.data(), line.size(), "SD %luUS DEC %luUS", static_cast<unsigned long>(t.sd_worst_us), static_cast<unsigned long>(t.decoder_worst_us)); text.line(4, line.data());
            std::snprintf(line.data(), line.size(), "HEAP %u PSRAM %u", static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)), static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM))); text.line(5, line.data());
            text.line(6, "BATTERY UNMEASURED"); text.line(7, "HOLD PLAY: BROWSER"); break;
        }
    }
    return text;
}
void PlayerFrontend::tick(std::uint32_t now) {
    const bool detected = gpio_get_level(static_cast<gpio_num_t>(hardware::kHeadphoneDetect)) == 0;
    if (detected != detect_candidate_) { detect_candidate_ = detected; route_changed_ = now; }
    if (now - route_changed_ >= 50 && headphone_ != detected) {
        headphone_ = detected; player_.set_output(headphone_ ? OutputPath::kLine : OutputPath::kSpeaker);
    }
    ButtonEvent e;
    while (input_.poll(e)) event(e);
    if (was_running_ && !player_.playing() && player_.errors()) nav_.screen = PlayerScreen::kCorrupt;
    was_running_ = player_.playing();
    if (dirty_ && now - changed_at_ >= 2000) {
        if (nvs_ready_ && !save_settings(settings_)) ESP_LOGW("ui", "settings save failed");
        dirty_ = false;
    }
    nav_.tick(now);
    if (oled_ready_ && display_asleep_ != nav_.asleep) {
        if (!oled_.sleep(nav_.asleep)) oled_ready_ = false;
        display_asleep_ = nav_.asleep;
    }
    if (oled_ready_ && !nav_.asleep && now - last_render_ >= 200) {
        last_render_ = now;
        if (!oled_.show(frame())) { oled_ready_ = false; ESP_LOGW("ui", "OLED write failed; controls remain active"); }
    }
}
}  // namespace nightwave
