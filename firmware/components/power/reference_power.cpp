#include "nightwave/reference_power.h"
namespace nightwave {
namespace {
std::uint16_t be(const std::uint8_t* p) { return (std::uint16_t(p[0])<<8)|p[1]; }
std::uint16_t le(const std::uint8_t* p) { return (std::uint16_t(p[1])<<8)|p[0]; }
}
PowerReading ReferencePower::sample(std::uint32_t now) {
    PowerReading result{}; result.sampled_ms=now;
    diagnostic_={}; // Never reuse old readings following a failed transfer.
    std::uint8_t version[2]{}, config[2]{}, cell[2]{}, soc[2]{};
    // MAX17048 register words are MSB first. Reject sleep/unexpected version.
    // VERSION alone cannot distinguish MAX17048 from MAX17049; BOM must match.
    if (bus_.read(0x36,0x08,version,2) && (be(version)&0xfff0)==0x0010 &&
        bus_.read(0x36,0x0c,config,2) && !(be(config)&0x0080) &&
        bus_.read(0x36,0x02,cell,2)) {
        const auto mv=static_cast<std::uint16_t>((std::uint32_t(be(cell))*5+32)/64);
        if (mv>=2500 && mv<=4500) {
            result.state.battery_millivolts=mv;
            result.voltage_valid=true;
            // SOC is a model estimate, not a capacity or runtime measurement.
            if (now>=1000 && bus_.read(0x36,0x04,soc,2) && be(soc)<=25600) {
                result.state.state_of_charge_percent=static_cast<std::uint8_t>(be(soc)/256);
                result.charge_percent_valid=true;
            }
        }
    }
    std::uint8_t id{}, status[3]{}, limits[6]{};
    // Read status, not read-to-clear flags. BQ words are LITTLE endian.
    if (bus_.read(0x6a,0x38,&id,1) && (id&0xf8)==0x20 &&
        bus_.read(0x6a,0x1d,status,3) && bus_.read(0x6a,0x02,limits,6)) {
        const auto vbus=status[1]&7;
        if (!(status[1]&0xe0) && (vbus==0 || vbus==4)) {
            diagnostic_={true,status[0],status[1],status[2],
                static_cast<std::uint16_t>(((le(limits)>>5)&0x3f)*40),
                static_cast<std::uint16_t>(((le(limits+2)>>3)&0x1ff)*10),
                static_cast<std::uint16_t>(((le(limits+4)>>4)&0xff)*10)};
            // VBUS present does not establish whether the battery supplements it.
            if (vbus==4) result.state.source=PowerSource::kUsb;
            else if (result.voltage_valid) result.state.source=PowerSource::kBattery;
            result.state.charging=((status[1]>>3)&3)!=0;
            result.charging_valid=true;
        }
    }
    return result;
}
}
