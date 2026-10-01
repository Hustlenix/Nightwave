#pragma once
#include <cstdint>
using esp_err_t = int;
inline constexpr int ESP_OK = 0;
inline constexpr int ESP_ERR_TIMEOUT = 1;
using gpio_num_t = int;
inline constexpr int GPIO_MODE_OUTPUT = 1;
inline constexpr int GPIO_MODE_INPUT = 2;
inline constexpr int GPIO_PULLUP_ENABLE = 1;
inline constexpr int GPIO_PULLDOWN_DISABLE = 0;
inline constexpr int GPIO_PULLUP_DISABLE = 0;
inline constexpr int GPIO_PULLDOWN_ENABLE = 1;
inline constexpr int GPIO_INTR_DISABLE = 0;
struct gpio_config_t {
    std::uint64_t pin_bit_mask{};
    int mode{}, pull_up_en{}, pull_down_en{}, intr_type{};
};
esp_err_t gpio_config(const gpio_config_t*);
esp_err_t gpio_set_level(gpio_num_t, int);
int gpio_get_level(gpio_num_t);
