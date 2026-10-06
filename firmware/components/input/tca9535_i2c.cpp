#include "nightwave/tca9535_i2c.h"

namespace nightwave {

bool Tca9535I2c::attach(i2c_master_bus_handle_t bus, std::uint8_t address) {
    if (device_ || !bus || address < 0x20 || address > 0x27) return false;
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = 100000; // Shared with BQ25628E; respect its START spacing.
    i2c_master_dev_handle_t handle{};
    if (i2c_master_bus_add_device(bus, &config, &handle) != ESP_OK) return false;
    device_ = handle;
    owns_device_ = true;
    return true;
}

bool Tca9535I2c::attach_borrowed(i2c_master_dev_handle_t device) {
    if (device_ || !device) return false;
    device_=device; owns_device_=false;
    return true;
}

bool Tca9535I2c::detach() {
    if (!device_) return true;
    if (owns_device_ && i2c_master_bus_rm_device(device_) != ESP_OK) return false;
    device_ = nullptr;
    owns_device_ = false;
    return true;
}

bool Tca9535I2c::write_pair(std::uint8_t first_register, std::uint8_t port0,
                           std::uint8_t port1) {
    if (!device_ || (first_register != 4 && first_register != 6)) return false;
    const std::uint8_t packet[]{first_register, port0, port1};
    return i2c_master_transmit(device_, packet, sizeof(packet), 10) == ESP_OK;
}

bool Tca9535I2c::read_pair(std::uint8_t first_register, std::uint8_t (&ports)[2]) {
    if (!device_ || (first_register != 0 && first_register != 4 && first_register != 6)) return false;
    std::uint8_t received[2]{};
    if (i2c_master_transmit_receive(device_, &first_register, 1, received, 2, 10) != ESP_OK) return false;
    ports[0] = received[0];
    ports[1] = received[1];
    return true;
}

}  // namespace nightwave
