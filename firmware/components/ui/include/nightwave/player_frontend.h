#pragma once
#include <array>
#include <cstdio>
#include <memory>
#include "nightwave/player_navigation.h"
#include "nightwave/oled_display.h"
#include "nightwave/streaming_player.h"
#include "nightwave/storage.h"
#include "nightwave/settings.h"
#include "nightwave/button_monitor.h"
#include "nightwave/media_documents.h"
#include "nightwave/playback_policy.h"
#include "nightwave/library_catalog.h"
namespace nightwave {
// Main/UiTask call these methods under one command mutex. I2S remains owned by
// AudioOutputTask; this class only changes atomic controls or waits for stop.
class PlayerFrontend {
 public:
    PlayerFrontend(SdStorage& sd, StreamingPlayer& player, I2sAudioSink& audio,
                   ButtonMonitor& input, const char* root = "/sdcard")
        : sd_(sd), player_(player), audio_(audio), input_(input), root_(root) {
        std::snprintf(folder_.data(), folder_.size(), "%s", root_);
    }
    void initialize();
    void tick(std::uint32_t now);
    void track_started(const char* path);
    void volume(std::uint8_t value, std::uint32_t now);
    void mode(PlaybackMode value, std::uint32_t now);
    void sleep_timer(SleepMode value, std::uint32_t now) { sleep_.set(value, now); }
    bool resume_saved();
    bool seek_current(std::uint32_t position) { return play_queue(playing_index_, position); }
    void stopped();
    bool output_preference(OutputPreference value, std::uint32_t now);
    std::size_t queue_size() const { return assets_ ? (catalog_queue_ ? catalog_queue_count_ : queue_count_) : 0; }
    std::uint32_t catalog_scanned() const { return assets_ ? assets_->catalog.scanned() : 0; }
    bool catalog_building() const { return assets_ && assets_->catalog.building(); }
    bool catalog_fast_lookup() const { return assets_ && assets_->catalog.fast_lookup_ready(); }
 private:
    struct Entry { std::array<char, 256> name{}; bool directory{false}; };
    struct UiAssets {
        LibraryCatalog catalog{};
        Lyrics lyrics{};
        std::array<std::array<char, 256>, 128> queue{};
    };
    struct AssetsDeleter { void operator()(UiAssets*) const; };
    bool scan();
    bool play(std::size_t index);
    bool play_queue(std::size_t index, std::uint32_t position = 0);
    bool open_playlist(std::size_t index);
    bool start_path(const char* path, std::uint32_t position = 0);
    void catalog_query(bool last_cursor = false);
    void catalog_event(const ButtonEvent&);
    void catalog_selected();
    void save_catalog_context();
    OutputPath selected_output() const;
    void next(int direction, bool automatic = false);
    void parent();
    void event(const ButtonEvent&);
    TextFrame frame() const;
    SdStorage& sd_; StreamingPlayer& player_; I2sAudioSink& audio_; ButtonMonitor& input_;
    const char* root_;
    OledDisplay oled_{};
    PlayerNavigation nav_{};
    Settings settings_{};
    PlaybackPolicy policy_{};
    SleepTimer sleep_{};
    TrackMetadata metadata_{};
    std::unique_ptr<UiAssets, AssetsDeleter> assets_{};
    DocumentStatus lyric_status_{DocumentStatus::kMissing};
    std::array<Entry, 128> entries_{};
    std::array<char, 256> folder_{'/','s','d','c','a','r','d',0};
    std::array<char, 256> title_{};
    std::size_t queue_count_{0}, menu_item_{0};
    std::size_t playing_index_{0};
    std::uint32_t last_render_{0}, changed_at_{0}, route_changed_{0};
    std::uint32_t last_checkpoint_{0}, sleep_fade_at_{0};
    std::uint32_t now_{0};
    CatalogView catalog_view_{CatalogView::kSongs};
    CatalogFilter catalog_filter_{CatalogFilter::kNone}, queue_filter_{CatalogFilter::kNone};
    std::array<char, 64> catalog_filter_text_{}, catalog_anchor_{}, queue_filter_text_{};
    std::uint32_t catalog_offset_{0}, catalog_queue_count_{0}, pending_index_{0}, pending_position_{0};
    std::uint32_t resume_hint_{UINT32_MAX};
    bool oled_ready_{false}, dirty_{false}, nvs_ready_{false}, was_running_{false};
    bool headphone_{false}, detect_candidate_{false}, truncated_{false}, display_asleep_{false};
    bool auto_advance_{false}, sleep_fading_{false};
    bool catalog_started_{false}, catalog_queue_{false}, catalog_play_pending_{false}, catalog_resume_pending_{false};
    bool catalog_last_cursor_{false}, catalog_backwards_{false}, catalog_page_waiting_{false};
};
}  // namespace nightwave
