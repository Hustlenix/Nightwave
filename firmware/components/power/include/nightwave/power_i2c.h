#pragma once
#include "driver/i2c_master.h"
#include "nightwave/reference_power.h"
namespace nightwave {
// Owned by the serialized application/UI command path, never AudioTask.
class PowerI2c final : public PowerRegisterBus {
 public:
    bool read(std::uint8_t address,std::uint8_t reg,
              std::uint8_t* data,std::size_t count) override;
};
}
