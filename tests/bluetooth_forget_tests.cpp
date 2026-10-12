#include "nightwave/bluetooth_source.h"
#include <cassert>
#include <cstdio>
using namespace nightwave;
struct Backend final : BluetoothBackend {
    bool supported=true; unsigned requests=0;
    BtCapabilities capabilities() const override { return {true,false,true,true}; }
    bool discover(std::uint32_t) override { return true; }
    bool connect(const BtDevice&,std::uint32_t) override { return true; }
    bool start(const AudioFormat&,std::uint32_t) override { return true; }
    void disconnect(std::uint32_t) override {}
    BtWrite write(const PcmBlock&) override { return BtWrite::kAccepted; }
    bool forget(const BtDevice&,std::uint32_t) override { ++requests; return supported; }
};
int main() {
    Backend backend; BluetoothSource source(backend); BtDevice device;
    assert(!source.forget(device,0)); device.address[0]=1;
    assert(source.forget(device,0)); const auto first=source.epoch();
    assert(source.state()==BtState::kForgetting);
    assert(!source.take_forget_confirmation());
    assert(!source.forgotten(first+1)); assert(!source.reconnect(device,0));
    assert(!source.discover(0)); assert(!source.forget(device,0));
    assert(source.forgotten(first)); assert(source.take_forget_confirmation());
    assert(!source.take_forget_confirmation()); assert(!source.forgotten(first));
    assert(source.forget(device,0xfffffff0u)); source.tick(30);
    assert(source.state()==BtState::kForgetting);
    source.tick(4984); assert(source.state()==BtState::kFault);
    assert(!source.take_forget_confirmation());
    assert(!source.forgotten(first));
    backend.supported=false; assert(!source.forget(device,100));
    assert(!source.take_forget_confirmation()); backend.supported=true;
    assert(source.forget(device,100)); auto epoch=source.epoch();
    source.disconnect(); assert(!source.forgotten(epoch));
    assert(source.forget(device,200)); assert(source.forgotten(source.epoch()));
    assert(source.reconnect(device,200)); assert(!source.take_forget_confirmation());
    assert(source.connected(source.epoch())); assert(!source.forget(device,200));
    UnavailableBluetooth unavailable; BluetoothSource absent(unavailable);
    assert(!absent.forget(device,0)); assert(!absent.take_forget_confirmation());
    std::puts("Bluetooth forget confirmation, unsupported, stale and timeout tests passed");
}
