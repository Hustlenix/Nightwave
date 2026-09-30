#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nightwave/audio_sink.h"
#include "nightwave/button_monitor.h"
#include "nightwave/diagnostics.h"
#include "nightwave/hardware_config.h"
#include "nightwave/storage.h"
#include "nightwave/wav_player.h"

namespace {

constexpr char kTag[] = "nightwave";
constexpr float kPi = 3.14159265358979323846F;

nightwave::SdStorage g_storage;
nightwave::I2sAudioSink g_audio;
nightwave::WavPlayer g_player;
nightwave::ButtonMonitor g_buttons;

static_assert(nightwave::hardware::pins_are_unique(),
              "Nightwave GPIO assignments must be unique");
static_assert(nightwave::hardware::pins_avoid_restricted_set(),
              "Nightwave GPIO assignments overlap a restricted ESP32-S3 pin");

nightwave::OutputPath parse_output(const char* value) {
    if (value != nullptr && std::strcmp(value, "speaker") == 0) {
        return nightwave::OutputPath::kSpeaker;
    }
    if (value != nullptr && std::strcmp(value, "line") == 0) {
        return nightwave::OutputPath::kLine;
    }
    return nightwave::OutputPath::kMuted;
}

void print_help() {
    std::printf(
        "\nNightwave diagnostics\n"
        "  board                         boot/heap/PSRAM report\n"
        "  sd                            mount + enumerate WAV/MP3\n"
        "  bench [absolute-path]         SD average/worst read report\n"
        "  tone <speaker|line> [hz]      2 s low-level diagnostic tone\n"
        "  wav <speaker|line> <path>     buffered PCM WAV playback\n"
        "  stop                          stop playback and mute outputs\n"
        "  status                        queue/underrun/error counters\n"
        "  help                          show this text\n\n");
}

void run_tone(nightwave::OutputPath output, float frequency_hz) {
    if (output == nightwave::OutputPath::kMuted || g_player.playing()) {
        ESP_LOGE(kTag, "tone requires speaker|line and idle playback");
        return;
    }
    nightwave::AudioFormat format{44100, 2, 16};
    if (g_audio.configure(format) != nightwave::AudioSinkStatus::kAccepted) {
        ESP_LOGE(kTag, "tone I2S setup failed");
        return;
    }
    if (!g_audio.select_output(output)) {
        g_audio.stop();
        return;
    }
    constexpr std::size_t kFrames = 256;
    constexpr std::int16_t kPeak = 1200;
    std::array<std::int16_t, kFrames * 2> pcm{};
    std::uint64_t frame_index = 0;
    const std::uint64_t total_frames = format.sample_rate_hz * 2ULL;
    ESP_LOGI(kTag, "TONE START hz=%.1f peak=%d duration_ms=2000", frequency_hz,
             kPeak);
    while (frame_index < total_frames) {
        const auto frames = static_cast<std::size_t>(
            std::min<std::uint64_t>(kFrames, total_frames - frame_index));
        for (std::size_t index = 0; index < frames; ++index) {
            const float phase = 2.0F * kPi * frequency_hz *
                                static_cast<float>(frame_index + index) /
                                static_cast<float>(format.sample_rate_hz);
            float envelope = 1.0F;
            const auto absolute = frame_index + index;
            constexpr std::uint64_t kRampFrames = 2205;
            if (absolute < kRampFrames) {
                envelope = static_cast<float>(absolute) / kRampFrames;
            } else if (total_frames - absolute < kRampFrames) {
                envelope = static_cast<float>(total_frames - absolute) /
                           kRampFrames;
            }
            const auto sample = static_cast<std::int16_t>(
                std::sin(phase) * envelope * kPeak);
            pcm[index * 2] = sample;
            pcm[index * 2 + 1] = sample;
        }
        nightwave::PcmBlock block{pcm.data(), frames, format, frame_index};
        if (g_audio.write(block) != nightwave::AudioSinkStatus::kAccepted) {
            ESP_LOGE(kTag, "tone write failed");
            break;
        }
        frame_index += frames;
    }
    pcm.fill(0);
    nightwave::PcmBlock silence{pcm.data(), kFrames, format, frame_index};
    g_audio.write(silence);
    g_audio.stop();
    ESP_LOGI(kTag, "TONE STOP output=muted");
}

void execute_command(char* line) {
    char* context = nullptr;
    const char* command = strtok_r(line, " \r\n", &context);
    if (command == nullptr) return;
    if (std::strcmp(command, "help") == 0) {
        print_help();
    } else if (std::strcmp(command, "board") == 0) {
        nightwave::log_boot_diagnostics();
    } else if (std::strcmp(command, "sd") == 0) {
        if (g_storage.mount()) g_storage.enumerate_supported_files();
    } else if (std::strcmp(command, "bench") == 0) {
        const char* path = strtok_r(nullptr, " \r\n", &context);
        if (path == nullptr) path = "/sdcard/02_left_right_stereo_44100.wav";
        if (g_storage.mount()) g_storage.benchmark(path);
    } else if (std::strcmp(command, "tone") == 0) {
        const auto output = parse_output(strtok_r(nullptr, " \r\n", &context));
        const char* frequency = strtok_r(nullptr, " \r\n", &context);
        run_tone(output,
                 frequency == nullptr ? 440.0F : std::strtof(frequency, nullptr));
    } else if (std::strcmp(command, "wav") == 0) {
        const auto output = parse_output(strtok_r(nullptr, " \r\n", &context));
        const char* path = strtok_r(nullptr, "\r\n", &context);
        while (path != nullptr && *path == ' ') ++path;
        if (output == nightwave::OutputPath::kMuted || path == nullptr ||
            !g_storage.mount() || !g_player.start(path, g_audio, output, 8)) {
            ESP_LOGE(kTag, "usage: wav <speaker|line> </sdcard/file.wav>");
        }
    } else if (std::strcmp(command, "stop") == 0) {
        g_player.stop();
        g_audio.stop();
    } else if (std::strcmp(command, "status") == 0) {
        const auto status = nightwave::capture_diagnostics(
            g_player.underruns(), g_storage.errors(), g_player.errors(),
            g_player.queue_depth_ms());
        ESP_LOGI(kTag,
                 "STATUS playing=%d queue_ms=%lu underruns=%lu storage_errors=%lu decode_errors=%lu heap_min=%lu",
                 g_player.playing(),
                 static_cast<unsigned long>(status.pcm_queue_depth_ms),
                 static_cast<unsigned long>(status.audio_underruns),
                 static_cast<unsigned long>(status.storage_errors),
                 static_cast<unsigned long>(status.decode_errors),
                 static_cast<unsigned long>(status.minimum_free_heap_bytes));
    } else {
        ESP_LOGW(kTag, "unknown command: %s", command);
        print_help();
    }
}

}  // namespace

extern "C" void app_main() {
    g_audio.initialize_safe_outputs();
    nightwave::log_boot_diagnostics();
    if (!g_buttons.start()) ESP_LOGE(kTag, "button monitor failed to start");
    if (!g_storage.mount()) {
        ESP_LOGW(kTag, "SD unavailable; console and button diagnostics remain active");
    } else {
        g_storage.enumerate_supported_files();
    }
    print_help();

    std::array<char, 256> line{};
    while (true) {
        std::printf("nightwave> ");
        std::fflush(stdout);
        if (std::fgets(line.data(), line.size(), stdin) != nullptr) {
            execute_command(line.data());
        } else {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
