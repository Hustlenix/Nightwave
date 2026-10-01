#pragma once
inline void fake_esp_log(const char*, const char*, ...) {}
#define ESP_LOGI(...) fake_esp_log(__VA_ARGS__)
#define ESP_LOGE(...) fake_esp_log(__VA_ARGS__)
#define ESP_LOGW(...) fake_esp_log(__VA_ARGS__)
