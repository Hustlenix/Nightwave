#pragma once
#include <cstddef>
#include <cstdint>
#include "driver/gpio.h"
using i2s_chan_handle_t = void*;
inline constexpr int I2S_NUM_0 = 0, I2S_ROLE_MASTER = 1;
inline constexpr int I2S_DATA_BIT_WIDTH_16BIT = 16, I2S_SLOT_MODE_STEREO = 2;
inline constexpr int I2S_GPIO_UNUSED = -1;
struct i2s_chan_config_t {
    std::size_t dma_desc_num{}, dma_frame_num{};
    bool auto_clear_after_cb{};
};
struct i2s_std_config_t {
    std::uint32_t clk_cfg{};
    int slot_cfg{};
    struct {
        int mclk{}, bclk{}, ws{}, dout{}, din{};
        struct { bool mclk_inv{}, bclk_inv{}, ws_inv{}; } invert_flags;
    } gpio_cfg;
};
#define I2S_CHANNEL_DEFAULT_CONFIG(port, role) i2s_chan_config_t{}
#define I2S_STD_CLK_DEFAULT_CONFIG(rate) (rate)
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits, mode) (mode)
esp_err_t i2s_new_channel(const i2s_chan_config_t*, i2s_chan_handle_t*, void*);
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t, const i2s_std_config_t*);
esp_err_t i2s_channel_preload_data(i2s_chan_handle_t, const void*, std::size_t, std::size_t*);
esp_err_t i2s_channel_enable(i2s_chan_handle_t);
esp_err_t i2s_channel_disable(i2s_chan_handle_t);
esp_err_t i2s_del_channel(i2s_chan_handle_t);
esp_err_t i2s_channel_write(i2s_chan_handle_t, const void*, std::size_t,
                            std::size_t*, std::uint32_t);
