#pragma once

#include <cstdint>

#include "nightwave/audio_types.h"

namespace nightwave {

enum class AudioSinkStatus : std::uint8_t { kAccepted, kWouldBlock, kNotReady, kFault };

class AudioSink {
  public:
    virtual ~AudioSink() = default;
    virtual AudioSinkStatus configure(const AudioFormat& format) = 0;
    virtual AudioSinkStatus write(const PcmBlock& block) = 0;
    virtual void stop() = 0;
};

}  // namespace nightwave
