#pragma once

#include <cstdint>

namespace nightwave {

class ButtonDebouncer {
  public:
    ButtonDebouncer() = default;
    explicit ButtonDebouncer(std::uint32_t debounce_ms)
        : debounce_ms_(debounce_ms) {}

    bool update(bool raw_pressed, std::uint32_t now_ms) {
        if (raw_pressed != candidate_pressed_) {
            candidate_pressed_ = raw_pressed;
            candidate_since_ms_ = now_ms;
        }
        if (stable_pressed_ != candidate_pressed_ &&
            static_cast<std::uint32_t>(now_ms - candidate_since_ms_) >=
                debounce_ms_) {
            stable_pressed_ = candidate_pressed_;
            changed_ = true;
            return true;
        }
        return false;
    }

    bool pressed() const { return stable_pressed_; }
    bool consume_changed() {
        const bool changed = changed_;
        changed_ = false;
        return changed;
    }

  private:
    std::uint32_t debounce_ms_{25};
    std::uint32_t candidate_since_ms_{0};
    bool candidate_pressed_{false};
    bool stable_pressed_{false};
    bool changed_{false};
};

}  // namespace nightwave
