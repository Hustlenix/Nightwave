#pragma once

#include "driver/i2c_master.h"
#include "nightwave/tca9535_input.h"

namespace nightwave {

// Borrows an existing, initialized shared bus; never creates/reassigns GPIO.
// The bus must outlive this device. Not enabled by the legacy bench firmware.
class Tca9535I2c final : public Tca9535Bus {
  public:
    Tca9535I2c() = default;
    ~Tca9535I2c() override { detach(); }
    Tca9535I2c(const Tca9535I2c&) = delete;
    Tca9535I2c& operator=(const Tca9535I2c&) = delete;
    bool attach(i2c_master_bus_handle_t bus, std::uint8_t address);
    bool detach();
    bool write_pair(std::uint8_t first_register, std::uint8_t port0,
                    std::uint8_t port1) override;
    bool read_pair(std::uint8_t first_register, std::uint8_t (&ports)[2]) override;

  private:
    i2c_master_dev_handle_t device_{};
};

}  // namespace nightwave
