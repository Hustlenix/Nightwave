#include "nightwave/button_monitor.h"

#include <array>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nightwave/button_debouncer.h"
#include "nightwave/hardware_config.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "buttons";
struct ButtonDefinition {
    gpio_num_t pin;
    const char* name;
    ButtonId id;
};
constexpr std::array<ButtonDefinition, 5> kButtons{{
    {static_cast<gpio_num_t>(hardware::kButtonPrevious), "PREVIOUS", ButtonId::kPrevious},
    {static_cast<gpio_num_t>(hardware::kButtonPlayPause), "PLAY_PAUSE", ButtonId::kPlayPause},
    {static_cast<gpio_num_t>(hardware::kButtonNext), "NEXT", ButtonId::kNext},
    {static_cast<gpio_num_t>(hardware::kButtonVolumeDown), "VOLUME_DOWN", ButtonId::kVolumeDown},
    {static_cast<gpio_num_t>(hardware::kButtonVolumeUp), "VOLUME_UP", ButtonId::kVolumeUp},
}};
}  // namespace

bool ButtonMonitor::start() {
    if (task_handle_ != nullptr) return true;
    std::uint64_t pins = 0;
    for (const auto& button : kButtons) pins |= (1ULL << button.pin);
    gpio_config_t config{};
    config.pin_bit_mask = pins;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&config) != ESP_OK) return false;
    queue_ = xQueueCreate(16, sizeof(ButtonEvent));
    if (!queue_) return false;

    TaskHandle_t handle = nullptr;
    if (xTaskCreate(task_entry, "InputTask", 3072, this, 4, &handle) != pdPASS) {
        vQueueDelete(static_cast<QueueHandle_t>(queue_)); queue_ = nullptr;
        return false;
    }
    task_handle_ = handle;
    ESP_LOGI(kTag,
             "BUTTONS READY debounce=25ms PREVIOUS=GPIO1 PLAY_PAUSE=GPIO2 NEXT=GPIO4 VOLUME_DOWN=GPIO10 VOLUME_UP=GPIO15");
    return true;
}
bool ButtonMonitor::poll(ButtonEvent& event) {
    return queue_ && xQueueReceive(static_cast<QueueHandle_t>(queue_), &event, 0) == pdTRUE;
}

void ButtonMonitor::task_entry(void* context) {
    static_cast<ButtonMonitor*>(context)->task();
}

void ButtonMonitor::task() {
    std::array<ButtonDebouncer, kButtons.size()> debouncers{};
    std::array<std::uint32_t, kButtons.size()> pressed_at{};
    std::array<bool, kButtons.size()> long_sent{};
    const auto emit = [this](ButtonId id, ButtonGesture gesture, std::uint32_t time) {
        const ButtonEvent event{id, gesture, time};
        if (xQueueSend(static_cast<QueueHandle_t>(queue_), &event, 0) != pdTRUE)
            ESP_LOGW(kTag, "button queue full: gesture dropped");
    };
    while (true) {
        const auto now_ms = static_cast<std::uint32_t>(esp_timer_get_time() / 1000);
        for (std::size_t index = 0; index < kButtons.size(); ++index) {
            const bool pressed = gpio_get_level(kButtons[index].pin) == 0;
            if (debouncers[index].update(pressed, now_ms)) {
                if (debouncers[index].pressed()) {
                    pressed_at[index] = now_ms; long_sent[index] = false;
                } else if (!long_sent[index]) {
                    emit(kButtons[index].id, ButtonGesture::kPress, now_ms);
                }
                ESP_LOGI(kTag, "BUTTON %s %s t_ms=%lu", kButtons[index].name,
                         debouncers[index].pressed() ? "PRESSED" : "RELEASED",
                         static_cast<unsigned long>(now_ms));
            }
            if (debouncers[index].pressed() && !long_sent[index] &&
                now_ms - pressed_at[index] >= 700) {
                long_sent[index] = true;
                emit(kButtons[index].id, ButtonGesture::kLongPress, now_ms);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

}  // namespace nightwave
