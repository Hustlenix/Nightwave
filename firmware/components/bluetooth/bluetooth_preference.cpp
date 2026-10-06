#include "nightwave/bluetooth_preference.h"
#include "nvs.h"
namespace nightwave {
bool load_bluetooth_preference(BluetoothPreference& out) {
    nvs_handle_t h;
    if(nvs_open("nightwave", NVS_READONLY, &h)!=ESP_OK) return false;
    BluetoothPreferenceCodec::Record r{}; std::size_t size=r.size();
    const auto result=nvs_get_blob(h,"bt_pref_v1",r.data(),&size);
    nvs_close(h);
    return result==ESP_OK && size==r.size() && BluetoothPreferenceCodec::decode(r,out);
}
bool save_bluetooth_preference(const BluetoothPreference& p) {
    BluetoothPreferenceCodec::Record r{};
    if(!BluetoothPreferenceCodec::encode(p,r)) return false;
    nvs_handle_t h;
    if(nvs_open("nightwave",NVS_READWRITE,&h)!=ESP_OK) return false;
    bool ok=nvs_set_blob(h,"bt_pref_v1",r.data(),r.size())==ESP_OK;
    if(ok) ok=nvs_commit(h)==ESP_OK;
    nvs_close(h); return ok;
}
}
