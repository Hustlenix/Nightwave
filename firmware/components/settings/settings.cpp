#include "nightwave/settings.h"
#include "nvs.h"
#include "nvs_flash.h"
namespace nightwave {
bool initialize_settings() {
    // Do not silently erase the entire NVS partition on version/storage errors.
    return nvs_flash_init() == ESP_OK;
}
bool load_settings(Settings& settings) {
    nvs_handle_t handle;
    if (nvs_open("nightwave", NVS_READONLY, &handle) != ESP_OK) return false;
    std::uint8_t volume = settings.volume_percent;
    const auto status = nvs_get_u8(handle, "volume", &volume);
    if (status == ESP_OK && volume <= 100) settings.volume_percent = volume;
    nvs_close(handle);
    return status == ESP_OK && volume <= 100;
}
bool save_settings(const Settings& settings) {
    if (settings.volume_percent > 100) return false;
    nvs_handle_t handle;
    if (nvs_open("nightwave", NVS_READWRITE, &handle) != ESP_OK) return false;
    bool ok = nvs_set_u8(handle, "volume", settings.volume_percent) == ESP_OK;
    if (ok) ok = nvs_commit(handle) == ESP_OK;
    nvs_close(handle); return ok;
}
}  // namespace nightwave
