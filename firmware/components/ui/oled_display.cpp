#include "nightwave/oled_display.h"
#include <algorithm>
#include "driver/i2c_master.h"
#include "nightwave/hardware_config.h"
namespace nightwave {
bool OledDisplay::command(const std::uint8_t* bytes, std::size_t count) {
    if (!device_ || count > 31) return false;
    std::array<std::uint8_t, 32> packet{};
    std::copy_n(bytes, count, packet.data() + 1);
    return i2c_master_transmit(static_cast<i2c_master_dev_handle_t>(device_),
        packet.data(), count + 1, 100) == ESP_OK;
}
bool OledDisplay::initialize(OledController controller) {
    controller_ = controller;
    i2c_master_bus_config_t config{};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = static_cast<gpio_num_t>(hardware::kI2cSda);
    config.scl_io_num = static_cast<gpio_num_t>(hardware::kI2cScl);
    config.clk_source = I2C_CLK_SRC_DEFAULT; config.glitch_ignore_cnt = 7;
    // External 3.3 V pull-ups required. Internal pull-ups are not a design substitute.
    i2c_master_bus_handle_t bus;
    if (i2c_new_master_bus(&config, &bus) != ESP_OK) return false;
    bus_ = bus;
    i2c_device_config_t device{};
    device.dev_addr_length = I2C_ADDR_BIT_LEN_7; device.device_address = 0x3c;
    device.scl_speed_hz = 400000;
    i2c_master_dev_handle_t handle;
    if (i2c_master_bus_add_device(bus, &device, &handle) != ESP_OK) {
        i2c_del_master_bus(bus); bus_ = nullptr; return false;
    }
    device_ = handle;
    // Provisional 128x64 module with internal charge pump. The final
    // display BOM/connector must confirm controller, address and supply mode.
    constexpr std::uint8_t ssd_init[]{0xae,0xd5,0x80,0xa8,0x3f,0xd3,0x00,0x40,
        0x8d,0x14,0x20,0x00,0xa1,0xc8,0xda,0x12,0x81,0x7f,0xd9,0xf1,
        0xdb,0x40,0xa4,0xa6,0x2e,0xaf};
    constexpr std::uint8_t sh_init[]{0xae,0xd5,0x80,0xa8,0x3f,0xd3,0x00,0x40,
        0xad,0x8b,0xa1,0xc8,0xda,0x12,0x81,0x7f,0xd9,0x22,0xdb,0x35,
        0xa4,0xa6,0xaf};
    if (controller == OledController::kSh1106 ? command(sh_init, sizeof(sh_init))
                                            : command(ssd_init, sizeof(ssd_init))) return true;
    i2c_master_bus_rm_device(handle); i2c_del_master_bus(bus);
    device_ = nullptr; bus_ = nullptr; return false;
}
bool OledDisplay::sleep(bool asleep) {
    const std::uint8_t code = asleep ? 0xae : 0xaf;
    return command(&code, 1);
}
bool OledDisplay::show(const TextFrame& text) {
    if (!device_) return false;
    constexpr std::uint8_t window[]{0x21,0,127,0x22,0,7};
    if (controller_ == OledController::kSsd1306 && !command(window, sizeof(window))) return false;
    const auto pixels = rasterize(text);
    std::array<std::uint8_t, 129> page{}; page[0] = 0x40;
    for (std::size_t i = 0; i < 8; ++i) {
        if (controller_ == OledController::kSh1106) {
            const std::uint8_t address[]{static_cast<std::uint8_t>(0xb0 + i), 0x02, 0x10};
            if (!command(address, sizeof(address))) return false;
        }
        std::copy_n(pixels.data() + i * 128, 128, page.data() + 1);
        if (i2c_master_transmit(static_cast<i2c_master_dev_handle_t>(device_),
            page.data(), page.size(), 100) != ESP_OK) return false;
    }
    return true;
}
}  // namespace nightwave
