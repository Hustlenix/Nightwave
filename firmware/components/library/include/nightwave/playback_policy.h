#pragma once
#include <cstddef>
#include <cstdint>
namespace nightwave {
enum class PlaybackMode : std::uint8_t { kNormal, kShuffle, kRepeatAll, kRepeatTrack };
enum class SleepMode : std::uint8_t { kOff, k15, k30, k45, k60, kEndTrack };
class PlaybackPolicy {
 public:
    PlaybackMode mode{PlaybackMode::kNormal};
    std::size_t next(std::size_t current, std::size_t count, bool automatic) {
        if (!count) return count;
        if (automatic && mode == PlaybackMode::kRepeatTrack) return current < count ? current : 0;
        if (mode == PlaybackMode::kShuffle && count > 1) {
            random_ ^= random_ << 13; random_ ^= random_ >> 17; random_ ^= random_ << 5;
            return (current + 1 + random_ % (count - 1)) % count;
        }
        const auto candidate = current + 1;
        if (candidate < count) return candidate;
        return automatic && mode == PlaybackMode::kNormal ? count : 0;
    }
 private:
    std::uint32_t random_{0x4e575631}; // Not cryptographic.
};
class SleepTimer {
 public:
    void set(SleepMode mode, std::uint32_t now) { mode_ = mode; started_ = now; }
    SleepMode mode() const { return mode_; }
    bool expired(std::uint32_t now, bool ended) const {
        if (mode_ == SleepMode::kOff) return false;
        if (mode_ == SleepMode::kEndTrack) return ended;
        return now - started_ >= static_cast<std::uint32_t>(mode_) * 15U * 60000U;
    }
 private:
    SleepMode mode_{SleepMode::kOff}; std::uint32_t started_{0};
};
}
