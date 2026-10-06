#include "nightwave/reference_power.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace nightwave;
struct Bus final : PowerRegisterBus {
    std::array<std::uint8_t,256> gauge{},charger{};
    int calls=0, fail=0;
    Bus() {
        gauge[9]=0x12;
        gauge[2]=0xb9; gauge[3]=0; // 3700 mV: 47360 * 0.078125 mV.
        gauge[4]=73; gauge[5]=128;
        charger[0x38]=0x22;
        charger[0x1e]=0x0c; // VBUS + constant-current charging.
        charger[3]=1; charger[4]=0x20; charger[5]=0x0d; charger[7]=0x0a;
    }
    bool read(std::uint8_t a,std::uint8_t r,std::uint8_t* p,std::size_t n) override {
        ++calls; if (calls==fail) return false;
        assert(a==0x36 || a==0x6a);
        const auto& source=a==0x36?gauge:charger;
        assert(n && n<=6 && r+n<=source.size());
        for (std::size_t i=0;i<n;++i) p[i]=source[r+i];
        return true;
    }
};
int main() {
    Bus bus; ReferencePower power(bus);
    auto r=power.sample(1000);
    assert(r.voltage_valid && r.state.battery_millivolts==3700);
    assert(r.charge_percent_valid && r.state.state_of_charge_percent==73);
    assert(r.charging_valid && r.state.charging && r.state.source==PowerSource::kUsb);
    assert(r.sampled_ms==1000 && !power.request_shutdown());
    auto d=power.diagnostic();
    assert(d.valid && d.charge_limit_ma==320 && d.voltage_limit_mv==4200 && d.input_limit_ma==1600);
    assert(!power.sample(999).charge_percent_valid);
    bus.gauge[4]=101; assert(!power.sample(2000).charge_percent_valid);
    bus.gauge[4]=100; bus.gauge[5]=0; assert(power.sample(2000).charge_percent_valid);
    bus.gauge[13]=0x80; assert(!power.sample(2000).voltage_valid);
    bus.gauge[13]=0; bus.gauge[2]=0xff; bus.gauge[3]=0xff;
    assert(!power.sample(2000).voltage_valid);
    bus.gauge[2]=0; bus.gauge[3]=0; assert(!power.sample(2000).voltage_valid);
    bus=Bus{}; bus.gauge[9]=0xff; assert(!power.sample(2000).voltage_valid);
    bus=Bus{}; bus.charger[0x38]=0xff; assert(!power.sample(2000).charging_valid);
    assert(!power.diagnostic().valid);
    bus=Bus{}; bus.charger[0x1e]=0; r=power.sample(2000);
    assert(r.charging_valid && !r.state.charging && r.state.source==PowerSource::kBattery);
    for (unsigned raw=0;raw<256;++raw) {
        bus=Bus{}; bus.charger[0x1e]=static_cast<std::uint8_t>(raw);
        r=power.sample(2000);
        const bool valid=!(raw&0xe0) && ((raw&7)==0 || (raw&7)==4);
        assert(r.charging_valid==valid);
    }
    for (int failure=1; failure<=7; ++failure) {
        bus=Bus{}; power.sample(2000); // Prior success must not leak stale fields.
        bus.calls=0; bus.fail=failure; r=power.sample(2100);
        if (failure<=3) assert(!r.voltage_valid && !r.charge_percent_valid);
        if (failure==4) assert(r.voltage_valid && !r.charge_percent_valid);
        if (failure>=5) assert(!r.charging_valid && !power.diagnostic().valid);
        bus.fail=0; assert(power.sample(2200).voltage_valid);
    }
    std::puts("Reference power register, bounds, endianness and failure tests passed");
}
