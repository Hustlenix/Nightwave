#pragma once
#include <cstddef>
#include "nightwave/power_hal.h"
namespace nightwave {
class PowerRegisterBus {
 public:
    virtual ~PowerRegisterBus()=default;
    // Register-address transfer only: deliberately no register-write API.
    virtual bool read(std::uint8_t address, std::uint8_t reg,
                      std::uint8_t* data, std::size_t count)=0;
};
struct ChargerDiagnostic {
    bool valid{false};
    std::uint8_t status0{0}, status1{0}, faults{0};
    std::uint16_t charge_limit_ma{0}, voltage_limit_mv{0}, input_limit_ma{0};
};
class ReferencePower final : public PowerHal {
 public:
    explicit ReferencePower(PowerRegisterBus& bus):bus_(bus) {}
    PowerReading sample(std::uint32_t now) override;
    bool request_shutdown() override { return false; } // No verified latch policy.
    ChargerDiagnostic diagnostic() const { return diagnostic_; }
 private:
    PowerRegisterBus& bus_;
    ChargerDiagnostic diagnostic_{};
};
}
