#include "nightwave/player_frontend.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <memory>
#include <new>
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
bool playlist(const char* name) {
    const auto* dot = std::strrchr(name, '.');
    return dot && (!strcasecmp(dot, ".m3u") || !strcasecmp(dot, ".m3u8"));
}
}
void PlayerFrontend::initialize() {
    nvs_ready_ = initialize_settings();
    if (nvs_ready_) load_settings(settings_);
    policy_.mode = settings_.shuffle ? PlaybackMode::kShuffle : settings_.repeat_mode == RepeatMode::kTrack ?
        PlaybackMode::kRepeatTrack : settings_.repeat_mode == RepeatMode::kAll ? PlaybackMode::kRepeatAll : PlaybackMode::kNormal;
    if (!nvs_ready_) ESP_LOGW("ui", "NVS unavailable; settings are volatile, partition not erased");
    oled_ready_ = oled_.initialize();
    if (!oled_ready_) ESP_LOGW("ui", "OLED unavailable (provisional SH1106 128x64 @0x3c)");
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
    if (std::strcmp(folder_.data(), root_)) {
        std::strcpy(entries_[0].name.data(), ".."); entries_[0].directory = true; nav_.count = 1;
    }
    unsigned examined = 0;
    while (const auto* item = readdir(directory)) {
        if (++examined > 512) { truncated_ = true; break; }
        if (item->d_name[0] == '.') continue;
        std::array<char, 512> path{};
        const int size = std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), item->d_name);
        if (size <= 0 || static_cast<std::size_t>(size) >= 256 || std::strlen(item->d_name) >= 256) continue;
        struct stat info{};
        if (stat(path.data(), &info) != 0) continue;
        const bool is_directory = S_ISDIR(info.st_mode);
        if (!is_directory && (!S_ISREG(info.st_mode) || (!media(item->d_name) && !playlist(item->d_name)))) continue;
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
    std::snprintf(title_.data(), title_.size(), "%.255s", slash ? slash + 1 : (path ? path : ""));
    read_metadata(path, metadata_);
    lyric_status_ = lyrics_.load(path);
    if (metadata_.title[0]) std::snprintf(title_.data(), title_.size(), "%s", metadata_.title.data());
    std::snprintf(settings_.resume_path.data(), settings_.resume_path.size(), "%s", path ? path : "");
    settings_.resume_position_ms = player_.position_ms();
    dirty_ = true; changed_at_ = now_;
    nav_.screen = PlayerScreen::kNowPlaying; was_running_ = true;
}
void PlayerFrontend::mode(PlaybackMode value, std::uint32_t now) {
    if (static_cast<unsigned>(value) > 3) return;
    policy_.mode = value; settings_.shuffle = value == PlaybackMode::kShuffle;
    settings_.repeat_mode = value == PlaybackMode::kRepeatTrack ? RepeatMode::kTrack :
        value == PlaybackMode::kRepeatAll ? RepeatMode::kAll : RepeatMode::kOff;
    dirty_ = true; changed_at_ = now;
}
void PlayerFrontend::stopped() {
    settings_.resume_position_ms = std::min<std::uint32_t>(1800000, player_.position_ms());
    dirty_ = true; changed_at_ = now_; auto_advance_ = false; was_running_ = false;
}
bool PlayerFrontend::resume_saved() {
    // A saved absolute path must remain under the selected SD root.
    const auto root_length = std::strlen(root_);
    if (root_length + 1 >= settings_.resume_path.size()) return false;
    if (std::strncmp(settings_.resume_path.data(), root_, root_length) || settings_.resume_path[root_length] != '/') return false;
    std::array<char, 256> checked{};
    if (!local_media_path(root_, settings_.resume_path.data() + root_length + 1, root_, checked.data(), checked.size())) return false;
    std::snprintf(queue_[0].data(), queue_[0].size(), "%s", checked.data()); queue_count_ = 1;
    auto_advance_ = false;
    return play_queue(0, settings_.resume_position_ms);
}
void PlayerFrontend::volume(std::uint8_t value, std::uint32_t now) {
    settings_.volume_percent = std::min<std::uint8_t>(100, value);
    player_.set_volume(settings_.volume_percent); dirty_ = true; changed_at_ = now;
}
bool PlayerFrontend::play(std::size_t index) {
    if (index >= nav_.count || entries_[index].directory) return false;
    if (playlist(entries_[index].name.data())) return open_playlist(index);
    queue_count_ = 0; std::size_t selected = 0;
    for (std::size_t i = 0; i < nav_.count; ++i) {
        if (entries_[i].directory || !media(entries_[i].name.data())) continue;
        if (i == index) selected = queue_count_;
        std::array<char, 512> joined{};
        const int bytes = std::snprintf(joined.data(), joined.size(), "%s/%s", folder_.data(), entries_[i].name.data());
        if (bytes <= 0 || static_cast<std::size_t>(bytes) >= queue_[queue_count_].size()) continue;
        std::strcpy(queue_[queue_count_].data(), joined.data());
        ++queue_count_;
    }
    settings_.playlist_path[0] = 0;
    std::snprintf(settings_.library_folder.data(), settings_.library_folder.size(), "%s", folder_.data());
    auto_advance_ = true; return play_queue(selected);
}
bool PlayerFrontend::open_playlist(std::size_t index) {
    std::array<char, 512> path{};
    std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), entries_[index].name.data());
    // Large bounded playlist lives on heap, not the 6 KiB UI stack.
    std::unique_ptr<Playlist> list(new (std::nothrow) Playlist);
    if (!list || list->load(path.data(), root_) != DocumentStatus::kOk || !list->size()) { nav_.screen = PlayerScreen::kCorrupt; return false; }
    queue_count_ = list->size();
    for (std::size_t i = 0; i < queue_count_; ++i) std::strcpy(queue_[i].data(), list->path(i));
    std::snprintf(settings_.playlist_path.data(), settings_.playlist_path.size(), "%.255s", path.data());
    auto_advance_ = true; return play_queue(0);
}
bool PlayerFrontend::play_queue(std::size_t index, std::uint32_t position) {
    if (index >= queue_count_) return false;
    playing_index_ = index;
    const auto* path = queue_[index].data();
    const auto* slash = std::strrchr(path, '/');
    std::snprintf(title_.data(), title_.size(), "%s", slash ? slash + 1 : path);
    if (!player_.stop()) { nav_.screen = PlayerScreen::kCorrupt; return false; }
    if (!player_.start(path, audio_, headphone_ ? OutputPath::kLine : OutputPath::kSpeaker,
                       settings_.volume_percent, position)) {
        nav_.screen = PlayerScreen::kCorrupt; return false;
    }
    track_started(path); return true;
}
void PlayerFrontend::next(int direction, bool automatic) {
    if (!queue_count_) return;
    const auto index = direction > 0 ? policy_.next(playing_index_, queue_count_, automatic) :
        (playing_index_ + queue_count_ - 1) % queue_count_;
    if (index < queue_count_) play_queue(index);
    else { settings_.resume_position_ms = 0; dirty_ = true; }
}
void PlayerFrontend::parent() {
    if (std::strcmp(folder_.data(), root_) == 0) return;
    auto* slash = std::strrchr(folder_.data(), '/');
    if (slash && static_cast<std::size_t>(slash - folder_.data()) >= std::strlen(root_)) *slash = 0;
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
        else if (e.button == ButtonId::kVolumeDown) nav_.screen = PlayerScreen::kSettings;
        else if (e.button == ButtonId::kNext) nav_.screen = PlayerScreen::kLyrics;
        return;
    }
    if (nav_.screen == PlayerScreen::kSettings) {
        if (e.button == ButtonId::kPrevious) menu_item_ = (menu_item_ + 2) % 3;
        else if (e.button == ButtonId::kNext) menu_item_ = (menu_item_ + 1) % 3;
        else if (e.button == ButtonId::kPlayPause) {
            if (menu_item_ == 0) mode(static_cast<PlaybackMode>((static_cast<unsigned>(policy_.mode) + 1) % 4), e.monotonic_ms);
            else if (menu_item_ == 1) sleep_timer(static_cast<SleepMode>((static_cast<unsigned>(sleep_.mode()) + 1) % 6), e.monotonic_ms);
            else resume_saved();
        }
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
        if (player_.playing()) {
            player_.pause(!player_.paused()); nav_.screen = PlayerScreen::kNowPlaying;
            settings_.resume_position_ms = std::min<std::uint32_t>(1800000, player_.position_ms()); dirty_ = true; changed_at_ = e.monotonic_ms;
        }
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
            text.line(4, metadata_.artist.data());
            text.line(5, !player_.playing() ? "STOPPED / END" : (player_.paused() ? "PAUSED" : "PLAYING"));
            std::snprintf(line.data(), line.size(), "%lu:%02lu / %s", static_cast<unsigned long>(player_.position_ms() / 60000),
                static_cast<unsigned long>(player_.position_ms() / 1000 % 60), metadata_.duration_ms ? "WAV LENGTH KNOWN" : "LENGTH UNKNOWN"); text.line(6, line.data());
            text.line(7, "HOLD NEXT: LYRICS"); break;
        case PlayerScreen::kLyrics: {
            const auto* current = lyrics_.current(player_.position_ms());
            const auto* upcoming = lyrics_.next(player_.position_ms());
            text.line(2, title_.data());
            text.line(4, current ? current->text.data() : lyric_status_ == DocumentStatus::kOk ? "LYRICS START SOON" : "NO VALID .LRC");
            text.line(6, upcoming ? upcoming->text.data() : "");
            text.line(7, "HOLD PLAY: BROWSER"); break;
        }
        case PlayerScreen::kSettings: {
            static const char* modes[] = {"NORMAL", "SHUFFLE", "REPEAT ALL", "REPEAT TRACK"};
            static const char* timers[] = {"OFF", "15 MIN", "30 MIN", "45 MIN", "60 MIN", "END TRACK"};
            std::snprintf(line.data(), line.size(), "%cMODE %s", menu_item_ == 0 ? '>' : ' ', modes[static_cast<unsigned>(policy_.mode)]); text.line(3, line.data());
            std::snprintf(line.data(), line.size(), "%cSLEEP %s", menu_item_ == 1 ? '>' : ' ', timers[static_cast<unsigned>(sleep_.mode())]); text.line(4, line.data());
            text.line(5, menu_item_ == 2 ? ">RESUME LAST TRACK" : " RESUME LAST TRACK");
            text.line(6, "BT HW NOT SELECTED"); text.line(7, "PLAY: CHANGE/RESUME"); break;
        }
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
            text.line(7, truncated_ ? "LIBRARY LIMIT REACHED" : "PLAY: OPEN/PLAY"); break;
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
    now_ = now;
    const bool detected = gpio_get_level(static_cast<gpio_num_t>(hardware::kHeadphoneDetect)) == 0;
    if (detected != detect_candidate_) { detect_candidate_ = detected; route_changed_ = now; }
    if (now - route_changed_ >= 50 && headphone_ != detected) {
        headphone_ = detected; player_.set_output(headphone_ ? OutputPath::kLine : OutputPath::kSpeaker);
    }
    ButtonEvent e;
    while (input_.poll(e)) event(e);
    const bool ended = was_running_ && !player_.playing();
    if (sleep_.expired(now, ended) && !sleep_fading_) {
        stopped(); sleep_fading_ = true; sleep_fade_at_ = now; player_.pause(true);
    }
    if (sleep_fading_ && now - sleep_fade_at_ >= 200 && player_.stop()) {
        stopped(); sleep_fading_ = false; sleep_.set(SleepMode::kOff, now); nav_.last_input_ms = now - 30000;
    }
    if (ended && player_.errors()) { nav_.screen = PlayerScreen::kCorrupt; auto_advance_ = false; }
    else if (ended && auto_advance_ && !sleep_fading_) next(1, true);
    was_running_ = player_.playing();
    if (player_.playing() && now - last_checkpoint_ >= 60000) {
        last_checkpoint_ = now; settings_.resume_position_ms = std::min<std::uint32_t>(1800000, player_.position_ms()); dirty_ = true; changed_at_ = now;
    }
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
