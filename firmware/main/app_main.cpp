#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#include "nightwave/audio_sink.h"
#include "nightwave/button_monitor.h"
#include "nightwave/diagnostics.h"
#include "nightwave/hardware_config.h"
#include "nightwave/storage.h"
#include "nightwave/streaming_player.h"
#include "nightwave/player_frontend.h"
#include "nightwave/engineering_report.h"
#include "nightwave/bluetooth_source.h"
#include "nightwave/power_hal.h"
#include "nightwave/bm83_at_codec.h"
#include "esp_heap_caps.h"

namespace {

constexpr char kTag[] = "nightwave";
constexpr float kPi = 3.14159265358979323846F;

nightwave::SdStorage g_storage;
nightwave::I2sAudioSink g_audio;
nightwave::StreamingPlayer g_player;
nightwave::ButtonMonitor g_buttons;
nightwave::PlayerFrontend g_frontend(g_storage, g_player, g_audio, g_buttons);
SemaphoreHandle_t g_commands = nullptr;
nightwave::UnavailableBluetooth g_bluetooth_backend;
nightwave::BluetoothSource g_bluetooth(g_bluetooth_backend);
nightwave::UnavailablePower g_power;

void ui_task(void*) {
    while (true) {
        if (xSemaphoreTake(g_commands, pdMS_TO_TICKS(10)) == pdTRUE) {
            g_frontend.tick(static_cast<std::uint32_t>(esp_timer_get_time() / 1000));
            xSemaphoreGive(g_commands);
        }
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

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
        "  play <speaker|line> <path>    buffered WAV/MP3 playback\n"
        "  pause | resume                pause/resume buffered playback\n"
        "  volume <0..100>               ramped digital volume\n"
        "  stop                          stop playback and mute outputs\n"
        "  status                        queue/underrun/error counters\n"
        "  mode <normal|shuffle|all|track> playback sequence mode\n"
        "  sleep <off|15|30|45|60|end>    ramp/stop/display sleep timer\n"
        "  seek <milliseconds>           WAV seek / indexed MP3 with safe fallback\n"
        "  last                          explicit saved-track resume\n"
        "  selftest                      JSON software/hardware availability\n"
        "  engineering                   bounded JSON library/seek/memory telemetry\n"
        "  help                          show this text\n\n");
}

void run_tone(nightwave::OutputPath output, float frequency_hz) {
    if (!std::isfinite(frequency_hz) || frequency_hz < 20.0F ||
        frequency_hz > 20000.0F) {
        ESP_LOGE(kTag, "tone frequency must be finite and between 20 and 20000 Hz");
        return;
    }
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
    if (g_audio.write(silence) != nightwave::AudioSinkStatus::kAccepted ||
        g_audio.drain() != nightwave::AudioSinkStatus::kAccepted) {
        ESP_LOGE(kTag, "tone silence/drain failed");
    }
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
    } else if (std::strcmp(command, "wav") == 0 || std::strcmp(command, "play") == 0) {
        const auto output = parse_output(strtok_r(nullptr, " \r\n", &context));
        const char* path = strtok_r(nullptr, "\r\n", &context);
        while (path != nullptr && *path == ' ') ++path;
        g_frontend.stopped(); // Console playback has no folder auto-advance queue.
        if (output == nightwave::OutputPath::kMuted || path == nullptr ||
            !g_storage.mount() || !g_player.start(path, g_audio, output, 8)) {
            ESP_LOGE(kTag, "usage: play <speaker|line> </sdcard/file.wav|mp3>");
        } else {
            g_frontend.track_started(path);
        }
    } else if (std::strcmp(command, "pause") == 0) {
        g_player.pause(true);
    } else if (std::strcmp(command, "resume") == 0) {
        g_player.pause(false);
    } else if (std::strcmp(command, "volume") == 0) {
        const char* value = strtok_r(nullptr, " \r\n", &context);
        std::uint32_t volume = 0;
        if (nightwave::parse_unsigned(value, 100, volume)) g_frontend.volume(static_cast<std::uint8_t>(volume), static_cast<std::uint32_t>(esp_timer_get_time() / 1000));
        else ESP_LOGW(kTag, "volume requires an integer 0..100");
    } else if (std::strcmp(command, "stop") == 0) {
        g_frontend.stopped();
        if (!g_player.stop()) {
            ESP_LOGW(kTag, "stop still pending; retain hardware and retry status/stop");
        }
    } else if (std::strcmp(command, "last") == 0) {
        if (!g_storage.mount() || !g_frontend.resume_saved()) ESP_LOGW(kTag, "saved-track resume unavailable/invalid");
    } else if (std::strcmp(command, "seek") == 0) {
        const char* value = strtok_r(nullptr, " \r\n", &context);
        std::uint32_t position = 0;
        if (!nightwave::parse_unsigned(value, 1800000, position) || !g_frontend.seek_current(position)) ESP_LOGW(kTag, "seek requires 0..1800000 ms and an active queue track");
    } else if (std::strcmp(command, "mode") == 0) {
        const char* value = strtok_r(nullptr, " \r\n", &context);
        if (value) {
            const char* names[] = {"normal", "shuffle", "all", "track"};
            for (unsigned i = 0; i < 4; ++i) if (!std::strcmp(value, names[i])) g_frontend.mode(static_cast<nightwave::PlaybackMode>(i), static_cast<std::uint32_t>(esp_timer_get_time() / 1000));
        }
    } else if (std::strcmp(command, "sleep") == 0) {
        const char* value = strtok_r(nullptr, " \r\n", &context);
        if (value) {
            const char* names[] = {"off", "15", "30", "45", "60", "end"};
            for (unsigned i = 0; i < 6; ++i) if (!std::strcmp(value, names[i])) g_frontend.sleep_timer(static_cast<nightwave::SleepMode>(i), static_cast<std::uint32_t>(esp_timer_get_time() / 1000));
        }
    } else if (std::strcmp(command, "selftest") == 0) {
        std::printf("{\"type\":\"nightwave_selftest\",\"schema\":1,\"gpio_map_unique\":%s,\"storage_mounted\":%s,\"bluetooth\":\"BM83SM1-00TA_backend_unqualified\",\"battery\":\"unmeasured\",\"acoustic_output\":\"unverified\",\"physical_pass\":false}\n",
            nightwave::hardware::pins_are_unique() ? "true" : "false", g_storage.mount() ? "true" : "false");
    } else if (std::strcmp(command, "engineering") == 0) {
        nightwave::EngineeringReport report;
        report.stream = g_player.telemetry(); report.position_ms = g_player.position_ms();
        report.catalog_tracks = g_frontend.catalog_tracks(); report.query_reads = g_frontend.catalog_query_reads();
        report.catalog_building = g_frontend.catalog_building(); report.catalog_fast = g_frontend.catalog_fast_lookup();
        report.heap_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        report.heap_min = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        report.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        std::array<char, 1024> json{};
        if (nightwave::format_engineering_report(report, json.data(), json.size())) std::puts(json.data());
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
        const auto t = g_player.telemetry();
        std::printf("{\"type\":\"nightwave_stream\",\"schema\":1,\"position_ms\":%lu,\"pcm_ms\":%lu,\"encoded_bytes\":%lu,\"underruns\":%lu,\"errors\":%lu,\"sd_worst_us\":%lu,\"decode_worst_us\":%lu,\"bluetooth\":\"BM83_backend_unqualified\",\"battery\":null}\n",
            static_cast<unsigned long>(g_player.position_ms()), static_cast<unsigned long>(t.pcm_ms), static_cast<unsigned long>(t.compressed_bytes),
            static_cast<unsigned long>(t.underruns), static_cast<unsigned long>(t.errors), static_cast<unsigned long>(t.sd_worst_us), static_cast<unsigned long>(t.decoder_worst_us));
        std::printf("{\"type\":\"nightwave_performance\",\"schema\":1,\"sd_bytes\":%lu,\"sd_reads\":%lu,\"sd_total_us\":%lu,\"decode_calls\":%lu,\"decode_total_us\":%lu,\"encoded_low_bytes\":%lu,\"pcm_low_frames\":%lu,\"stack_min_bytes\":[%lu,%lu,%lu]}\n",
            static_cast<unsigned long>(t.sd_bytes), static_cast<unsigned long>(t.sd_reads), static_cast<unsigned long>(t.sd_total_us),
            static_cast<unsigned long>(t.decode_calls), static_cast<unsigned long>(t.decode_total_us), static_cast<unsigned long>(t.encoded_low_bytes),
            static_cast<unsigned long>(t.pcm_low_frames), static_cast<unsigned long>(t.storage_stack_bytes), static_cast<unsigned long>(t.decoder_stack_bytes), static_cast<unsigned long>(t.audio_stack_bytes));
        ESP_LOGI(kTag, "STREAM compressed=%lu sd_worst_us=%lu decode_worst_us=%lu stack_min_bytes=%lu/%lu/%lu",
            static_cast<unsigned long>(t.compressed_bytes), static_cast<unsigned long>(t.sd_worst_us),
            static_cast<unsigned long>(t.decoder_worst_us), static_cast<unsigned long>(t.storage_stack_bytes),
            static_cast<unsigned long>(t.decoder_stack_bytes), static_cast<unsigned long>(t.audio_stack_bytes));
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
    g_commands = xSemaphoreCreateMutex();
    if (!g_commands) { ESP_LOGE(kTag, "command mutex unavailable"); return; }
    g_frontend.initialize();
    if (xTaskCreate(ui_task, "UiTask", 6144, nullptr, 3, nullptr) != pdPASS)
        ESP_LOGE(kTag, "UI task unavailable; console remains active");

    std::array<char, 256> line{};
    bool discard_line = false;
    while (true) {
        std::printf("nightwave> ");
        std::fflush(stdout);
        if (std::fgets(line.data(), line.size(), stdin) != nullptr) {
            const bool terminated = std::strchr(line.data(), '\n') != nullptr;
            if (discard_line) { discard_line = !terminated; continue; }
            if (!terminated) { discard_line = true; ESP_LOGW(kTag, "overlong/incomplete command discarded"); continue; }
            xSemaphoreTake(g_commands, portMAX_DELAY);
            execute_command(line.data());
            xSemaphoreGive(g_commands);
        } else {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
