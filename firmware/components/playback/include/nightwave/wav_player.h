#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>

#include "nightwave/audio_sink.h"
#include "nightwave/pcm_ring_buffer.h"
#include "nightwave/wav_parser.h"

namespace nightwave {

class WavPlayer {
  public:
    bool start(const char* path, I2sAudioSink& sink, OutputPath output,
               std::uint8_t volume_percent);
    void stop();
    bool playing() const { return playing_.load(); }
    std::uint32_t underruns() const { return underruns_.load(); }
    std::uint32_t errors() const { return errors_.load(); }
    std::uint32_t queue_depth_ms() const;

  private:
    static constexpr std::size_t kRingSlots = 16385;
    static constexpr std::size_t kReadBytes = 4096;
    static constexpr std::size_t kDmaFrames = 256;

    static void storage_task_entry(void* context);
    static void audio_task_entry(void* context);
    void storage_task();
    void audio_task();

    std::array<StereoFrame, kRingSlots> ring_storage_{};
    PcmRingBuffer<StereoFrame> ring_{ring_storage_.data(), ring_storage_.size()};
    WavInfo info_{};
    std::FILE* file_{nullptr};
    I2sAudioSink* sink_{nullptr};
    OutputPath output_{OutputPath::kMuted};
    std::uint16_t gain_q15_{0};
    std::atomic<bool> source_done_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> playing_{false};
    std::atomic<std::uint32_t> underruns_{0};
    std::atomic<std::uint32_t> errors_{0};
    void* storage_task_handle_{nullptr};
    void* audio_task_handle_{nullptr};
};

}  // namespace nightwave
