#include "nightwave/settings.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <cstring>
namespace nightwave {
namespace {
constexpr std::size_t kRecordSize = 788;
std::uint32_t checksum(const std::uint8_t* bytes, std::size_t size) {
    std::uint32_t value = 2166136261U;
    for (std::size_t i = 0; i < size; ++i) value = (value ^ bytes[i]) * 16777619U;
    return value;
}
void put(std::uint8_t* p, std::uint32_t n) { for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(n >> (i * 8)); }
std::uint32_t get(const std::uint8_t* p) { return p[0] | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24); }
bool valid(const Settings& s) {
    return s.volume_percent <= 100 && static_cast<unsigned>(s.repeat_mode) <= 2 &&
        static_cast<unsigned>(s.output) <= 3 && s.resume_position_ms <= 1800000 &&
        std::memchr(s.resume_path.data(), 0, 256) && std::memchr(s.playlist_path.data(), 0, 256) && std::memchr(s.library_folder.data(), 0, 256);
}
}
bool initialize_settings() {
    // Do not silently erase the entire NVS partition on version/storage errors.
    return nvs_flash_init() == ESP_OK;
}
bool load_settings(Settings& settings) {
    nvs_handle_t handle;
    if (nvs_open("nightwave", NVS_READONLY, &handle) != ESP_OK) return false;
    std::array<std::uint8_t, kRecordSize> record{}; std::size_t size = record.size();
    const auto status = nvs_get_blob(handle, "state_v2", record.data(), &size);
    if (status == ESP_ERR_NVS_NOT_FOUND) {
        std::uint8_t volume = settings.volume_percent;
        const auto legacy = nvs_get_u8(handle, "volume", &volume);
        nvs_close(handle);
        if (legacy != ESP_OK || volume > 100) return false;
        settings.volume_percent = volume; return true;
    }
    nvs_close(handle);
    if (status != ESP_OK || size != record.size() || std::memcmp(record.data(), "NWS2", 4) ||
        get(record.data() + 4) != 2 || get(record.data() + 784) != checksum(record.data(), 784) || record[10] > 1) return false;
    Settings candidate; candidate.volume_percent = record[8]; candidate.repeat_mode = static_cast<RepeatMode>(record[9]);
    candidate.shuffle = record[10] != 0; candidate.output = static_cast<OutputPreference>(record[11]);
    candidate.resume_position_ms = get(record.data() + 12);
    std::memcpy(candidate.resume_path.data(), record.data() + 16, 256);
    std::memcpy(candidate.playlist_path.data(), record.data() + 272, 256);
    std::memcpy(candidate.library_folder.data(), record.data() + 528, 256);
    if (!valid(candidate)) return false;
    // Bluetooth audio is not implemented yet. Older/experimental NVS records
    // may still contain that preference; migrate them to a usable route
    // instead of booting into a permanently muted player.
    if (candidate.output == OutputPreference::kBluetooth)
        candidate.output = OutputPreference::kAutomatic;
    settings = candidate; return true;
}
bool save_settings(const Settings& settings) {
    // Never persist an output route the current firmware cannot actually use.
    if (!valid(settings) || settings.output == OutputPreference::kBluetooth) return false;
    nvs_handle_t handle;
    if (nvs_open("nightwave", NVS_READWRITE, &handle) != ESP_OK) return false;
    std::array<std::uint8_t, kRecordSize> record{};
    std::memcpy(record.data(), "NWS2", 4); put(record.data() + 4, 2);
    record[8] = settings.volume_percent; record[9] = static_cast<std::uint8_t>(settings.repeat_mode);
    record[10] = settings.shuffle; record[11] = static_cast<std::uint8_t>(settings.output);
    put(record.data() + 12, settings.resume_position_ms);
    std::memcpy(record.data() + 16, settings.resume_path.data(), 256);
    std::memcpy(record.data() + 272, settings.playlist_path.data(), 256);
    std::memcpy(record.data() + 528, settings.library_folder.data(), 256);
    put(record.data() + 784, checksum(record.data(), 784));
    bool ok = nvs_set_blob(handle, "state_v2", record.data(), record.size()) == ESP_OK;
    if (ok) ok = nvs_set_u8(handle, "volume", settings.volume_percent) == ESP_OK;
    if (ok) ok = nvs_commit(handle) == ESP_OK;
    nvs_close(handle); return ok;
}
}  // namespace nightwave
