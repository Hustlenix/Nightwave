#include "nightwave/board_i2c.h"
#include "nightwave/hardware_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
namespace nightwave {
namespace {
SemaphoreHandle_t guard{};
i2c_master_bus_handle_t bus{};
}
bool prepare_board_i2c() {
    if (!hardware::kReferenceBoard) return true;
    if (!guard) guard = xSemaphoreCreateMutex();
    return guard && reference_i2c_bus();
}
i2c_master_bus_handle_t reference_i2c_bus() {
    if (!hardware::kReferenceBoard || !guard ||
        xSemaphoreTake(guard, pdMS_TO_TICKS(20)) != pdTRUE) return nullptr;
    if (!bus) {
        i2c_master_bus_config_t cfg{};
        cfg.i2c_port=I2C_NUM_0;
        cfg.sda_io_num=static_cast<gpio_num_t>(hardware::kI2cSda);
        cfg.scl_io_num=static_cast<gpio_num_t>(hardware::kI2cScl);
        cfg.clk_source=I2C_CLK_SRC_DEFAULT;
        cfg.glitch_ignore_cnt=7;
        i2c_master_bus_handle_t created{};
        if (i2c_new_master_bus(&cfg,&created)==ESP_OK) bus=created;
    }
    const auto result=bus;
    xSemaphoreGive(guard);
    return result;
}
}
