#pragma once
#include <cstdint>
#include <cstddef>
#include "driver/gpio.h"
using nvs_handle_t = unsigned;
constexpr int NVS_READONLY = 0, NVS_READWRITE = 1;
constexpr int ESP_ERR_NVS_NOT_FOUND = 0x1102;
esp_err_t nvs_open(const char*, int, nvs_handle_t*);
esp_err_t nvs_get_u8(nvs_handle_t, const char*, std::uint8_t*);
esp_err_t nvs_set_u8(nvs_handle_t, const char*, std::uint8_t);
esp_err_t nvs_get_blob(nvs_handle_t, const char*, void*, std::size_t*);
esp_err_t nvs_set_blob(nvs_handle_t, const char*, const void*, std::size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
