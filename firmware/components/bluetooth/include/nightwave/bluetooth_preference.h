#pragma once
#include "nightwave/bluetooth_source.h"
namespace nightwave {
// Persist only a preferred endpoint, never link keys or a claim of connection.
struct BluetoothPreference { BtDevice device{}; bool enabled{false}; };
class BluetoothPreferenceCodec {
 public:
    using Record = std::array<std::uint8_t, 64>;
    static bool encode(const BluetoothPreference& p, Record& out) {
        if (p.enabled && (p.device.address == std::array<std::uint8_t, 6>{} ||
            !std::memchr(p.device.name.data(), 0, p.device.name.size()))) return false;
        Record r{}; r[0]='N'; r[1]='B'; r[2]=1; r[3]=p.enabled ? 1 : 0;
        if (p.enabled) {
            std::memcpy(r.data()+4, p.device.address.data(), 6);
            std::memcpy(r.data()+10, p.device.name.data(), 48);
        }
        const auto c = checksum(r);
        for (unsigned i=0;i<4;++i) r[60+i]=static_cast<std::uint8_t>(c>>(8*i));
        out=r; return true;
    }
    static bool decode(const Record& r, BluetoothPreference& out) {
        if (r[0]!='N' || r[1]!='B' || r[2]!=1 || r[3]>1 || r[58] || r[59]) return false;
        std::uint32_t c=0;
        for (unsigned i=0;i<4;++i) c |= std::uint32_t(r[60+i])<<(8*i);
        if(c!=checksum(r)) return false;
        BluetoothPreference p; p.enabled=r[3]!=0;
        std::memcpy(p.device.address.data(), r.data()+4, 6);
        std::memcpy(p.device.name.data(), r.data()+10, 48);
        Record canonical{};
        if(!encode(p, canonical) || canonical!=r) return false;
        out=p; return true;
    }
 private:
    static std::uint32_t checksum(const Record& r) {
        std::uint32_t c=2166136261u;
        for(unsigned i=0;i<60;++i) c=(c^r[i])*16777619u;
        return c;
    }
};
bool load_bluetooth_preference(BluetoothPreference&);
bool save_bluetooth_preference(const BluetoothPreference&);
} // namespace nightwave
