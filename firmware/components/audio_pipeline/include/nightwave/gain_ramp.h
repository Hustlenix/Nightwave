#pragma once
#include <cstdint>
#include "nightwave/audio_math.h"
namespace nightwave {
// Output-task-owned, bounded 1/1024-full-scale steps (~23 ms at 44.1 kHz).
class GainRamp {
 public:
    void reset() { gain_ = 0; }
    std::uint16_t step(std::uint16_t target) {
        if (gain_ < target) gain_ = static_cast<std::uint16_t>(
            target - gain_ > 32 ? gain_ + 32 : target);
        else if (gain_ > target) gain_ = static_cast<std::uint16_t>(
            gain_ - target > 32 ? gain_ - 32 : target);
        return gain_;
    }
    std::uint16_t value() const { return gain_; }
 private:
    std::uint16_t gain_{0};
};
}  // namespace nightwave
