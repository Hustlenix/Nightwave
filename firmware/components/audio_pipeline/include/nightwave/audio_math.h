#pragma once

#include <cstdint>
#include <limits>

namespace nightwave {

inline constexpr std::int16_t stereo_to_mono(std::int16_t left,
                                              std::int16_t right) {
    return static_cast<std::int16_t>((static_cast<std::int32_t>(left) +
                                      static_cast<std::int32_t>(right)) /
                                     2);
}

inline constexpr std::int16_t scale_sample_q15(std::int16_t sample,
                                                std::uint16_t gain_q15) {
    const auto scaled = (static_cast<std::int32_t>(sample) * gain_q15) >> 15;
    if (scaled > std::numeric_limits<std::int16_t>::max()) {
        return std::numeric_limits<std::int16_t>::max();
    }
    if (scaled < std::numeric_limits<std::int16_t>::min()) {
        return std::numeric_limits<std::int16_t>::min();
    }
    return static_cast<std::int16_t>(scaled);
}

inline constexpr std::uint16_t percent_to_q15(std::uint8_t percent) {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(percent > 100 ? 100 : percent) * 32767U) /
        100U);
}

}  // namespace nightwave
