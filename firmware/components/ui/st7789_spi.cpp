#include "nightwave/st7789_spi.h"
#include "nightwave/reference_board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
namespace nightwave {
bool St7789Spi::connect() {
    if (device_) return true;
    namespace B = reference_board;
    gpio_config_t pins{};
    pins.pin_bit_mask=(1ULL<<B::kDisplayDc)|(1ULL<<B::kDisplayReset)|(1ULL<<B::kDisplayBacklight);
    pins.mode=GPIO_MODE_OUTPUT; pins.pull_down_en=GPIO_PULLDOWN_ENABLE;
    if (gpio_config(&pins)!=ESP_OK) return false;
    pins_ready_=true;
    if (!backlight(false)) return false;
    spi_bus_config_t cfg{};
    cfg.mosi_io_num=B::kDisplayMosi; cfg.sclk_io_num=B::kDisplayClock;
    cfg.miso_io_num=-1; cfg.quadwp_io_num=-1; cfg.quadhd_io_num=-1;
    cfg.max_transfer_sz=SelectedDisplayProfile::tile_bytes;
    if (spi_bus_initialize(SPI2_HOST,&cfg,SPI_DMA_CH_AUTO)!=ESP_OK) return false;
    bus_owned_=true;
    spi_device_interface_config_t dev{};
    dev.clock_speed_hz=20*1000*1000; dev.mode=0;
    dev.spics_io_num=B::kDisplayCs; dev.queue_size=1;
    if (spi_bus_add_device(SPI2_HOST,&dev,&device_)!=ESP_OK) { disconnect(); return false; }
    return true;
}
void St7789Spi::disconnect() {
    backlight(false);
    if (device_) { spi_bus_remove_device(device_); device_=nullptr; }
    if (bus_owned_) { spi_bus_free(SPI2_HOST); bus_owned_=false; }
}
bool St7789Spi::reset(bool high) {
    return pins_ready_ && gpio_set_level(static_cast<gpio_num_t>(reference_board::kDisplayReset),high)==ESP_OK;
}
void St7789Spi::delay_ms(std::uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
bool St7789Spi::send(bool data, const std::uint8_t* bytes, std::size_t size) {
    if (!device_ || !bytes || !size || size>SelectedDisplayProfile::tile_bytes) return false;
    if (gpio_set_level(static_cast<gpio_num_t>(reference_board::kDisplayDc),data)!=ESP_OK) return false;
    spi_transaction_t tx{}; tx.length=size*8;
    // Small stack command payloads copied inline; tile storage is stable until
    // synchronous completion. SPI2 is dedicated to this single UiTask owner.
    if (size<=4) {
        tx.flags=SPI_TRANS_USE_TXDATA;
        for (std::size_t i=0;i<size;++i) tx.tx_data[i]=bytes[i];
    } else tx.tx_buffer=bytes;
    return spi_device_transmit(device_,&tx)==ESP_OK;
}
bool St7789Spi::command(std::uint8_t code,const std::uint8_t* data,std::size_t size) {
    if (size>14 || (size && !data)) return false;
    return send(false,&code,1) && (!size || send(true,data,size));
}
bool St7789Spi::pixels(const std::uint8_t* bytes,std::size_t size) { return send(true,bytes,size); }
bool St7789Spi::backlight(bool on) {
    return pins_ready_ && gpio_set_level(static_cast<gpio_num_t>(reference_board::kDisplayBacklight),on)==ESP_OK;
}
} // namespace nightwave
