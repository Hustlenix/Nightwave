#include "nightwave/diagnostics.h"

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_idf_version.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "diagnostics";
}

void log_boot_diagnostics() {
    esp_chip_info_t chip{};
    esp_chip_info(&chip);
    const esp_app_desc_t* app = esp_app_get_description();
    const auto psram_bytes = esp_psram_is_initialized() ? esp_psram_get_size() : 0;

    ESP_LOGI(kTag, "BOOT OK");
    ESP_LOGI(kTag, "project=%s version=%s idf=%s", app->project_name,
             app->version, esp_get_idf_version());
    ESP_LOGI(kTag,
             "target=ESP32-S3 revision=%d cores=%d flash=%luMB reset_reason=%d",
             chip.revision, chip.cores,
             static_cast<unsigned long>(chip.flash_size / (1024U * 1024U)),
             static_cast<int>(esp_reset_reason()));
    ESP_LOGI(kTag, "heap_free=%lu heap_min=%lu psram=%lu",
             static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
             static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)),
             static_cast<unsigned long>(psram_bytes));
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
