#pragma once

#include <cstdint>

namespace nightwave {

enum class PlaybackCommandType : std::uint8_t {
    kPlay,
    kPause,
    kTogglePause,
    kNext,
    kPrevious,
    kSeekRelativeMs,
    kSetVolume,
    kStop,
};

struct PlaybackCommand {
    PlaybackCommandType type{PlaybackCommandType::kPlay};
    std::int32_t value{0};
    std::uint32_t issued_at_ms{0};
};

}  // namespace nightwave
