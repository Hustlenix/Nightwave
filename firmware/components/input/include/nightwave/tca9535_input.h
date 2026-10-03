#pragma once

#include <cstdint>

namespace nightwave {

// One serialized owner. Implementations must use finite bus timeouts. No I/O
// belongs in an ISR or audio task; INT is only a scheduling hint.
class Tca9535Bus {
  public:
    virtual ~Tca9535Bus() = default;
    virtual bool write_pair(std::uint8_t first_register, std::uint8_t port0,
                            std::uint8_t port1) = 0;
    virtual bool read_pair(std::uint8_t first_register, std::uint8_t (&ports)[2]) = 0;
};

struct Tca9535Sample {
    std::uint16_t raw_levels{};
    std::uint16_t changed{};
};

// Input-only BD-16 driver; output/mute/power control is intentionally absent.
// Reads both ports (not an atomic 16-bit hardware snapshot). External pulls
// must define every input, including unused inputs. No board pinout is implied.
class Tca9535Input {
  public:
    explicit Tca9535Input(Tca9535Bus& bus) : bus_(bus) {}

    bool initialize() {
        ready_ = false;
        // High impedance first, then disable polarity inversion. Never drive
        // a button/status net while configuring or recovering the device.
        if (!bus_.write_pair(6, 0xff, 0xff) || !bus_.write_pair(4, 0, 0)) return false;
        std::uint8_t ports[2]{};
        if (!bus_.read_pair(6, ports) || ports[0] != 0xff || ports[1] != 0xff) return false;
        if (!bus_.read_pair(4, ports) || ports[0] != 0 || ports[1] != 0) return false;
        if (!bus_.read_pair(0, ports)) return false;
        previous_ = combine(ports);
        ready_ = true;
        return true;
    }

    bool poll(Tca9535Sample& out) {
        if (!ready_) return false;
        std::uint8_t ports[2]{};
        if (!bus_.read_pair(0, ports)) {
            ready_ = false;
            return false;  // No synthetic presses or partially updated sample.
        }
        const auto raw = combine(ports);
        out = {raw, static_cast<std::uint16_t>(raw ^ previous_)};
        previous_ = raw;
        return true;
    }

    bool ready() const { return ready_; }
    void invalidate() { ready_ = false; }

  private:
    static std::uint16_t combine(const std::uint8_t (&ports)[2]) {
        return static_cast<std::uint16_t>(ports[0] | (static_cast<std::uint16_t>(ports[1]) << 8));
    }
    Tca9535Bus& bus_;
    std::uint16_t previous_{};
    bool ready_{};
};

}  // namespace nightwave
