#include "esp_log.h"

#include "nightwave/app_state.h"
#include "nightwave/audio_types.h"
#include "nightwave/diagnostics.h"
#include "nightwave/hardware_config.h"

namespace {

constexpr char kTag[] = "nightwave";

static_assert(nightwave::hardware::pins_are_unique(),
              "Nightwave GPIO assignments must be unique");
static_assert(nightwave::hardware::pins_avoid_restricted_set(),
              "Nightwave GPIO assignments overlap a restricted ESP32-S3 pin");

}  // namespace

extern "C" void app_main() {
    constexpr nightwave::AudioFormat kBootFormat{
        44100,
        2,
        16,
    };
    constexpr nightwave::DiagnosticSnapshot kInitialDiagnostics{};

    ESP_LOGI(kTag, "Nightwave Phase 1 interface scaffold started");
    ESP_LOGI(kTag, "Default PCM contract: %lu Hz, %u channels, %u-bit",
             static_cast<unsigned long>(kBootFormat.sample_rate_hz),
             kBootFormat.channel_count,
             kBootFormat.bits_per_sample);
    ESP_LOGI(kTag, "Initial state=%u underruns=%lu",
             static_cast<unsigned>(nightwave::AppState::kBooting),
             static_cast<unsigned long>(kInitialDiagnostics.audio_underruns));
    ESP_LOGW(kTag, "No hardware drivers are started in the Phase 1 scaffold");
}
