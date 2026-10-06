#pragma once
#include "nightwave/st7789_display.h"
#include "driver/spi_master.h"
namespace nightwave {
class St7789Spi final : public St7789Bus {
 public:
    bool connect() override;
    void disconnect() override;
    bool reset(bool high) override;
    void delay_ms(std::uint32_t ms) override;
    bool command(std::uint8_t code, const std::uint8_t* data, std::size_t count) override;
    bool pixels(const std::uint8_t* data, std::size_t count) override;
    bool backlight(bool on) override;
 private:
    bool send(bool data, const std::uint8_t* bytes, std::size_t size);
    spi_device_handle_t device_{nullptr};
    bool bus_owned_{false}, pins_ready_{false};
};
}
