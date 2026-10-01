#include "nightwave/diagnostics.h"
#include <cstdio>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_idf_version.h"
#include "esp_flash.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "diagnostics";
void json_string(const char* text) {
    std::putchar('"');
    for (const auto* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (*p == '"' || *p == '\\') { std::putchar('\\'); std::putchar(*p); }
        else if (*p < 32) std::printf("\\u%04x", *p);
        else std::putchar(*p);
    }
    std::putchar('"');
}
}

void log_boot_diagnostics() {
    esp_chip_info_t chip{};
    esp_chip_info(&chip);
    const esp_app_desc_t* app = esp_app_get_description();
    const auto psram_bytes = esp_psram_is_initialized() ? esp_psram_get_size() : 0;
    std::uint32_t flash_bytes = 0;
    if (esp_flash_get_size(nullptr, &flash_bytes) != ESP_OK) flash_bytes = 0;

    ESP_LOGI(kTag, "BOOT OK");
    ESP_LOGI(kTag, "project=%s version=%s idf=%s", app->project_name,
             app->version, esp_get_idf_version());
    ESP_LOGI(kTag,
             "target=ESP32-S3 revision=%d cores=%d flash=%luMB reset_reason=%d",
             chip.revision, chip.cores,
             static_cast<unsigned long>(flash_bytes / (1024U * 1024U)),
             static_cast<int>(esp_reset_reason()));
    ESP_LOGI(kTag, "heap_free=%lu heap_min=%lu psram=%lu",
             static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
             static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)),
             static_cast<unsigned long>(psram_bytes));
    std::printf("{\"type\":\"nightwave_boot\",\"schema\":1,\"firmware_version\":");
    json_string(app->version);
    std::printf(",\"idf_version\":"); json_string(esp_get_idf_version());
    std::printf(",\"reset_reason\":%d,\"heap_free_bytes\":%lu,\"heap_min_bytes\":%lu,\"psram_bytes\":%lu}\n",
        static_cast<int>(esp_reset_reason()), static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)), static_cast<unsigned long>(psram_bytes));
}

DiagnosticSnapshot capture_diagnostics(std::uint32_t audio_underruns,
                                       std::uint32_t storage_errors,
                                       std::uint32_t decode_errors,
                                       std::uint32_t pcm_queue_depth_ms) {
    DiagnosticSnapshot snapshot{};
    snapshot.uptime_ms = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);
    snapshot.audio_underruns = audio_underruns;
    snapshot.storage_errors = storage_errors;
    snapshot.decode_errors = decode_errors;
    snapshot.minimum_free_heap_bytes =
        heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    snapshot.pcm_queue_depth_ms = pcm_queue_depth_ms;
    return snapshot;
}

}  // namespace nightwave
