#include "nightwave/streaming_player.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <new>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nightwave/gain_ramp.h"

namespace nightwave {
namespace {
bool reader(void* ctx, std::uint64_t offset, std::uint8_t* dst, std::size_t size) {
    return offset <= LONG_MAX && std::fseek(static_cast<FILE*>(ctx),
        static_cast<long>(offset), SEEK_SET) == 0 &&
        std::fread(dst, 1, size, static_cast<FILE*>(ctx)) == size;
}
std::int16_t sample(const std::uint8_t* p) {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(p[0]) |
                                    (static_cast<std::uint16_t>(p[1]) << 8));
}
void worst(std::atomic<std::uint32_t>& value, std::int64_t elapsed) {
    const auto bounded = static_cast<std::uint32_t>(std::min<std::int64_t>(
        std::max<std::int64_t>(0, elapsed), UINT32_MAX));
    if (bounded > value.load()) value.store(bounded);
}
void total(std::atomic<std::uint32_t>& value, std::uint64_t add) {
    value.store(static_cast<std::uint32_t>(std::min<std::uint64_t>(UINT32_MAX, value.load() + add)));
}
}
void StreamingPlayer::fail() { ++errors_; cancel_.store(true); }
bool StreamingPlayer::start(const char* path, I2sAudioSink& sink,
                            OutputPath output, std::uint8_t volume, std::uint32_t seek_ms) {
    if (running_.load() || !path || output == OutputPath::kMuted || seek_ms > 1800000) return false;
    // All workers from the previous session have published completion.
    mp3_.reset();
    errors_.store(0); underruns_.store(0); sd_us_.store(0); decoder_us_.store(0);
    rate_.store(0); paused_.store(false); cancel_.store(false);
    seek_ms_ = seek_ms; position_ms_.store(seek_ms);
    storage_stack_.store(0); decoder_stack_.store(0); audio_stack_.store(0);
    sd_bytes_.store(0); sd_reads_.store(0); sd_total_us_.store(0); decode_calls_.store(0); decode_total_us_.store(0);
    encoded_low_.store(UINT32_MAX); pcm_low_.store(UINT32_MAX);
    file_ = std::fopen(path, "rb");
    if (!file_) { ++errors_; return false; }
    auto reject = [this]() { std::fclose(file_); file_ = nullptr; ++errors_; return false; };
    if (std::fseek(file_, 0, SEEK_END) != 0) return reject();
    const long length = std::ftell(file_);
    if (length <= 0) return reject();
    std::rewind(file_);
    std::array<std::uint8_t, 4> magic{};
    if (std::fread(magic.data(), 1, magic.size(), file_) != magic.size()) return reject();
    if (std::memcmp(magic.data(), "RIFF", 4) == 0) {
        const auto parsed = parse_wav(reader, file_, static_cast<std::uint64_t>(length));
        if (parsed.status != WavParseStatus::kOk || parsed.info.data_size == 0) return reject();
        wav_ = parsed.info;
        rate_.store(wav_.format.sample_rate_hz);
        const auto frames = seek_ms * std::uint64_t(wav_.format.sample_rate_hz) / 1000;
        const auto skip = frames * wav_.block_align;
        if (skip >= wav_.data_size) return reject();
        bytes_remaining_ = wav_.data_size - skip;
        position_ms_.store(static_cast<std::uint32_t>(frames * 1000 / wav_.format.sample_rate_hz));
        if (wav_.data_offset + skip > LONG_MAX || std::fseek(file_,
            static_cast<long>(wav_.data_offset + skip), SEEK_SET) != 0) return reject();
    } else {
        mp3_.reset(new (std::nothrow) Mp3Decoder);
        if (!mp3_ || !mp3_->ready()) return reject();
        bytes_remaining_ = static_cast<std::uint64_t>(length);
        std::rewind(file_);
    }
    encoded_.reset(); pcm_.reset(); sink_ = &sink;
    output_.store(output); set_volume(volume);
    storage_done_.store(false); decoder_done_.store(false); running_.store(true);
    TaskHandle_t storage = nullptr, decoder = nullptr, audio = nullptr;
    if (xTaskCreate(storage_entry, "StorageTask", 4096, this, 5, &storage) != pdPASS ||
        xTaskCreate(decoder_entry, "DecoderTask", 8192, this, 6, &decoder) != pdPASS ||
        xTaskCreate(audio_entry, "AudioOutputTask", 4096, this, 8, &audio) != pdPASS) {
        if (storage) vTaskDelete(storage);
        if (decoder) vTaskDelete(decoder);
        if (audio) vTaskDelete(audio);
        sink.stop(); running_.store(false); return reject();
    }
    xTaskNotifyGive(storage); xTaskNotifyGive(decoder); xTaskNotifyGive(audio);
    return true;
}
bool StreamingPlayer::stop() {
    cancel_.store(true);
    for (int i = 0; i < 150 && running_.load(); ++i) vTaskDelay(pdMS_TO_TICKS(10));
    return !running_.load(); // Never free/reuse resources still owned by workers.
}
std::uint32_t StreamingPlayer::queue_depth_ms() const {
    const auto rate = rate_.load();
    return rate ? static_cast<std::uint32_t>(pcm_.size() * 1000ULL / rate) : 0;
}
StreamTelemetry StreamingPlayer::telemetry() const {
    return {static_cast<std::uint32_t>(encoded_.size()), queue_depth_ms(),
        underruns_.load(), errors_.load(), sd_us_.load(), decoder_us_.load(),
        storage_stack_.load(), decoder_stack_.load(), audio_stack_.load(),
        sd_bytes_.load(), sd_reads_.load(), sd_total_us_.load(), decode_calls_.load(), decode_total_us_.load(),
        encoded_low_.load() == UINT32_MAX ? 0 : encoded_low_.load(), pcm_low_.load() == UINT32_MAX ? 0 : pcm_low_.load()};
}
void StreamingPlayer::storage_entry(void* ctx) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); static_cast<StreamingPlayer*>(ctx)->storage_task();
}
void StreamingPlayer::decoder_entry(void* ctx) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); static_cast<StreamingPlayer*>(ctx)->decoder_task();
}
void StreamingPlayer::audio_entry(void* ctx) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); static_cast<StreamingPlayer*>(ctx)->audio_task();
}
void StreamingPlayer::storage_task() {
    while (!cancel_.load() && bytes_remaining_) {
        const auto wanted = static_cast<std::size_t>(std::min<std::uint64_t>(read_.size(), bytes_remaining_));
        const auto started = esp_timer_get_time();
        const auto got = std::fread(read_.data(), 1, wanted, file_);
        const auto elapsed = std::max<std::int64_t>(0, esp_timer_get_time() - started);
        worst(sd_us_, elapsed); total(sd_total_us_, static_cast<std::uint64_t>(elapsed));
        total(sd_bytes_, got); total(sd_reads_, 1);
        if (got != wanted) { fail(); break; }
        bytes_remaining_ -= got;
        for (std::size_t i = 0; i < got && !cancel_.load(); ++i) {
            while (!cancel_.load() && !encoded_.push(read_[i])) vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    std::fclose(file_); file_ = nullptr;
    storage_stack_.store(uxTaskGetStackHighWaterMark(nullptr));
    storage_done_.store(true); // Last access to session data by this worker.
    vTaskDelete(nullptr);
}
void StreamingPlayer::decoder_task() {
    std::size_t used = 0;
    std::uint64_t total_frames = 0, delivered = 0;
    while (!cancel_.load()) {
        if (total_frames && !storage_done_.load()) encoded_low_.store(std::min(encoded_low_.load(), static_cast<std::uint32_t>(encoded_.size())));
        while (used < staging_.size() && encoded_.pop(staging_[used])) ++used;
        // Acquire done before checking ring: no producer writes can follow it.
        const bool done = storage_done_.load();
        const bool eof = done && encoded_.empty();
        std::size_t consumed = 0;
        PcmBlock block{};
        const auto started = esp_timer_get_time();
        if (mp3_) {
            const auto result = mp3_->decode({staging_.data(), used}, eof);
            consumed = result.bytes_consumed;
            if (result.status == DecodeStatus::kFrameReady) block = result.pcm;
            else if (result.status == DecodeStatus::kEndOfStream) {
                if (!delivered) fail();
                break;
            } else if (result.status != DecodeStatus::kNeedInput) { fail(); break; }
        } else {
            consumed = used - used % wav_.block_align;
            if (consumed == 0 && eof) { if (used || !total_frames) fail(); break; }
        }
        const auto elapsed = std::max<std::int64_t>(0, esp_timer_get_time() - started);
        worst(decoder_us_, elapsed); total(decode_total_us_, static_cast<std::uint64_t>(elapsed)); total(decode_calls_, 1);
        if (consumed > used) { fail(); break; }
        if (block.frame_count) {
            if (!rate_.load()) rate_.store(block.format.sample_rate_hz);
            if (block.format.sample_rate_hz != rate_.load()) { fail(); break; }
            for (std::size_t i = 0; i < block.frame_count && !cancel_.load(); ++i) {
                ++total_frames;
                // Decode from the beginning to preserve the MP3 bit reservoir.
                // Bounded to 30 minutes; cancel checked every frame/sample.
                if (total_frames <= seek_ms_ * std::uint64_t(rate_.load()) / 1000) continue;
                const StereoFrame frame{block.interleaved_samples[i * 2], block.interleaved_samples[i * 2 + 1]};
                while (!cancel_.load() && !pcm_.push(frame)) vTaskDelay(pdMS_TO_TICKS(1));
                ++delivered;
            }
        } else if (!mp3_) {
            for (std::size_t i = 0; i < consumed && !cancel_.load(); i += wav_.block_align) {
                const StereoFrame frame{sample(staging_.data() + i), sample(staging_.data() + i +
                    (wav_.format.channel_count == 2 ? 2 : 0))};
                while (!cancel_.load() && !pcm_.push(frame)) vTaskDelay(pdMS_TO_TICKS(1));
                ++total_frames;
                ++delivered;
            }
        }
        if (consumed) {
            used -= consumed;
            std::memmove(staging_.data(), staging_.data() + consumed, used);
        } else {
            if (eof || used == staging_.size()) { fail(); break; }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    decoder_stack_.store(uxTaskGetStackHighWaterMark(nullptr));
    decoder_done_.store(true);
    vTaskDelete(nullptr);
}
void StreamingPlayer::audio_task() {
    while (!cancel_.load() && !decoder_done_.load() &&
           (!rate_.load() || pcm_.size() < rate_.load() / 10)) vTaskDelay(pdMS_TO_TICKS(2));
    const AudioFormat format{rate_.load(), 2, 16};
    if (!cancel_.load() && (!format.supported() ||
        sink_->configure(format) != AudioSinkStatus::kAccepted)) fail();
    GainRamp gain;
    std::array<StereoFrame, 256> samples{};
    OutputPath selected = OutputPath::kMuted;
    bool held = false;
    std::uint64_t accepted_frames = 0;
    const auto position_base = position_ms_.load();
    while (!cancel_.load()) {
        if (decoder_done_.load() && pcm_.empty()) break;
        const bool pause = paused_.load();
        if (pause && gain.value() == 0) {
            if (!held) {
                if (sink_->drain() != AudioSinkStatus::kAccepted) { fail(); break; }
                if (!sink_->select_output(OutputPath::kMuted)) { fail(); break; }
                selected = OutputPath::kMuted; held = true;
            }
            vTaskDelay(pdMS_TO_TICKS(5)); continue;
        }
        held = false;
        const auto requested = output_.load();
        const bool changing = requested != selected && selected != OutputPath::kMuted;
        if (requested != selected && gain.value() == 0) {
            // Fade old route to zero, drain accepted PCM, then break-before-make.
            if (selected != OutputPath::kMuted && sink_->drain() != AudioSinkStatus::kAccepted) { fail(); break; }
            if (!sink_->select_output(requested)) { fail(); break; }
            selected = requested; gain.reset();
        }
        std::size_t count = 0;
        const bool done = decoder_done_.load();
        if (!done) pcm_low_.store(std::min(pcm_low_.load(), static_cast<std::uint32_t>(pcm_.size())));
        for (; count < samples.size(); ++count) {
            if (!pcm_.pop(samples[count])) break;
            const auto level = gain.step(pause || changing ? 0 : percent_to_q15(volume_.load()));
            samples[count].left = scale_sample_q15(samples[count].left, level);
            samples[count].right = scale_sample_q15(samples[count].right, level);
        }
        const auto media_frames = count;
        if (!count) {
            if (done && pcm_.empty()) break;
            ++underruns_; samples.fill({}); count = samples.size();
            // Advance fade while filling an underrun with silence.
            for (std::size_t i = 0; i < count; ++i) gain.step(pause ? 0 : percent_to_q15(volume_.load()));
        }
        const PcmBlock block{reinterpret_cast<const std::int16_t*>(samples.data()), count, format, 0};
        if (sink_->write(block) != AudioSinkStatus::kAccepted) { fail(); break; }
        accepted_frames += media_frames;
        position_ms_.store(static_cast<std::uint32_t>(std::min<std::uint64_t>(UINT32_MAX,
            position_base + accepted_frames * 1000ULL / format.sample_rate_hz)));
    }
    if (!cancel_.load() && sink_->drain() != AudioSinkStatus::kAccepted) fail();
    sink_->select_output(OutputPath::kMuted);
    cancel_.store(true);
    while (!storage_done_.load() || !decoder_done_.load()) vTaskDelay(pdMS_TO_TICKS(1));
    sink_->stop();
    audio_stack_.store(uxTaskGetStackHighWaterMark(nullptr));
    const auto t = telemetry();
    ESP_LOGI("stream", "EXIT pcm_ms=%lu compressed=%lu underruns=%lu errors=%lu sd_us=%lu decode_us=%lu stack=%lu/%lu/%lu",
        static_cast<unsigned long>(t.pcm_ms), static_cast<unsigned long>(t.compressed_bytes),
        static_cast<unsigned long>(t.underruns), static_cast<unsigned long>(t.errors),
        static_cast<unsigned long>(t.sd_worst_us), static_cast<unsigned long>(t.decoder_worst_us),
        static_cast<unsigned long>(t.storage_stack_bytes), static_cast<unsigned long>(t.decoder_stack_bytes),
        static_cast<unsigned long>(t.audio_stack_bytes));
    running_.store(false); // Last session access; safe for owner to start another track.
    vTaskDelete(nullptr);
}
}  // namespace nightwave
