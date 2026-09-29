#pragma once

#include <cstddef>
#include <cstdint>

#include "nightwave/audio_types.h"

namespace nightwave {

enum class DecodeStatus : std::uint8_t {
    kFrameReady,
    kNeedInput,
    kEndOfStream,
    kUnsupported,
    kMalformedStream,
    kInternalError,
};

struct EncodedBytes {
    const std::uint8_t* data{nullptr};
    std::size_t size{0};
};

struct DecodeResult {
    DecodeStatus status{DecodeStatus::kNeedInput};
    std::size_t bytes_consumed{0};
    PcmBlock pcm{};
};

class AudioDecoder {
  public:
    virtual ~AudioDecoder() = default;
    virtual DecodeResult decode(EncodedBytes input) = 0;
    virtual void reset() = 0;
};

}  // namespace nightwave
