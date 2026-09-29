#pragma once

#include <array>
#include <cstdint>

#include "nightwave/app_state.h"

namespace nightwave {

struct UiModel {
    AppState state{AppState::kBooting};
    std::array<char, 64> title{};
    std::uint32_t elapsed_ms{0};
    std::uint32_t duration_ms{0};
    std::uint8_t volume_percent{0};
    std::uint8_t battery_percent{0};
    bool headphones_connected{false};
};

}  // namespace nightwave
