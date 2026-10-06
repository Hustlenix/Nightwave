#pragma once
#include <array>
#include "nightwave/button_debouncer.h"
#include "nightwave/input_event.h"

namespace nightwave {
// Lost samples cancel gestures. Recovery requires release before a new press.
class ButtonGestures {
 public:
    void invalidate() { armed_.fill(false); debouncers_ = {}; long_sent_.fill(false); }
    template<class Emit>
    void sample(std::uint8_t pressed_bits, std::uint32_t now, Emit emit) {
        constexpr std::array<ButtonId, 5> ids{ButtonId::kPrevious, ButtonId::kPlayPause,
            ButtonId::kNext, ButtonId::kVolumeDown, ButtonId::kVolumeUp};
        for (std::size_t i = 0; i < ids.size(); ++i) {
            const bool pressed = (pressed_bits & (1U << i)) != 0;
            if (!armed_[i]) { if (!pressed) armed_[i] = true; continue; }
            if (debouncers_[i].update(pressed, now)) {
                if (debouncers_[i].pressed()) { pressed_at_[i] = now; long_sent_[i] = false; }
                else if (!long_sent_[i]) emit(ButtonEvent{ids[i], ButtonGesture::kPress, now});
            }
            if (debouncers_[i].pressed() && !long_sent_[i] && now - pressed_at_[i] >= 700) {
                long_sent_[i] = true;
                emit(ButtonEvent{ids[i], ButtonGesture::kLongPress, now});
            }
        }
    }
 private:
    std::array<ButtonDebouncer, 5> debouncers_{};
    std::array<std::uint32_t, 5> pressed_at_{};
    std::array<bool, 5> armed_{}, long_sent_{};
};
} // namespace nightwave
