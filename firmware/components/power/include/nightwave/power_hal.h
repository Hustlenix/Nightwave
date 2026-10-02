#pragma once
#include "nightwave/power_state.h"
namespace nightwave {
struct PowerReading {
    PowerState state{};
    std::uint32_t sampled_ms{0};
    bool voltage_valid{false}, charge_percent_valid{false}, charging_valid{false};
};
class PowerHal {
 public:
    virtual ~PowerHal() = default;
    virtual PowerReading sample(std::uint32_t now) = 0;
    virtual bool request_shutdown() = 0; // Physical latch implementation/qualification required.
};
class UnavailablePower final : public PowerHal {
 public:
    PowerReading sample(std::uint32_t now) override { PowerReading r; r.sampled_ms = now; return r; }
    bool request_shutdown() override { return false; }
};
struct BatteryThresholds { std::uint16_t low_mv{0}, cutoff_mv{0}, hysteresis_mv{0}; };
struct BatteryStatus { bool available{false}, low{false}, stop_playback{false}; };
class BatteryPolicy {
 public:
    explicit BatteryPolicy(BatteryThresholds thresholds = {}) : thresholds_(thresholds) {}
    BatteryStatus update(const PowerReading& reading, std::uint32_t now) {
        const auto& t = thresholds_;
        if (!reading.voltage_valid || now - reading.sampled_ms > 5000 || reading.state.battery_millivolts < 2000 || reading.state.battery_millivolts > 4500 ||
            t.low_mv < 2500 || t.low_mv > 3700 || t.cutoff_mv < 2500 || t.cutoff_mv > t.low_mv || t.hysteresis_mv > 300) return {};
        const auto mv = reading.state.battery_millivolts;
        if (mv <= t.low_mv) low_ = true;
        else if (mv >= t.low_mv + t.hysteresis_mv) low_ = false;
        return {true, low_, mv <= t.cutoff_mv};
    }
 private:
    BatteryThresholds thresholds_; bool low_{false};
};
} // namespace nightwave
