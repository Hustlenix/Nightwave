#pragma once

#include <cstddef>
#include <cstdint>

namespace nightwave {

struct AudioFormat {
    std::uint32_t sample_rate_hz{44100};
    std::uint8_t channel_count{2};
    std::uint8_t bits_per_sample{16};

    constexpr bool supported() const {
        const bool rate_ok = sample_rate_hz == 22050 || sample_rate_hz == 32000 ||
                             sample_rate_hz == 44100 || sample_rate_hz == 48000;
        return rate_ok && (channel_count == 1 || channel_count == 2) &&
               bits_per_sample == 16;
    }
};

struct PcmBlock {
    const std::int16_t* interleaved_samples{nullptr};
    std::size_t frame_count{0};
    AudioFormat format{};
    std::uint64_t first_frame_index{0};
};

inline constexpr std::size_t kPcmQueueTargetMs = 250;
inline constexpr std::size_t kPcmQueueLowWaterMs = 100;

}  // namespace nightwave
