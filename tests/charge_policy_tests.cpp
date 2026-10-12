#include "nightwave/charge_policy.h"
#include <cassert>
#include <cstdio>
using namespace nightwave;
static ChargeObservation good() {
    ChargeObservation o;
    o.sampled_ms=1000; o.cc_valid=o.attached=o.source_at_least_1500ma=true;
    o.protection_good=o.charger_valid=o.pack_present=true;
    o.ntc_valid=o.temperature_in_window=o.register_readback_ok=true;
    o.available_charge_ma=1000; o.readback_charge_mv=4180; return o;
}
int main() {
    ChargePolicy unqualified;
    assert(!unqualified.update(good(),1000).input_permitted);
    // Synthetic 25% qualification envelope, NOT approval of charger accuracy.
    ChargePolicy policy({true,true,1000,4200,250,4,true});
    auto o=good(); auto p=policy.update(o,1000);
    assert(p.input_permitted && p.charge_permitted && p.input_limit_ma==1400 && p.charge_limit_ma==800);
    assert(p.charge_voltage_mv==4180);
    for (unsigned budget=0; budget<=2000; ++budget) {
        o=good(); o.available_charge_ma=static_cast<std::uint16_t>(budget);
        p=policy.update(o,1000);
        assert(p.charge_limit_ma<=budget && p.charge_limit_ma<=960 && p.charge_limit_ma%40==0);
        assert(unsigned(p.charge_limit_ma)*1250<=budget*1000);
        assert(p.charge_permitted==(budget>=50));
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
        ChargePolicy limited({true,true,static_cast<std::uint16_t>(pack),4200,250,4,true});
        assert(unsigned(limited.update(good(),1000).charge_limit_ma)*1250<=pack*1000);
    }
    ChargePolicy wrong_voltage({true,true,1000,4350,250,4,true});
    assert(!wrong_voltage.update(good(),1000).charge_permitted);
    ChargePolicy unknown_tolerance({true,true,1000,4200});
    assert(!unknown_tolerance.update(good(),1000).charge_permitted);
    for (unsigned error=1;error<=1000;++error) {
        ChargePolicy bounded({true,true,1000,4200,static_cast<std::uint16_t>(error),4,true});
        const auto result=bounded.update(good(),1000);
        assert(unsigned(result.charge_limit_ma)*(1000+error)<=1000000);
    }
    ChargePolicy unknown_voltage_error({true,true,1000,4200,250,0,true});
    assert(!unknown_voltage_error.update(good(),1000).charge_permitted);
    ChargePolicy unreviewed_stages({true,true,1000,4200,250,4,false});
    assert(!unreviewed_stages.update(good(),1000).charge_permitted);
    for (unsigned error=1;error<=100;++error) {
        ChargePolicy bounded({true,true,1000,4200,250,static_cast<std::uint16_t>(error),true});
        o=good(); o.readback_charge_mv=static_cast<std::uint16_t>((4200000/(1000+error)/10)*10);
        const auto result=bounded.update(o,1000);
        assert(result.charge_permitted && result.charge_voltage_mv%10==0);
        assert(unsigned(result.charge_voltage_mv)*(1000+error)<=4200000);
        ++o.readback_charge_mv;
        assert(!bounded.update(o,1000).charge_permitted);
    }
    o=good(); o.readback_charge_mv=4200; // POR setting is not qualified target
    assert(!policy.update(o,1000).charge_permitted);
    std::puts("Charge policy permission, bounds, freshness and latched-fault tests passed");
}
