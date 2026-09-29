#pragma once

#include <cstdint>

namespace nightwave {

enum class PowerSource : std::uint8_t { kBattery, kUsb, kUnknown };

struct PowerState {
    PowerSource source{PowerSource::kUnknown};
    std::uint16_t battery_millivolts{0};
    std::uint8_t state_of_charge_percent{0};
    bool charging{false};
    bool low_battery{false};
};

}  // namespace nightwave
