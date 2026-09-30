#include "nightwave/wav_player.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nightwave/audio_math.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "wav_player";

bool file_reader(void* context, std::uint64_t offset, std::uint8_t* destination,
                 std::size_t length) {
    auto* file = static_cast<std::FILE*>(context);
    if (offset > static_cast<std::uint64_t>(LONG_MAX) ||
        std::fseek(file, static_cast<long>(offset), SEEK_SET) != 0) {
        return false;
    }
    return std::fread(destination, 1, length, file) == length;
}

std::uint64_t file_size(std::FILE* file) {
    if (std::fseek(file, 0, SEEK_END) != 0) return 0;
    const long end = std::ftell(file);
    std::rewind(file);
    return end < 0 ? 0 : static_cast<std::uint64_t>(end);
}

std::int16_t read_le_i16(const std::uint8_t* value) {
    const auto raw = static_cast<std::uint16_t>(value[0]) |
                     (static_cast<std::uint16_t>(value[1]) << 8U);
    return static_cast<std::int16_t>(raw);
}
}  // namespace

bool WavPlayer::start(const char* path, I2sAudioSink& sink, OutputPath output,
                      std::uint8_t volume_percent) {
    if (playing() || path == nullptr || output == OutputPath::kMuted) return false;
    file_ = std::fopen(path, "rb");
    if (file_ == nullptr) {
        ESP_LOGE(kTag, "open failed: %s", path);
        ++errors_;
        return false;
    }
    const auto parsed = parse_wav(file_reader, file_, file_size(file_));
    if (parsed.status != WavParseStatus::kOk) {
        ESP_LOGE(kTag, "WAV rejected: %s", wav_status_name(parsed.status));
        std::fclose(file_);
        file_ = nullptr;
        ++errors_;
        return false;
    }
    info_ = parsed.info;
    if (std::fseek(file_, static_cast<long>(info_.data_offset), SEEK_SET) != 0) {
        std::fclose(file_);
        file_ = nullptr;
        ++errors_;
        return false;
    }

    AudioFormat output_format = info_.format;
    output_format.channel_count = 2;
    if (sink.configure(output_format) != AudioSinkStatus::kAccepted) {
        std::fclose(file_);
        file_ = nullptr;
        ++errors_;
        return false;
    }

    ring_.reset();
    sink_ = &sink;
    output_ = output;
    gain_q15_ = percent_to_q15(volume_percent);
    source_done_.store(false);
    stop_requested_.store(false);
    underruns_.store(0);
    errors_.store(0);
    playing_.store(true);

    TaskHandle_t storage_handle = nullptr;
    TaskHandle_t audio_handle = nullptr;
    if (xTaskCreate(storage_task_entry, "StorageTask", 4096, this, 5,
                    &storage_handle) != pdPASS ||
        xTaskCreate(audio_task_entry, "AudioOutputTask", 4096, this, 8,
                    &audio_handle) != pdPASS) {
        stop_requested_.store(true);
        if (storage_handle != nullptr) vTaskDelete(storage_handle);
        if (audio_handle != nullptr) vTaskDelete(audio_handle);
        std::fclose(file_);
        file_ = nullptr;
        sink.stop();
        playing_.store(false);
        ++errors_;
        return false;
    }
    // Workers wait for notification so partial task creation cannot race
    // rollback (including fclose and I2S teardown).
    xTaskNotifyGive(storage_handle);
    xTaskNotifyGive(audio_handle);
    ESP_LOGI(kTag,
             "WAV START rate=%lu source_channels=%u data_bytes=%llu unknown_chunks=%lu volume=%u",
             static_cast<unsigned long>(info_.format.sample_rate_hz),
             info_.format.channel_count,
             static_cast<unsigned long long>(info_.data_size),
             static_cast<unsigned long>(info_.unknown_chunk_count), volume_percent);
    return true;
}

