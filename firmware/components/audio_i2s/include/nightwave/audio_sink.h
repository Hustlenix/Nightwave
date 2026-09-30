#pragma once

#include <cstdint>

#include "nightwave/audio_types.h"

namespace nightwave {

enum class AudioSinkStatus : std::uint8_t { kAccepted, kWouldBlock, kNotReady, kFault };
enum class OutputPath : std::uint8_t { kMuted, kSpeaker, kLine };

class AudioSink {
  public:
    virtual ~AudioSink() = default;
    virtual AudioSinkStatus configure(const AudioFormat& format) = 0;
    virtual AudioSinkStatus write(const PcmBlock& block) = 0;
    virtual void stop() = 0;
};

class I2sAudioSink final : public AudioSink {
  public:
    bool initialize_safe_outputs();
    AudioSinkStatus configure(const AudioFormat& format) override;
    AudioSinkStatus write(const PcmBlock& block) override;
    void stop() override;
    bool select_output(OutputPath output);
    bool ready() const { return channel_ != nullptr; }

  private:
    void* channel_{nullptr};
    AudioFormat format_{};
    OutputPath output_{OutputPath::kMuted};
};

}  // namespace nightwave
