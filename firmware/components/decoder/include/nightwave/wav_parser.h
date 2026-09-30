#pragma once

#include <cstddef>
#include <cstdint>

#include "nightwave/audio_types.h"

namespace nightwave {

using WavReadCallback = bool (*)(void* context, std::uint64_t offset,
                                 std::uint8_t* destination, std::size_t length);

enum class WavParseStatus : std::uint8_t {
    kOk,
    kIoError,
    kTruncated,
    kNotRiffWave,
    kMissingFormat,
    kMissingData,
    kUnsupportedCodec,
    kUnsupportedFormat,
    kMalformedChunk,
};

struct WavInfo {
    AudioFormat format{};
    std::uint16_t block_align{0};
    std::uint32_t byte_rate{0};
    std::uint64_t data_offset{0};
    std::uint64_t data_size{0};
    std::uint32_t unknown_chunk_count{0};
};

struct WavParseResult {
    WavParseStatus status{WavParseStatus::kIoError};
    WavInfo info{};
};

WavParseResult parse_wav(WavReadCallback reader, void* context,
                         std::uint64_t file_size);
const char* wav_status_name(WavParseStatus status);

}  // namespace nightwave
