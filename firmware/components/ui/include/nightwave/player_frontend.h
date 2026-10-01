#pragma once
#include <array>
#include "nightwave/player_navigation.h"
#include "nightwave/oled_display.h"
#include "nightwave/streaming_player.h"
#include "nightwave/storage.h"
#include "nightwave/settings.h"
#include "nightwave/button_monitor.h"
namespace nightwave {
// Main/UiTask call these methods under one command mutex. I2S remains owned by
// AudioOutputTask; this class only changes atomic controls or waits for stop.
class PlayerFrontend {
 public:
    PlayerFrontend(SdStorage& sd, StreamingPlayer& player, I2sAudioSink& audio,
                   ButtonMonitor& input) : sd_(sd), player_(player), audio_(audio), input_(input) {}
    void initialize();
    void tick(std::uint32_t now);
    void track_started(const char* path);
    void volume(std::uint8_t value, std::uint32_t now);
 private:
    struct Entry { std::array<char, 256> name{}; bool directory{false}; };
    bool scan();
    bool play(std::size_t index);
    void next(int direction);
    void parent();
    void event(const ButtonEvent&);
    TextFrame frame() const;
    SdStorage& sd_; StreamingPlayer& player_; I2sAudioSink& audio_; ButtonMonitor& input_;
    OledDisplay oled_{};
    PlayerNavigation nav_{};
    Settings settings_{};
    std::array<Entry, 128> entries_{};
    std::array<char, 256> folder_{'/','s','d','c','a','r','d',0};
    std::array<char, 256> title_{};
    std::size_t playing_index_{0};
    std::uint32_t last_render_{0}, changed_at_{0}, route_changed_{0};
    bool oled_ready_{false}, dirty_{false}, nvs_ready_{false}, was_running_{false};
    bool headphone_{false}, detect_candidate_{false}, truncated_{false}, display_asleep_{false};
};
}  // namespace nightwave
