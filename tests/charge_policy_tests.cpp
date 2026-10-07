#include "nightwave/charge_policy.h"
#include <cassert>
#include <cstdio>
using namespace nightwave;
static ChargeObservation good() {
    ChargeObservation o;
    o.sampled_ms=1000; o.cc_valid=o.attached=o.source_at_least_1500ma=true;
    o.protection_good=o.charger_valid=o.pack_present=true;
    o.ntc_valid=o.temperature_in_window=o.register_readback_ok=true;
    o.available_charge_ma=1000; return o;
}
int main() {
    ChargePolicy unqualified;
    assert(!unqualified.update(good(),1000).input_permitted);
    ChargePolicy policy({true,true,1000,4200});
    auto o=good(); auto p=policy.update(o,1000);
    assert(p.input_permitted && p.charge_permitted && p.input_limit_ma==1400 && p.charge_limit_ma==960);
    for (unsigned budget=0; budget<=2000; ++budget) {
        o=good(); o.available_charge_ma=static_cast<std::uint16_t>(budget);
        p=policy.update(o,1000);
        assert(p.charge_limit_ma<=budget && p.charge_limit_ma<=960 && p.charge_limit_ma%40==0);
        assert(p.charge_permitted==(budget>=40));
    }
    for (auto member : {&ChargeObservation::cc_valid, &ChargeObservation::attached,
         &ChargeObservation::source_at_least_1500ma, &ChargeObservation::protection_good,
         &ChargeObservation::charger_valid, &ChargeObservation::register_readback_ok}) {
        o=good(); o.*member=false; p=policy.update(o,1000);
        assert(!p.input_permitted && !p.charge_permitted);
    }
    for (auto member : {&ChargeObservation::pack_present, &ChargeObservation::ntc_valid,
         &ChargeObservation::temperature_in_window}) {
        o=good(); o.*member=false; p=policy.update(o,1000);
        assert(!p.charge_permitted && p.charge_limit_ma==0);
    }
    assert(!policy.update(good(),1101).input_permitted);
    assert(!policy.update(good(),999).input_permitted); // future timestamp
    o=good(); o.sampled_ms=0xfffffff0u;
    assert(policy.update(o,20).charge_permitted); // monotonic wrap
    for (auto member : {&ChargeObservation::watchdog_fault, &ChargeObservation::timer_fault,
         &ChargeObservation::thermal_fault}) {
        o=good(); o.*member=true;
        assert(!policy.update(o,1000).charge_permitted);
        o=good(); assert(!policy.update(o,1000).charge_permitted);
        assert(!policy.rearm(o,1000));
        o.attached=false; assert(!policy.rearm(o,1101));
        assert(policy.rearm(o,1000));
        assert(policy.update(good(),1000).charge_permitted);
    }
    for (unsigned pack=40; pack<=2000; ++pack) {
        ChargePolicy limited({true,true,static_cast<std::uint16_t>(pack),4200});
        assert(limited.update(good(),1000).charge_limit_ma<=pack);
    }
    ChargePolicy wrong_voltage({true,true,1000,4350});
    assert(!wrong_voltage.update(good(),1000).charge_permitted);
    std::puts("Charge policy permission, bounds, freshness and latched-fault tests passed");
}
