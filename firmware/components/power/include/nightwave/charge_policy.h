#pragma once
#include <algorithm>
#include <cstdint>

namespace nightwave {
// Pure supervisory policy for the approved >=1.5 A Type-C architecture.
// Not a hardware protection substitute or a driver for the old reference PCB.
enum class ChargeBlock { kNone, kSource, kStale, kUnqualified, kFault, kTemperature, kBudget };
struct ChargeObservation {
    std::uint32_t sampled_ms{0};
    bool cc_valid{false}, attached{false}, source_at_least_1500ma{false};
    bool protection_good{false}, charger_valid{false}, pack_present{false};
    bool ntc_valid{false}, temperature_in_window{false};
    bool watchdog_fault{false}, timer_fault{false}, thermal_fault{false};
    bool register_readback_ok{false};
    // Conservative available battery-side charging current after system load,
    // conversion loss and input tolerance. Zero if not established.
    std::uint16_t available_charge_ma{0};
};
struct ChargeQualification {
    // Defaults deliberately prevent enabling a not-yet-selected pack/circuit.
    bool hardware_reviewed{false}, pack_reviewed{false};
    std::uint16_t max_pack_charge_ma{0}, max_charge_mv{0};
};
struct ChargePlan {
    bool input_permitted{false}, charge_permitted{false};
    std::uint16_t input_limit_ma{0}, charge_limit_ma{0};
    ChargeBlock blocked{ChargeBlock::kSource};
};
class ChargePolicy {
 public:
    explicit ChargePolicy(ChargeQualification qualification = {}) : qualification_(qualification) {}
    ChargePlan update(const ChargeObservation& o, std::uint32_t now) {
        // Faults require explicit re-arm, never repeated CE pulses/timer resets.
        if (o.watchdog_fault || o.timer_fault || o.thermal_fault) fault_latched_ = true;
        ChargePlan p;
        if (now-o.sampled_ms > 100) { p.blocked=ChargeBlock::kStale; return p; }
        if (!o.cc_valid || !o.attached || !o.source_at_least_1500ma || !o.protection_good) return p;
        if (!qualification_.hardware_reviewed || !qualification_.pack_reviewed ||
            qualification_.max_charge_mv != 4200 || qualification_.max_pack_charge_ma < 40 ||
            qualification_.max_pack_charge_ma > 2000) {
            p.blocked=ChargeBlock::kUnqualified; return p;
        }
        if (fault_latched_ || !o.charger_valid || !o.register_readback_ok) {
            p.blocked=ChargeBlock::kFault; return p;
        }
        // 1400 mA is a planning ceiling; hardware must independently enforce
        // <=1500 mA TOTAL including all control/protection loads and tolerances.
        p.input_permitted=true; p.input_limit_ma=1400;
        if (!o.pack_present || !o.ntc_valid || !o.temperature_in_window) {
            p.blocked=ChargeBlock::kTemperature; return p;
        }
        const auto limit=std::min<std::uint16_t>(960,
            std::min(qualification_.max_pack_charge_ma, o.available_charge_ma));
        p.charge_limit_ma=static_cast<std::uint16_t>((limit/40)*40);
        if (!p.charge_limit_ma) { p.blocked=ChargeBlock::kBudget; return p; }
        p.charge_permitted=true; p.blocked=ChargeBlock::kNone; return p;
    }
    bool rearm(const ChargeObservation& o, std::uint32_t now) {
        // Require fresh, explicitly detached and fault-free observation.
        if (now-o.sampled_ms > 100 || !o.cc_valid || o.attached ||
            o.watchdog_fault || o.timer_fault || o.thermal_fault) return false;
        fault_latched_=false; return true;
    }
 private:
    ChargeQualification qualification_;
    bool fault_latched_{false};
};
} // namespace nightwave