bool WavPlayer::stop() {
    stop_requested_.store(true);
    for (int attempt = 0; attempt < 100 && playing(); ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (playing()) {
        ESP_LOGE(kTag, "STOP PENDING: worker cleanup incomplete; track restart refused");
        return false;
    }
    return true;
}

std::uint32_t WavPlayer::queue_depth_ms() const {
    if (info_.format.sample_rate_hz == 0) return 0;
    return static_cast<std::uint32_t>(
        ring_.size() * 1000ULL / info_.format.sample_rate_hz);
}

void WavPlayer::storage_task_entry(void* context) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    static_cast<WavPlayer*>(context)->storage_task();
}

void WavPlayer::audio_task_entry(void* context) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    static_cast<WavPlayer*>(context)->audio_task();
}

void WavPlayer::storage_task() {
    auto& bytes = read_buffer_;
    std::uint64_t remaining = info_.data_size;
    while (!stop_requested_.load() && remaining > 0) {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<std::uint64_t>(bytes.size(), remaining));
        const std::size_t aligned = wanted - (wanted % info_.block_align);
        if (aligned == 0) break;
        const std::size_t received = std::fread(bytes.data(), 1, aligned, file_);
        if (received != aligned) {
            ++errors_;
            stop_requested_.store(true);
            ESP_LOGE(kTag, "WAV read incomplete: requested=%u received=%u",
                     static_cast<unsigned>(aligned), static_cast<unsigned>(received));
            break;
        }
        remaining -= received;
        for (std::size_t offset = 0; offset + info_.block_align <= received;
             offset += info_.block_align) {
            StereoFrame frame{};
            frame.left = scale_sample_q15(read_le_i16(bytes.data() + offset),
                                          gain_q15_);
            frame.right = info_.format.channel_count == 1
                              ? frame.left
                              : scale_sample_q15(
                                    read_le_i16(bytes.data() + offset + 2),
                                    gain_q15_);
            while (!stop_requested_.load() && !ring_.push(frame)) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
    }
    std::fclose(file_);
    file_ = nullptr;
    ESP_LOGI(kTag, "STORAGE EXIT stack_free_min_bytes=%u",
             static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
    // Last publication: after this store the producer never touches track data.
    source_done_.store(true);
    vTaskDelete(nullptr);
}

void WavPlayer::audio_task() {
    const std::size_t prefill_frames = std::min<std::size_t>(
        ring_.capacity() / 2,
        info_.format.sample_rate_hz * kPcmQueueLowWaterMs / 1000U);
    while (!stop_requested_.load() && !source_done_.load() &&
           ring_.size() < prefill_frames) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (!stop_requested_.load()) sink_->select_output(output_);

    std::array<StereoFrame, kDmaFrames> output{};
    while (!stop_requested_.load() && (!source_done_.load() || !ring_.empty())) {
        std::size_t frames = 0;
        for (; frames < output.size(); ++frames) {
            if (!ring_.pop(output[frames])) break;
        }
        if (frames == 0) {
            if (source_done_.load()) break;
            ++underruns_;
            ESP_LOGW(kTag, "AUDIO UNDERRUN count=%lu queue_ms=%lu",
                     static_cast<unsigned long>(underruns_.load()),
                     static_cast<unsigned long>(queue_depth_ms()));
            output.fill(StereoFrame{});
            frames = output.size();
        }
        PcmBlock block{};
        block.interleaved_samples =
            reinterpret_cast<const std::int16_t*>(output.data());
        block.frame_count = frames;
        block.format = info_.format;
        block.format.channel_count = 2;
        if (sink_->write(block) != AudioSinkStatus::kAccepted) {
            ++errors_;
            // Stop the producer before it can block forever on a full ring.
            stop_requested_.store(true);
            break;
        }
    }
    // The audio worker owns I2S teardown. Do not advertise idle until the
    // producer has closed the file and stopped accessing the ring/config.
    while (!source_done_.load()) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    sink_->stop();
    ESP_LOGI(kTag, "WAV STOP underruns=%lu errors=%lu",
             static_cast<unsigned long>(underruns_.load()),
             static_cast<unsigned long>(errors_.load()));
    ESP_LOGI(kTag, "AUDIO EXIT stack_free_min_bytes=%u",
             static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
    playing_.store(false);
    vTaskDelete(nullptr);
}

}  // namespace nightwave
