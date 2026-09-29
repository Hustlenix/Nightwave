#pragma once

#include <array>
#include <cstdint>

namespace nightwave {

enum class AudioCodec : std::uint8_t { kUnknown, kMp3, kWavPcm };

struct LibraryEntry {
    std::uint32_t id{0};
    std::array<char, 96> relative_path{};
    AudioCodec codec{AudioCodec::kUnknown};
    std::uint32_t duration_ms{0};
    std::uint64_t file_size_bytes{0};
};

}  // namespace nightwave
