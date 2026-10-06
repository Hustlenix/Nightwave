#include <cassert>
#include <cstring>
#include "nightwave/bluetooth_preference.h"
#include "nvs.h"
namespace {
nightwave::BluetoothPreferenceCodec::Record disk{}, staged{};
bool fail_open=false,fail_read=false,fail_write=false,fail_commit=false;
std::size_t disk_size=64; unsigned closes=0,commits=0;
struct Backend:nightwave::BluetoothBackend {
    nightwave::BtCapabilities capabilities() const override{return {true,false,true,true};}
    bool discover(std::uint32_t) override{return true;}
    bool connect(const nightwave::BtDevice&,std::uint32_t) override{return true;}
    bool start(const nightwave::AudioFormat&,std::uint32_t) override{return true;}
    void disconnect(std::uint32_t) override{}
    nightwave::BtWrite write(const nightwave::PcmBlock&) override{return nightwave::BtWrite::kAccepted;}
};
}
esp_err_t nvs_open(const char*,int,nvs_handle_t* h){*h=1;return fail_open?-1:ESP_OK;}
esp_err_t nvs_get_blob(nvs_handle_t,const char* key,void* out,std::size_t* size){
    assert(std::strcmp(key,"bt_pref_v1")==0);
    std::memcpy(out,disk.data(),disk.size());*size=disk_size;return fail_read?-1:ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t,const char* key,const void* in,std::size_t size){
    assert(std::strcmp(key,"bt_pref_v1")==0 && size==64);
    if(fail_write)return -1;
    std::memcpy(staged.data(),in,size);return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t){++commits;if(fail_commit)return -1;disk=staged;return ESP_OK;}
void nvs_close(nvs_handle_t){++closes;}
int main(){
    using namespace nightwave;
    BluetoothPreference p; p.enabled=true;p.device.address[0]=1;
    std::strcpy(p.device.name.data(),"Test headphones");
    assert(save_bluetooth_preference(p));BluetoothPreference loaded;
    assert(load_bluetooth_preference(loaded));assert(loaded.enabled && loaded.device.address==p.device.address);
    const auto saved=disk;
    for(unsigned i=0;i<disk.size();++i){
        disk=saved;disk[i]^=1;
        assert(!load_bluetooth_preference(loaded));assert(loaded.enabled);
    }
    disk=saved;disk_size=63;assert(!load_bluetooth_preference(loaded));disk_size=64;
    fail_read=true;assert(!load_bluetooth_preference(loaded));fail_read=false;
    fail_open=true;assert(!save_bluetooth_preference(p));fail_open=false;
    fail_write=true;const auto before=commits;assert(!save_bluetooth_preference(p));assert(commits==before);fail_write=false;
    fail_commit=true;assert(!save_bluetooth_preference({}));assert(disk==saved);fail_commit=false;
    assert(save_bluetooth_preference({}));assert(load_bluetooth_preference(loaded));assert(!loaded.enabled);
    // Clearing this preference is NOT deleting the radio's own pairing database.
    p.device.name.fill('x');assert(!save_bluetooth_preference(p));p.device.name[0]=0;
    p.device.address.fill(0);assert(!save_bluetooth_preference(p));p.device.address[0]=1;
    Backend b;BluetoothSource source(b);
    assert(source.reconnect(p.device,UINT32_MAX-100));const auto epoch=source.epoch();
    assert(source.state()==BtState::kReconnecting);assert(!source.pairing(epoch-1));
    assert(source.pairing(epoch));assert(!source.connected(epoch-1));
    source.tick(10000);assert(source.state()==BtState::kFault && source.take_stop_request());
    assert(!source.connected(epoch));source.disconnect();
    assert(source.reconnect(p.device,1));assert(source.connected(source.epoch()));
    source.disconnect();assert(source.take_stop_request());
    UnavailableBluetooth absent;BluetoothSource unavailable(absent);
    assert(!unavailable.reconnect(p.device,0));
    assert(closes>60);
}
