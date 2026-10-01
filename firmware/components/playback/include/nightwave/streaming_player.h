#pragma once
#include <array>
#include <atomic>
#include <cstdio>
#include <memory>
#include "nightwave/audio_sink.h"
#include "nightwave/mp3_decoder.h"
#include "nightwave/pcm_ring_buffer.h"
#include "nightwave/wav_parser.h"

namespace nightwave {
struct StreamTelemetry {
    std::uint32_t compressed_bytes, pcm_ms, underruns, errors;
    std::uint32_t sd_worst_us, decoder_worst_us;
    std::uint32_t storage_stack_bytes, decoder_stack_bytes, audio_stack_bytes;
};
// Commands have one serialized owner. Worker-facing controls are atomic.
// Object must outlive workers, including after a timed-out stop().
class StreamingPlayer {
 public:
    bool start(const char* path, I2sAudioSink& sink, OutputPath output,
               std::uint8_t volume);
    bool stop();
    bool playing() const { return running_.load(); }
    bool paused() const { return paused_.load(); }
    void pause(bool value) { paused_.store(value); }
    void set_volume(std::uint8_t value) { volume_.store(value > 100 ? 100 : value); }
    void set_output(OutputPath value) { output_.store(value); }
    std::uint32_t underruns() const { return underruns_.load(); }
    std::uint32_t errors() const { return errors_.load(); }
    std::uint32_t queue_depth_ms() const;
    StreamTelemetry telemetry() const;
 private:
    static void storage_entry(void*);
    static void decoder_entry(void*);
    static void audio_entry(void*);
    void storage_task();
    void decoder_task();
    void audio_task();
    void fail();
    std::array<std::uint8_t, 32769> encoded_storage_{};
    std::array<StereoFrame, 16385> pcm_storage_{};
    std::array<std::uint8_t, 4096> read_{};
    std::array<std::uint8_t, 4096> staging_{};
    PcmRingBuffer<std::uint8_t> encoded_{encoded_storage_.data(), encoded_storage_.size()};
    PcmRingBuffer<StereoFrame> pcm_{pcm_storage_.data(), pcm_storage_.size()};
    std::unique_ptr<Mp3Decoder> mp3_{};
    WavInfo wav_{};
    std::uint64_t bytes_remaining_{0};
    std::FILE* file_{nullptr};
    I2sAudioSink* sink_{nullptr};
    std::atomic<bool> running_{false}, cancel_{false}, paused_{false};
    std::atomic<bool> storage_done_{false}, decoder_done_{false};
    std::atomic<std::uint8_t> volume_{8};
    std::atomic<OutputPath> output_{OutputPath::kMuted};
    std::atomic<std::uint32_t> rate_{0}, underruns_{0}, errors_{0};
    std::atomic<std::uint32_t> sd_us_{0}, decoder_us_{0};
    std::atomic<std::uint32_t> storage_stack_{0}, decoder_stack_{0}, audio_stack_{0};
};
}  // namespace nightwave
