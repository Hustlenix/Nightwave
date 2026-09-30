#pragma once

#include <cstdint>

#include "nightwave/app_state.h"
#include "nightwave/playback_command.h"

namespace nightwave {

class PlaybackStateMachine {
  public:
    AppState state() const { return state_; }
    std::uint8_t volume_percent() const { return volume_percent_; }

    bool apply(const PlaybackCommand& command) {
        switch (command.type) {
            case PlaybackCommandType::kPlay:
                state_ = AppState::kPlaying;
                return true;
            case PlaybackCommandType::kPause:
                if (state_ != AppState::kPlaying) return false;
                state_ = AppState::kPaused;
                return true;
            case PlaybackCommandType::kTogglePause:
                if (state_ == AppState::kPlaying) {
                    state_ = AppState::kPaused;
                } else if (state_ == AppState::kPaused || state_ == AppState::kIdle) {
                    state_ = AppState::kPlaying;
                } else {
                    return false;
                }
                return true;
            case PlaybackCommandType::kStop:
                state_ = AppState::kIdle;
                return true;
            case PlaybackCommandType::kSetVolume:
                volume_percent_ = static_cast<std::uint8_t>(
                    command.value < 0 ? 0 : (command.value > 100 ? 100 : command.value));
                return true;
            case PlaybackCommandType::kNext:
            case PlaybackCommandType::kPrevious:
            case PlaybackCommandType::kSeekRelativeMs:
                return state_ == AppState::kPlaying || state_ == AppState::kPaused;
        }
        return false;
    }

  private:
    AppState state_{AppState::kIdle};
    std::uint8_t volume_percent_{20};
};

}  // namespace nightwave
