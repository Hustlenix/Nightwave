#include "nightwave/button_monitor.h"
#include "nightwave/button_gestures.h"
#include "nightwave/hardware_config.h"
#include "nightwave/tca9535_i2c.h"
#include "nightwave/board_i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "buttons";
#if defined(CONFIG_NIGHTWAVE_REFERENCE_BOARD) && CONFIG_NIGHTWAVE_REFERENCE_BOARD
Tca9535I2c bus;
Tca9535Input expander(bus);
bool attached = false;
bool initialize_source() {
    if (!attached) {
        const auto shared=reference_i2c_device(reference_board::kTcaAddress);
        if (!shared || !bus.attach_borrowed(shared)) return false;
        attached=true;
    }
    return expander.initialize();
}
bool sample_source(std::uint8_t& pressed) {
    Tca9535Sample value{};
    if (!expander.poll(value)) return false;
    pressed = static_cast<std::uint8_t>(~value.raw_levels & 0x1f);
    return true;
}
#else
constexpr std::array<std::int8_t, 5> pins{hardware::kButtonPrevious,
    hardware::kButtonPlayPause, hardware::kButtonNext,
    hardware::kButtonVolumeDown, hardware::kButtonVolumeUp};
bool initialize_source() {
    gpio_config_t cfg{};
    for (const auto pin : pins) cfg.pin_bit_mask |= (1ULL << pin);
    cfg.mode = GPIO_MODE_INPUT; cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; cfg.intr_type = GPIO_INTR_DISABLE;
    return gpio_config(&cfg) == ESP_OK;
}
bool sample_source(std::uint8_t& pressed) {
    pressed = 0;
    for (std::size_t i = 0; i < pins.size(); ++i)
        if (!gpio_get_level(static_cast<gpio_num_t>(pins[i]))) pressed |= (1U << i);
    return true;
}
#endif
} // namespace

bool ButtonMonitor::start() {
    if (task_handle_) return true;
    queue_ = xQueueCreate(16, sizeof(ButtonEvent));
    if (!queue_) return false;
    TaskHandle_t handle = nullptr;
    if (xTaskCreate(task_entry, "InputTask", 4096, this, 4, &handle) != pdPASS) {
        vQueueDelete(static_cast<QueueHandle_t>(queue_)); queue_ = nullptr; return false;
    }
    task_handle_ = handle;
    return true; // Task exists; source readiness can recover independently.
}
bool ButtonMonitor::poll(ButtonEvent& event) {
    return queue_ && xQueueReceive(static_cast<QueueHandle_t>(queue_), &event, 0) == pdTRUE;
}
void ButtonMonitor::task_entry(void* context) { static_cast<ButtonMonitor*>(context)->task(); }
void ButtonMonitor::task() {
    ButtonGestures gestures;
    bool ready = false;
    const auto emit = [this](const ButtonEvent& event) {
        if (xQueueSend(static_cast<QueueHandle_t>(queue_), &event, 0) != pdTRUE)
            ESP_LOGW(kTag, "button queue full: gesture dropped");
    };
    while (true) {
        if (!ready) {
            gestures.invalidate();
            ready = initialize_source();
            if (!ready) {
                ESP_LOGW(kTag, "input source unavailable; retrying in 1 second");
                vTaskDelay(pdMS_TO_TICKS(1000)); continue;
            }
            ESP_LOGI(kTag, "input source ready: %s", hardware::kReferenceBoard ? "TCA9535 0x20 P0.0..4" : "legacy GPIO");
        }
        std::uint8_t pressed = 0;
        if (sample_source(pressed)) {
            gestures.sample(pressed, static_cast<std::uint32_t>(esp_timer_get_time() / 1000), emit);
        } else {
            ready = false; gestures.invalidate();
            ESP_LOGW(kTag, "input read failed; pending gestures cancelled");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
} // namespace nightwave
