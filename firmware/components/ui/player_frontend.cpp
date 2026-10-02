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
bool catalog_context(const char* value, CatalogFilter& filter, char* text, std::uint32_t& hint) {
    const bool version2 = !std::strncmp(value, "@catalog2:", 10);
    if (!version2 && std::strncmp(value, "@catalog:", 9)) return false;
    const auto* at = value + (version2 ? 10 : 9);
    if (*at < '0' || *at > '2' || at[1] != ':') return false;
    filter = static_cast<CatalogFilter>(*at - '0'); at += 2; hint = UINT32_MAX;
    if (version2) {
        if (*at < '0' || *at > '9') return false;
        std::uint32_t ordinal = 0; unsigned digits = 0;
        while (*at >= '0' && *at <= '9') {
            if (++digits > 4) return false;
            ordinal = ordinal * 10 + static_cast<unsigned>(*at++ - '0');
        }
        if (*at++ != ':') return false;
        hint = ordinal;
    }
    if (std::strlen(at) >= 64) return false;
    std::strcpy(text, at); return true;
}
}
void PlayerFrontend::AssetsDeleter::operator()(UiAssets* p) const {
    if (!p) return;
#ifdef ESP_PLATFORM
    p->~UiAssets(); heap_caps_free(p);
#else
    delete p;
#endif
}
void PlayerFrontend::initialize() {
#ifdef ESP_PLATFORM
    auto* memory = heap_caps_malloc(sizeof(UiAssets), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (memory) assets_.reset(new (memory) UiAssets);
#else
    assets_.reset(new (std::nothrow) UiAssets);
#endif
    if (!assets_) ESP_LOGW("ui", "UI assets unavailable; playback UI disabled, console remains available");
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
    if (assets_ && sd_.mounted()) {
        assets_->catalog.open(root_); assets_->catalog.rebuild(); catalog_started_ = true;
    }
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
    if (assets_ && !auto_advance_ && path && path != assets_->queue[0].data() && std::strlen(path) < assets_->queue[0].size()) {
        std::strcpy(assets_->queue[0].data(), path); queue_count_ = 1; playing_index_ = 0;
    }
    const auto* slash = path ? std::strrchr(path, '/') : nullptr;
    std::snprintf(title_.data(), title_.size(), "%.255s", slash ? slash + 1 : (path ? path : ""));
    read_metadata(path, metadata_);
    lyric_status_ = assets_ ? assets_->lyrics.load(path) : DocumentStatus::kLimit;
    if (metadata_.title[0]) std::snprintf(title_.data(), title_.size(), "%s", metadata_.title.data());
    std::snprintf(settings_.resume_path.data(), settings_.resume_path.size(), "%s", path ? path : "");
    settings_.resume_position_ms = player_.position_ms();
    if (catalog_queue_) save_catalog_context();
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
    catalog_queue_ = catalog_play_pending_ = catalog_page_waiting_ = catalog_resume_pending_ = false;
    if (assets_) {
        assets_->catalog.cancel_query();
        if (settings_.resume_path[0]) { std::strcpy(assets_->queue[0].data(), settings_.resume_path.data()); queue_count_ = 1; playing_index_ = 0; }
    }
}
OutputPath PlayerFrontend::selected_output() const {
    if (settings_.output == OutputPreference::kBluetooth) return OutputPath::kMuted;
    if (settings_.output == OutputPreference::kSpeaker) return OutputPath::kSpeaker;
    if (settings_.output == OutputPreference::kWired) return OutputPath::kLine;
    return headphone_ ? OutputPath::kLine : OutputPath::kSpeaker;
}
bool PlayerFrontend::output_preference(OutputPreference value, std::uint32_t now) {
    // No pretend BLE audio and no unexpected speaker fallback after BT loss.
    if (value == OutputPreference::kBluetooth || static_cast<unsigned>(value) > 3) return false;
    settings_.output = value; player_.set_output(selected_output()); dirty_ = true; changed_at_ = now; return true;
}
bool PlayerFrontend::resume_saved() {
    if (!assets_) return false;
    catalog_play_pending_ = catalog_resume_pending_ = false; catalog_queue_ = false; assets_->catalog.cancel_query();
    // A saved absolute path must remain under the selected SD root.
    const auto root_length = std::strlen(root_);
    if (root_length + 1 >= settings_.resume_path.size()) return false;
    if (std::strncmp(settings_.resume_path.data(), root_, root_length) || settings_.resume_path[root_length] != '/') return false;
    std::array<char, 256> checked{};
    if (!local_media_path(root_, settings_.resume_path.data() + root_length + 1, root_, checked.data(), checked.size())) return false;
    std::size_t index = 0; bool restored = false;
    const auto* saved_list = settings_.playlist_path.data();
    if (*saved_list && !std::strncmp(saved_list, root_, root_length) && saved_list[root_length] == '/' && !std::strstr(saved_list, "/../")) {
        std::unique_ptr<Playlist> list(new (std::nothrow) Playlist);
        if (list && list->load(saved_list, root_) == DocumentStatus::kOk) {
            for (std::size_t i = 0; i < list->size(); ++i) if (!std::strcmp(list->path(i), checked.data())) { index = i; restored = true; break; }
            if (restored) {
                queue_count_ = list->size();
                for (std::size_t i = 0; i < queue_count_; ++i) std::strcpy(assets_->queue[i].data(), list->path(i));
            }
        }
    }
    if (!restored) { std::strcpy(assets_->queue[0].data(), checked.data()); queue_count_ = 1; }
    // Restore a bounded folder queue if this is not a playlist/index context.
    const auto* saved_folder = settings_.library_folder.data();
    std::array<char, 256> folder_probe{}; std::array<char, 512> expected_probe{};
    std::snprintf(expected_probe.data(), expected_probe.size(), "%s/__nightwave_probe__.wav", saved_folder);
    const bool canonical_folder = local_media_path(saved_folder, "__nightwave_probe__.wav", root_, folder_probe.data(), folder_probe.size()) &&
        !std::strcmp(folder_probe.data(), expected_probe.data());
    if (!restored && !*saved_list && !std::strncmp(saved_folder, root_, root_length) &&
        (saved_folder[root_length] == 0 || saved_folder[root_length] == '/') &&
        canonical_folder) {
        std::snprintf(folder_.data(), folder_.size(), "%s", saved_folder);
        if (scan()) {
            queue_count_ = 0;
            for (std::size_t i = 0; i < nav_.count; ++i) {
                if (entries_[i].directory || !media(entries_[i].name.data())) continue;
                std::array<char, 512> candidate{};
                const int length = std::snprintf(candidate.data(), candidate.size(), "%s/%s", folder_.data(), entries_[i].name.data());
                if (length <= 0 || static_cast<std::size_t>(length) >= 256) continue;
                if (!std::strcmp(candidate.data(), checked.data())) { index = queue_count_; restored = true; }
                std::strcpy(assets_->queue[queue_count_++].data(), candidate.data());
            }
        }
        if (!restored) { queue_count_ = 1; index = 0; std::strcpy(assets_->queue[0].data(), checked.data()); }
    }
    auto_advance_ = restored;
    const bool started = play_queue(index, settings_.resume_position_ms);
    if (started && !restored && catalog_context(saved_folder, queue_filter_, queue_filter_text_.data(), resume_hint_)) {
        catalog_resume_pending_ = assets_->catalog.locate_song(queue_filter_, queue_filter_text_.data(), checked.data(), resume_hint_);
    }
    return started;
}
void PlayerFrontend::volume(std::uint8_t value, std::uint32_t now) {
    settings_.volume_percent = std::min<std::uint8_t>(100, value);
    player_.set_volume(settings_.volume_percent); dirty_ = true; changed_at_ = now;
}
bool PlayerFrontend::play(std::size_t index) {
    if (!assets_ || index >= nav_.count || entries_[index].directory) return false;
    if (playlist(entries_[index].name.data())) return open_playlist(index);
    catalog_play_pending_ = catalog_resume_pending_ = catalog_queue_ = false; assets_->catalog.cancel_query();
    queue_count_ = 0; std::size_t selected = 0;
    for (std::size_t i = 0; i < nav_.count; ++i) {
        if (entries_[i].directory || !media(entries_[i].name.data())) continue;
        if (i == index) selected = queue_count_;
        std::array<char, 512> joined{};
        const int bytes = std::snprintf(joined.data(), joined.size(), "%s/%s", folder_.data(), entries_[i].name.data());
        if (bytes <= 0 || static_cast<std::size_t>(bytes) >= assets_->queue[queue_count_].size()) continue;
        std::strcpy(assets_->queue[queue_count_].data(), joined.data());
        ++queue_count_;
    }
    settings_.playlist_path[0] = 0;
    std::snprintf(settings_.library_folder.data(), settings_.library_folder.size(), "%s", folder_.data());
    auto_advance_ = true; return play_queue(selected);
}
bool PlayerFrontend::open_playlist(std::size_t index) {
    if (!assets_) return false;
    catalog_play_pending_ = catalog_resume_pending_ = catalog_queue_ = false; assets_->catalog.cancel_query();
    std::array<char, 512> path{};
    std::snprintf(path.data(), path.size(), "%s/%s", folder_.data(), entries_[index].name.data());
    // Large bounded playlist lives on heap, not the 6 KiB UI stack.
    std::unique_ptr<Playlist> list(new (std::nothrow) Playlist);
    if (!list || list->load(path.data(), root_) != DocumentStatus::kOk || !list->size()) { nav_.screen = PlayerScreen::kCorrupt; return false; }
    queue_count_ = list->size();
    for (std::size_t i = 0; i < queue_count_; ++i) std::strcpy(assets_->queue[i].data(), list->path(i));
    std::snprintf(settings_.playlist_path.data(), settings_.playlist_path.size(), "%.255s", path.data());
    auto_advance_ = true; return play_queue(0);
}
bool PlayerFrontend::play_queue(std::size_t index, std::uint32_t position) {
    if (assets_ && catalog_queue_) {
        if (index >= catalog_queue_count_) return false;
        pending_index_ = static_cast<std::uint32_t>(index); pending_position_ = position;
        catalog_page_waiting_ = false;
        catalog_play_pending_ = assets_->catalog.seek_song(queue_filter_, queue_filter_text_.data(), pending_index_);
        if (!catalog_play_pending_) { nav_.screen = PlayerScreen::kCorrupt; auto_advance_ = false; }
        return catalog_play_pending_;
    }
    if (!assets_ || index >= queue_count_) return false;
    playing_index_ = index;
    const auto* path = assets_->queue[index].data();
    return start_path(path, position);
}
bool PlayerFrontend::start_path(const char* path, std::uint32_t position) {
    const auto* slash = std::strrchr(path, '/');
    std::snprintf(title_.data(), title_.size(), "%s", slash ? slash + 1 : path);
    if (!player_.stop()) { nav_.screen = PlayerScreen::kCorrupt; return false; }
    if (!player_.start(path, audio_, selected_output(),
                       settings_.volume_percent, position)) {
        nav_.screen = PlayerScreen::kCorrupt; return false;
    }
    track_started(path); return true;
}
void PlayerFrontend::next(int direction, bool automatic) {
    const auto count = queue_size();
    if (!count || catalog_play_pending_ || catalog_resume_pending_) return;
    const auto index = direction > 0 ? policy_.next(playing_index_, count, automatic) :
        (playing_index_ + count - 1) % count;
    if (index < count) play_queue(index);
    else { settings_.resume_position_ms = 0; dirty_ = true; }
}
void PlayerFrontend::parent() {
    if (nav_.screen == PlayerScreen::kCatalog) {
        if (catalog_filter_ != CatalogFilter::kNone) {
            catalog_view_ = catalog_filter_ == CatalogFilter::kArtist ? CatalogView::kArtists : CatalogView::kAlbums;
            catalog_filter_ = CatalogFilter::kNone; catalog_filter_text_[0] = catalog_anchor_[0] = 0;
            catalog_offset_ = 0; catalog_backwards_ = false; catalog_query();
        } else { if (assets_) assets_->catalog.cancel_query(); catalog_page_waiting_ = false; nav_.screen = PlayerScreen::kLibrary; nav_.populate(4); }
        return;
    }
    if (nav_.screen == PlayerScreen::kLibrary) { nav_.screen = PlayerScreen::kBrowser; scan(); return; }
    if (std::strcmp(folder_.data(), root_) == 0) return;
    auto* slash = std::strrchr(folder_.data(), '/');
    if (slash && static_cast<std::size_t>(slash - folder_.data()) >= std::strlen(root_)) *slash = 0;
    nav_.screen = scan() ? PlayerScreen::kBrowser : PlayerScreen::kNoSd;
}
void PlayerFrontend::catalog_query(bool last_cursor) {
    nav_.screen = PlayerScreen::kCatalog; nav_.populate(0);
    catalog_play_pending_ = catalog_resume_pending_ = false; catalog_last_cursor_ = last_cursor;
    catalog_page_waiting_ = assets_ && assets_->catalog.query(catalog_view_, catalog_filter_,
        catalog_filter_text_.data(), catalog_offset_, catalog_anchor_.data(), catalog_backwards_);
}
void PlayerFrontend::catalog_selected() {
    if (!assets_) return;
    const auto& page = assets_->catalog.page();
    if (!page.complete || page.error || nav_.cursor >= page.count) return;
    const auto& row = page.rows[nav_.cursor];
    if (catalog_view_ != CatalogView::kSongs) {
        catalog_filter_ = catalog_view_ == CatalogView::kArtists ? CatalogFilter::kArtist : CatalogFilter::kAlbum;
        const auto* tag = catalog_filter_ == CatalogFilter::kArtist ? row.metadata.artist.data() : row.metadata.album.data();
        std::snprintf(catalog_filter_text_.data(), catalog_filter_text_.size(), "%s", *tag ? tag : "(UNKNOWN)");
        catalog_view_ = CatalogView::kSongs; catalog_offset_ = 0; catalog_anchor_[0] = 0;
        catalog_backwards_ = false; catalog_query(); return;
    }
    catalog_queue_count_ = page.matches; playing_index_ = catalog_offset_ + nav_.cursor;
    queue_filter_ = catalog_filter_; queue_filter_text_ = catalog_filter_text_;
    catalog_queue_ = auto_advance_ = true; catalog_play_pending_ = false;
    std::strcpy(assets_->queue[0].data(), row.path.data()); queue_count_ = 1;
    settings_.playlist_path[0] = 0;
    save_catalog_context();
    start_path(assets_->queue[0].data());
}
void PlayerFrontend::save_catalog_context() {
    std::snprintf(settings_.library_folder.data(), settings_.library_folder.size(), "@catalog2:%u:%lu:%s", static_cast<unsigned>(queue_filter_),
        static_cast<unsigned long>(playing_index_), queue_filter_text_.data());
}
void PlayerFrontend::catalog_event(const ButtonEvent& e) {
    if (!assets_ || assets_->catalog.querying() || catalog_play_pending_) return;
    const auto& page = assets_->catalog.page();
    if (!page.complete || page.error) return;
    if (e.button == ButtonId::kPlayPause) { catalog_selected(); return; }
    if (!page.count) return;
    if (e.button == ButtonId::kNext && nav_.cursor + 1 == page.count) {
        // A backwards group page reports whether more *earlier* keys exist.
        const bool later = catalog_view_ == CatalogView::kSongs ? page.more : (catalog_backwards_ || page.more);
        if (!later) return;
        if (catalog_view_ == CatalogView::kSongs) catalog_offset_ += CatalogPage::kSize;
        else {
            const auto& row = page.rows[page.count - 1];
            const auto* key = catalog_view_ == CatalogView::kArtists ? row.metadata.artist.data() : row.metadata.album.data();
            std::snprintf(catalog_anchor_.data(), catalog_anchor_.size(), "%s", *key ? key : "(UNKNOWN)");
        }
        catalog_backwards_ = false; catalog_query();
    } else if (e.button == ButtonId::kPrevious && !nav_.cursor) {
        if (catalog_view_ == CatalogView::kSongs) {
            if (!catalog_offset_) return;
            catalog_offset_ -= std::min<std::uint32_t>(CatalogPage::kSize, catalog_offset_);
        } else {
            if (!*catalog_anchor_.data() && !catalog_backwards_) return;
            if (catalog_backwards_ && !page.more) return;
            const auto& row = page.rows[0];
            const auto* key = catalog_view_ == CatalogView::kArtists ? row.metadata.artist.data() : row.metadata.album.data();
            std::snprintf(catalog_anchor_.data(), catalog_anchor_.size(), "%s", *key ? key : "(UNKNOWN)");
            catalog_backwards_ = true;
        }
        catalog_query(true);
    } else if (e.button == ButtonId::kNext) nav_.move(1);
    else if (e.button == ButtonId::kPrevious) nav_.move(-1);
}
void PlayerFrontend::event(const ButtonEvent& e) {
    if (!nav_.interact(e.monotonic_ms)) return;
    if (e.gesture == ButtonGesture::kLongPress) {
        if (e.button == ButtonId::kPlayPause) {
            if (assets_) assets_->catalog.cancel_query();
            catalog_play_pending_ = catalog_page_waiting_ = catalog_resume_pending_ = false;
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
        if (e.button == ButtonId::kPrevious) menu_item_ = (menu_item_ + 5) % 6;
        else if (e.button == ButtonId::kNext) menu_item_ = (menu_item_ + 1) % 6;
        else if (e.button == ButtonId::kPlayPause) {
            if (menu_item_ == 0) mode(static_cast<PlaybackMode>((static_cast<unsigned>(policy_.mode) + 1) % 4), e.monotonic_ms);
            else if (menu_item_ == 1) sleep_timer(static_cast<SleepMode>((static_cast<unsigned>(sleep_.mode()) + 1) % 6), e.monotonic_ms);
            else if (menu_item_ == 2) resume_saved();
            else if (menu_item_ == 3) output_preference(static_cast<OutputPreference>((static_cast<unsigned>(settings_.output) + 1) % 3), e.monotonic_ms);
            else if (menu_item_ == 4) { nav_.screen = PlayerScreen::kLibrary; nav_.populate(4); }
            else if (assets_ && !player_.playing()) {
                catalog_queue_ = catalog_play_pending_ = catalog_resume_pending_ = false; auto_advance_ = false; queue_count_ = 0;
                assets_->catalog.cancel_query(); assets_->catalog.rebuild(); catalog_started_ = true;
                nav_.screen = PlayerScreen::kLibrary; nav_.populate(4);
            }
        }
        return;
    }
    if (e.button == ButtonId::kVolumeUp || e.button == ButtonId::kVolumeDown) {
        const int value = settings_.volume_percent + (e.button == ButtonId::kVolumeUp ? 2 : -2);
        volume(static_cast<std::uint8_t>(std::clamp(value, 0, 100)), e.monotonic_ms); return;
    }
    if (nav_.screen == PlayerScreen::kLibrary) {
        if (e.button == ButtonId::kPrevious) nav_.move(-1);
        else if (e.button == ButtonId::kNext) nav_.move(1);
        else if (e.button == ButtonId::kPlayPause) {
            if (nav_.cursor == 3) { scan(); nav_.screen = PlayerScreen::kBrowser; }
            else {
                catalog_view_ = static_cast<CatalogView>(nav_.cursor); catalog_filter_ = CatalogFilter::kNone;
                catalog_offset_ = 0; catalog_filter_text_[0] = catalog_anchor_[0] = 0;
                catalog_backwards_ = false; catalog_query();
            }
        }
        return;
    }
    if (nav_.screen == PlayerScreen::kCatalog) { catalog_event(e); return; }
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
            text.line(2, metadata_.album.data());
            text.line(3, title_.data());
            text.line(4, metadata_.artist.data());
            text.line(5, !player_.playing() ? "STOPPED / END" : (player_.paused() ? "PAUSED" : "PLAYING"));
            if (metadata_.duration_ms) std::snprintf(line.data(), line.size(), "%lu:%02lu / %lu:%02lu", static_cast<unsigned long>(player_.position_ms() / 60000),
                static_cast<unsigned long>(player_.position_ms() / 1000 % 60), static_cast<unsigned long>(metadata_.duration_ms / 60000), static_cast<unsigned long>(metadata_.duration_ms / 1000 % 60));
            else std::snprintf(line.data(), line.size(), "%lu:%02lu / UNKNOWN", static_cast<unsigned long>(player_.position_ms() / 60000), static_cast<unsigned long>(player_.position_ms() / 1000 % 60));
            text.line(6, line.data());
            text.line(7, catalog_resume_pending_ ? "RESTORING QUEUE" : catalog_play_pending_ ? "LOADING NEXT TRACK" : "HOLD NEXT: LYRICS"); break;
        case PlayerScreen::kLyrics: {
            const auto* current = assets_ ? assets_->lyrics.current(player_.position_ms()) : nullptr;
            const auto* upcoming = assets_ ? assets_->lyrics.next(player_.position_ms()) : nullptr;
            text.line(2, title_.data());
            text.line(4, current ? current->text.data() : lyric_status_ == DocumentStatus::kOk ? "LYRICS START SOON" : "NO VALID .LRC");
            text.line(6, upcoming ? upcoming->text.data() : "");
            text.line(7, "HOLD PLAY: BROWSER"); break;
        }
        case PlayerScreen::kSettings: {
            static const char* modes[] = {"NORMAL", "SHUFFLE", "REPEAT ALL", "REPEAT TRACK"};
            static const char* timers[] = {"OFF", "15 MIN", "30 MIN", "45 MIN", "60 MIN", "END TRACK"};
            static const char* outputs[] = {"AUTO", "SPEAKER", "WIRED", "BT UNAVAILABLE"};
            if (menu_item_ < 4) {
                std::snprintf(line.data(), line.size(), "%cMODE %s", menu_item_ == 0 ? '>' : ' ', modes[static_cast<unsigned>(policy_.mode)]); text.line(3, line.data());
                std::snprintf(line.data(), line.size(), "%cSLEEP %s", menu_item_ == 1 ? '>' : ' ', timers[static_cast<unsigned>(sleep_.mode())]); text.line(4, line.data());
                text.line(5, menu_item_ == 2 ? ">RESUME LAST TRACK" : " RESUME LAST TRACK");
                std::snprintf(line.data(), line.size(), "%cOUTPUT %s", menu_item_ == 3 ? '>' : ' ', outputs[static_cast<unsigned>(settings_.output)]); text.line(6, line.data());
            } else {
                text.line(3, menu_item_ == 4 ? ">LIBRARY" : " LIBRARY");
                text.line(4, menu_item_ == 5 ? ">REBUILD INDEX" : " REBUILD INDEX");
                text.line(6, player_.playing() ? "REBUILD: STOP FIRST" : "REBUILD: IDLE ONLY");
            }
            text.line(7, "BT HW NOT SELECTED"); break;
        }
        case PlayerScreen::kLibrary: {
            static const char* views[] = {"SONGS", "ARTISTS", "ALBUMS", "FOLDERS / PLAYLISTS"};
            for (std::size_t i = 0; i < 4; ++i) { std::snprintf(line.data(), line.size(), "%c%s", i == nav_.cursor ? '>' : ' ', views[i]); text.line(3 + i, line.data()); }
            if (assets_) {
                const auto& catalog = assets_->catalog;
                text.line(7, catalog.failed() ? "INDEX I/O ERROR" : catalog.building() ? (player_.playing() ? "INDEX PAUSED: PLAYING" : "INDEXING WHILE IDLE") : catalog.limited() ? "PARTIAL INDEX / LIMIT" : catalog.lookup_loading() ? "LOOKUP WARMING" : !catalog.fast_lookup_ready() ? "SLOW LOOKUP / FALLBACK" : "PLAY: OPEN LIBRARY");
            }
            break;
        }
        case PlayerScreen::kCatalog: {
            static const char* views[] = {"SONGS", "ARTISTS", "ALBUMS"};
            text.line(2, *catalog_filter_text_.data() ? catalog_filter_text_.data() : views[static_cast<unsigned>(catalog_view_)]);
            if (!assets_) { text.line(4, "UI MEMORY UNAVAILABLE"); break; }
            const auto& catalog = assets_->catalog;
            const auto& page = catalog.page();
            if (!catalog.ready()) text.line(4, catalog.failed() ? "INDEX I/O ERROR" : "INDEXING: WAIT IDLE");
            else if (catalog.querying()) text.line(4, "LOADING PAGE");
            else if (page.error) text.line(4, "CACHE ERROR: REBUILD");
            else if (!page.count) text.line(4, "NO MATCHING TRACKS");
            else {
                const auto start = nav_.cursor / 4 * 4;
                for (std::size_t i = 0; i < 4 && start + i < page.count; ++i) {
                    const auto& row = page.rows[start + i];
                    const auto* label = catalog_view_ == CatalogView::kSongs ? row.metadata.title.data() :
                        catalog_view_ == CatalogView::kArtists ? row.metadata.artist.data() : row.metadata.album.data();
                    std::snprintf(line.data(), line.size(), "%c%.20s", start + i == nav_.cursor ? '>' : ' ', *label ? label : "(UNKNOWN)"); text.line(3 + i, line.data());
                }
            }
            text.line(7, catalog.limited() ? "PARTIAL INDEX / LIMIT" : catalog.lookup_loading() ? "LOOKUP WARMING" : catalog.ready() && !catalog.fast_lookup_ready() ? "SLOW LOOKUP / FALLBACK" : "HOLD PREV: UP"); break;
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
        headphone_ = detected; player_.set_output(selected_output());
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
    if (assets_ && sd_.mounted()) {
        auto& catalog = assets_->catalog;
        if (!catalog_started_) { catalog.open(root_); catalog_started_ = true; catalog.rebuild(); }
        const bool was_fast = catalog.fast_lookup_ready();
        catalog.pump_lookup(4);
        if (!was_fast && catalog.fast_lookup_ready()) {
            if (catalog_play_pending_) catalog.seek_song(queue_filter_, queue_filter_text_.data(), pending_index_);
            else if (catalog_resume_pending_) catalog.locate_song(queue_filter_, queue_filter_text_.data(), settings_.resume_path.data(), resume_hint_);
            else if (nav_.screen == PlayerScreen::kCatalog) catalog_query();
        }
        const bool was_building = catalog.building();
        if (!player_.playing() && !catalog_queue_ && !catalog.querying()) catalog.pump_build(2);
        if (nav_.screen == PlayerScreen::kCatalog && !catalog_play_pending_ &&
            ((was_building && !catalog.building() && catalog.ready()) || (!catalog_page_waiting_ && !catalog.page().complete && !catalog.page().error && catalog.ready()))) catalog_query();
        catalog.pump_query(4);
        if (catalog_resume_pending_ && !catalog.querying()) {
            catalog_resume_pending_ = false;
            const auto& page = catalog.page();
            if (page.complete && !page.error && page.count && catalog.located_ordinal() < page.matches) {
                catalog_queue_ = true; catalog_queue_count_ = page.matches; playing_index_ = catalog.located_ordinal();
                save_catalog_context(); dirty_ = true; changed_at_ = now;
                auto_advance_ = player_.playing();
            }
        } else if (catalog_play_pending_ && !catalog.querying()) {
            catalog_play_pending_ = false;
            const auto& page = catalog.page();
            if (!page.complete || page.error || !page.count) { nav_.screen = PlayerScreen::kCorrupt; auto_advance_ = false; }
            else {
                std::strcpy(assets_->queue[0].data(), page.rows[0].path.data()); queue_count_ = 1;
                playing_index_ = pending_index_; start_path(assets_->queue[0].data(), pending_position_);
                was_running_ = player_.playing();
            }
        } else if (catalog_page_waiting_ && !catalog.querying()) {
            catalog_page_waiting_ = false; nav_.populate(catalog.page().count);
            if (catalog_last_cursor_ && nav_.count) nav_.cursor = nav_.count - 1;
        }
    }
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
