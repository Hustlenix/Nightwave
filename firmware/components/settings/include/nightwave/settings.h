#pragma once

#include <cstdint>

namespace nightwave {

enum class RepeatMode : std::uint8_t { kOff, kTrack, kAll };

struct Settings {
    std::uint8_t volume_percent{50};
    RepeatMode repeat_mode{RepeatMode::kOff};
    bool shuffle{false};
    std::uint32_t resume_track_id{0};
    std::uint32_t resume_position_ms{0};
};

}  // namespace nightwave
