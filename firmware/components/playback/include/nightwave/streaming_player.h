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
    std::uint32_t sd_bytes, sd_reads, sd_total_us, decode_calls, decode_total_us;
    std::uint32_t encoded_low_bytes, pcm_low_frames;
    std::uint32_t duration_ms, seek_base_samples, route_changes;
    bool indexed_seek;
};
// Commands have one serialized owner. Worker-facing controls are atomic.
// Object must outlive workers, including after a timed-out stop().
class StreamingPlayer {
 public:
    bool start(const char* path, I2sAudioSink& sink, OutputPath output,
               std::uint8_t volume, std::uint32_t seek_ms = 0, const char* index_root = "/sdcard");
    bool stop();
    bool playing() const { return running_.load(); }
    bool paused() const { return paused_.load(); }
    OutputPath requested_output() const { return output_.load(); }
    std::uint8_t requested_volume() const { return volume_.load(); }
    std::uint32_t playback_rate_hz() const { return rate_.load(); }
    // Boot-lifetime modulo-32-bit counters: track changes/seeks do not reset them.
    std::uint32_t media_frames_accepted() const { return media_frames_accepted_.load(); }
    std::uint32_t lifetime_errors() const { return lifetime_errors_.load(); }
    std::uint32_t lifetime_underruns() const { return lifetime_underruns_.load(); }
    // Media frames accepted by I2S, excluding underrun silence. DMA lead is
    // bounded by the sink queue; this is not an acoustic measurement.
    std::uint32_t position_ms() const { return position_ms_.load(); }
    std::uint32_t duration_ms() const { return duration_ms_.load(); }
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
    void mark_error() { ++errors_; ++lifetime_errors_; }
    std::array<std::uint8_t, 32769> encoded_storage_{};
    std::array<StereoFrame, 16385> pcm_storage_{};
    std::array<std::uint8_t, 4096> read_{};
    std::array<std::uint8_t, 4096> staging_{};
    PcmRingBuffer<std::uint8_t> encoded_{encoded_storage_.data(), encoded_storage_.size()};
    PcmRingBuffer<StereoFrame> pcm_{pcm_storage_.data(), pcm_storage_.size()};
    std::unique_ptr<Mp3Decoder> mp3_{};
    WavInfo wav_{};
    std::uint64_t bytes_remaining_{0};
    std::uint32_t seek_ms_{0};
    std::uint32_t decode_base_samples_{0};
    std::atomic<std::uint32_t> duration_ms_{0}, route_changes_{0};
    std::atomic<bool> indexed_seek_{false};
    std::FILE* file_{nullptr};
    I2sAudioSink* sink_{nullptr};
    std::atomic<bool> running_{false}, cancel_{false}, paused_{false};
    std::atomic<bool> storage_done_{false}, decoder_done_{false};
    std::atomic<std::uint8_t> volume_{8};
    std::atomic<OutputPath> output_{OutputPath::kMuted};
    std::atomic<std::uint32_t> rate_{0}, underruns_{0}, errors_{0};
    std::atomic<std::uint32_t> media_frames_accepted_{0}, lifetime_errors_{0}, lifetime_underruns_{0};
    std::atomic<std::uint32_t> position_ms_{0};
    std::atomic<std::uint32_t> sd_us_{0}, decoder_us_{0};
    std::atomic<std::uint32_t> storage_stack_{0}, decoder_stack_{0}, audio_stack_{0};
    std::atomic<std::uint32_t> sd_bytes_{0}, sd_reads_{0}, sd_total_us_{0}, decode_calls_{0}, decode_total_us_{0};
    std::atomic<std::uint32_t> encoded_low_{UINT32_MAX}, pcm_low_{UINT32_MAX};
};
}  // namespace nightwave
