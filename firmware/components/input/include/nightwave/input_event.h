#pragma once

#include <cstdint>

namespace nightwave {

enum class ButtonId : std::uint8_t { kPlayPause, kNext, kPrevious, kVolumeUp, kVolumeDown };
enum class ButtonGesture : std::uint8_t { kPress, kRelease, kLongPress, kRepeat };

struct ButtonEvent {
    ButtonId button{ButtonId::kPlayPause};
    ButtonGesture gesture{ButtonGesture::kPress};
    std::uint32_t monotonic_ms{0};
};

}  // namespace nightwave
