#include "nightwave/power_i2c.h"
#include "nightwave/board_i2c.h"
#include "esp_rom_sys.h"
namespace nightwave {
bool PowerI2c::read(std::uint8_t address,std::uint8_t reg,
                    std::uint8_t* data,std::size_t count) {
    if (!data || !count || count>6 || (address!=0x36 && address!=0x6a)) return false;
    const auto device=reference_i2c_device(address);
    if (!device) return false;
    std::uint8_t received[6]{};
    // BQ25628E section 8.5.1: 100 kHz plus 90 us minimum between STARTs,
    // including after NACK. Extra guard also covers consecutive polling calls.
    esp_rom_delay_us(100);
    if (i2c_master_transmit_receive(device,&reg,1,received,count,10)!=ESP_OK) return false;
    for (std::size_t i=0;i<count;++i) data[i]=received[i];
    return true;
}
}
