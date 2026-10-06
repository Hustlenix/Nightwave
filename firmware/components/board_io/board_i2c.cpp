#include "nightwave/board_i2c.h"
#include "nightwave/hardware_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
namespace nightwave {
namespace {
SemaphoreHandle_t guard{};
i2c_master_bus_handle_t bus{};
i2c_master_dev_handle_t devices[3]{};
void create_bus_locked() {
    if (bus) return;
    i2c_master_bus_config_t cfg{};
    cfg.i2c_port=I2C_NUM_0;
    cfg.sda_io_num=static_cast<gpio_num_t>(hardware::kI2cSda);
    cfg.scl_io_num=static_cast<gpio_num_t>(hardware::kI2cScl);
    cfg.clk_source=I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt=7;
    i2c_master_bus_handle_t created{};
    if (i2c_new_master_bus(&cfg,&created)==ESP_OK) bus=created;
}
}
bool prepare_board_i2c() {
    if (!hardware::kReferenceBoard) return true;
    if (!guard) guard = xSemaphoreCreateMutex();
    return guard && reference_i2c_bus();
}
i2c_master_bus_handle_t reference_i2c_bus() {
    if (!hardware::kReferenceBoard || !guard ||
        xSemaphoreTake(guard, pdMS_TO_TICKS(20)) != pdTRUE) return nullptr;
    create_bus_locked();
    const auto result=bus;
    xSemaphoreGive(guard);
    return result;
}
i2c_master_dev_handle_t reference_i2c_device(unsigned address) {
    const int index=address==0x20 ? 0 : address==0x36 ? 1 : address==0x6a ? 2 : -1;
    if (index<0 || !hardware::kReferenceBoard || !guard ||
        xSemaphoreTake(guard,pdMS_TO_TICKS(20))!=pdTRUE) return nullptr;
    create_bus_locked();
    if (bus && !devices[index]) {
        i2c_device_config_t cfg{};
        cfg.dev_addr_length=I2C_ADDR_BIT_LEN_7;
        cfg.device_address=address; cfg.scl_speed_hz=100000;
        i2c_master_dev_handle_t created{};
        if (i2c_master_bus_add_device(bus,&cfg,&created)==ESP_OK) devices[index]=created;
    }
    const auto result=devices[index];
    xSemaphoreGive(guard);
    return result;
}
}
