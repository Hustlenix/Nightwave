#include "nightwave/storage.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <new>
#include <string>

#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "nightwave/hardware_config.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "storage";
constexpr char kMountPoint[] = "/sdcard";

bool supported_extension(const char* name) {
    const char* dot = std::strrchr(name, '.');
    if (dot == nullptr) return false;
    std::string extension(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return std::tolower(value); });
    return extension == ".wav" || extension == ".mp3";
}
}  // namespace

bool SdStorage::mount() {
    if (mounted_) return true;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = static_cast<gpio_num_t>(hardware::kSdClk);
    slot.cmd = static_cast<gpio_num_t>(hardware::kSdCmd);
    slot.d0 = static_cast<gpio_num_t>(hardware::kSdD0);

    esp_vfs_fat_sdmmc_mount_config_t config{};
    config.format_if_mount_failed = false;
    // Catalog reader + idle builder/dir queue + one directory + playback and
    // sidecar/settings UI access. Traversal never holds a directory stack open.
    config.max_files = 8;
    config.allocation_unit_size = 16 * 1024;

    sdmmc_card_t* mounted_card = nullptr;
    const esp_err_t error = esp_vfs_fat_sdmmc_mount(
        kMountPoint, &host, &slot, &config, &mounted_card);
    if (error != ESP_OK) {
        ++errors_;
        ESP_LOGE(kTag, "SD mount failed: %s", esp_err_to_name(error));
        return false;
    }
    card_ = mounted_card;
    mounted_ = true;
    ESP_LOGI(kTag, "SD mounted at %s in 1-bit SDMMC mode", kMountPoint);
    sdmmc_card_print_info(stdout, mounted_card);
    return true;
}

void SdStorage::unmount() {
    if (!mounted_) return;
    esp_vfs_fat_sdcard_unmount(kMountPoint,
                               static_cast<sdmmc_card_t*>(card_));
    card_ = nullptr;
    mounted_ = false;
    ESP_LOGI(kTag, "SD unmounted");
}

std::size_t SdStorage::enumerate_supported_files() const {
    if (!mounted_) return 0;
    DIR* directory = opendir(kMountPoint);
    if (directory == nullptr) {
        ++errors_;
        ESP_LOGE(kTag, "Cannot enumerate %s", kMountPoint);
        return 0;
    }
    std::size_t count = 0;
    while (const dirent* entry = readdir(directory)) {
        if (entry->d_name[0] == '.') continue;
        if (supported_extension(entry->d_name)) {
            ++count;
            ESP_LOGI(kTag, "audio[%u]=%s", static_cast<unsigned>(count),
                     entry->d_name);
        }
    }
    closedir(directory);
    ESP_LOGI(kTag, "enumeration complete: %u supported files",
             static_cast<unsigned>(count));
    return count;
}

StorageBenchmark SdStorage::benchmark(const char* path,
                                      std::size_t chunk_bytes) const {
    StorageBenchmark result{};
    if (!mounted_ || path == nullptr || chunk_bytes == 0) {
        ++errors_;
        result.read_errors = 1;
        return result;
    }
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        ESP_LOGE(kTag, "benchmark open failed: %s", path);
        result.read_errors = 1;
        ++errors_;
        return result;
    }
    std::unique_ptr<std::uint8_t[]> buffer(new (std::nothrow)
                                               std::uint8_t[chunk_bytes]);
    if (!buffer) {
        std::fclose(file);
        result.read_errors = 1;
        ++errors_;
        return result;
    }
    const std::int64_t started_us = esp_timer_get_time();
    while (true) {
        const std::int64_t read_started_us = esp_timer_get_time();
        const std::size_t bytes = std::fread(buffer.get(), 1, chunk_bytes, file);
        const auto read_us = static_cast<std::uint32_t>(
            esp_timer_get_time() - read_started_us);
        result.worst_read_us = std::max(result.worst_read_us, read_us);
        result.bytes_read += bytes;
        if (bytes < chunk_bytes) {
            if (std::ferror(file)) {
                ++result.read_errors;
                ++errors_;
            }
            break;
        }
    }
    result.elapsed_ms = static_cast<std::uint32_t>(
        std::max<std::int64_t>(1, (esp_timer_get_time() - started_us) / 1000));
    result.average_kib_per_second = static_cast<std::uint32_t>(
        (result.bytes_read * 1000ULL) / (result.elapsed_ms * 1024ULL));
    std::fclose(file);
    ESP_LOGI(kTag,
             "SD BENCH bytes=%llu elapsed_ms=%lu avg_kib_s=%lu worst_read_us=%lu errors=%lu",
             static_cast<unsigned long long>(result.bytes_read),
             static_cast<unsigned long>(result.elapsed_ms),
             static_cast<unsigned long>(result.average_kib_per_second),
             static_cast<unsigned long>(result.worst_read_us),
             static_cast<unsigned long>(result.read_errors));
    return result;
}

}  // namespace nightwave
