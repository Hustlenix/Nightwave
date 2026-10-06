#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include "freertos/semphr.h"
#include "nightwave/board_i2c.h"
#include "nightwave/hardware_config.h"
#include "nightwave/power_i2c.h"
#include "nightwave/tca9535_i2c.h"
namespace {
std::timed_mutex guard;
bool mutex_fail=true,bus_fail=true,add_fail=true,transfer_fail=false;
std::atomic<bool> held{false};
int bus_token{},device_tokens[3]{},creates{},adds{},removes{},transfers{},delays{};
std::uint8_t last_reg{};
}
SemaphoreHandle_t xSemaphoreCreateMutex() { return mutex_fail?nullptr:&guard; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t h,TickType_t ticks) {
    assert(h==&guard && ticks==20);
    if (!guard.try_lock_for(std::chrono::milliseconds(ticks))) return 0;
    assert(!held.exchange(true)); return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t h) {
    assert(h==&guard && held.exchange(false)); guard.unlock(); return pdTRUE;
}
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t* cfg,i2c_master_bus_handle_t* out) {
    assert(held && cfg->sda_io_num==8 && cfg->scl_io_num==9 && cfg->glitch_ignore_cnt==7);
    ++creates;
    if (bus_fail) return ESP_ERR_TIMEOUT;
    *out=&bus_token; return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t b,const i2c_device_config_t* cfg,i2c_master_dev_handle_t* out) {
    assert(held && b==&bus_token && cfg->scl_speed_hz==100000 && cfg->dev_addr_length==I2C_ADDR_BIT_LEN_7);
    ++adds;
    if (add_fail) return ESP_ERR_TIMEOUT;
    const auto a=cfg->device_address;
    const int i=a==0x20?0:a==0x36?1:a==0x6a?2:-1;
    assert(i>=0); *out=&device_tokens[i]; return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t) { ++removes; return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t,const std::uint8_t*,std::size_t,int) {
    assert(false && "Power tests must never write registers"); return ESP_ERR_TIMEOUT;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t d,const std::uint8_t* reg,
    std::size_t n,std::uint8_t* bytes,std::size_t count,int timeout) {
    assert((d==&device_tokens[1] || d==&device_tokens[2]) && n==1 && timeout==10 && count<=6);
    assert(delays==transfers+1); ++transfers; last_reg=*reg;
    // Simulate a HAL that modifies the receive buffer even when returning error.
    for (std::size_t i=0;i<count;++i) bytes[i]=static_cast<std::uint8_t>(i+1);
    return transfer_fail?ESP_ERR_TIMEOUT:ESP_OK;
}
void esp_rom_delay_us(std::uint32_t us) { assert(us==100); ++delays; }
int main() {
    using namespace nightwave;
    assert(!reference_i2c_bus() && !reference_i2c_device(0x36));
    if (!hardware::kReferenceBoard) {
        assert(prepare_board_i2c());
        assert(!reference_i2c_bus() && !reference_i2c_device(0x36));
        assert(!creates && !adds);
        std::puts("Bench profile leaves board I2C untouched"); return 0;
    }
    assert(!prepare_board_i2c()); // Mutex allocation failure.
    mutex_fail=false;
    assert(!prepare_board_i2c()); // Bus creation failure, can recover.
    assert(!reference_i2c_device(0x36));
    bus_fail=false;
    assert(prepare_board_i2c());
    const auto successful_bus_creates=creates;
    assert(!reference_i2c_device(0x36)); // Device creation failure.
    add_fail=false; adds=0;
    std::array<std::thread,12> threads;
    for (unsigned t=0;t<threads.size();++t) threads[t]=std::thread([t] {
        const unsigned addresses[]{0x20,0x36,0x6a};
        for (unsigned i=0;i<100;++i) {
            const auto index=(i+t)%3;
            assert(reference_i2c_device(addresses[index])==&device_tokens[index]);
        }
    });
    for (auto& t:threads) t.join();
    assert(adds==3 && creates==successful_bus_creates);
    for (unsigned address=0;address<256;++address)
        if (address!=0x20 && address!=0x36 && address!=0x6a) assert(!reference_i2c_device(address));
    guard.lock();
    std::thread timeout([] { assert(!reference_i2c_device(0x36)); });
    timeout.join(); guard.unlock();
    assert(reference_i2c_device(0x36)); // Lock failure does not poison registry.
    {
        Tca9535I2c input;
        assert(!input.attach_borrowed(nullptr));
        assert(input.attach_borrowed(reference_i2c_device(0x20)));
        assert(!input.attach_borrowed(reference_i2c_device(0x20)));
        assert(input.detach());
        assert(input.attach_borrowed(reference_i2c_device(0x20)));
    }
    assert(!removes); // A borrowed adapter must not invalidate another device.
    PowerI2c power;
    std::array<std::uint8_t,7> data{}; data.fill(0xee);
    assert(!power.read(0x20,0,data.data(),2));
    assert(!power.read(0x36,0,nullptr,2));
    assert(!power.read(0x36,0,data.data(),0));
    assert(!power.read(0x36,0,data.data(),7));
    assert(!transfers);
    transfer_fail=true;
    assert(!power.read(0x36,8,data.data(),2));
    for (auto b:data) assert(b==0xee);
    transfer_fail=false;
    assert(power.read(0x36,8,data.data(),2) && last_reg==8 && data[0]==1 && data[2]==0xee);
    assert(power.read(0x6a,2,data.data(),6) && data[5]==6 && data[6]==0xee);
    assert(adds==3 && !removes && delays==transfers);
    std::puts("Shared I2C concurrent registration, retries, borrowed lifetime and actual power adapter tests passed");
}
