#include "nightwave/audio_sink.h"

#include <array>
#include <cstring>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "nightwave/hardware_config.h"

namespace nightwave {
namespace {
constexpr char kTag[] = "audio_i2s";

i2s_chan_handle_t as_channel(void* value) {
    return static_cast<i2s_chan_handle_t>(value);
}

void set_output_pins(bool speaker, bool line) {
    gpio_set_level(static_cast<gpio_num_t>(hardware::kSpeakerEnable), speaker);
    gpio_set_level(static_cast<gpio_num_t>(hardware::kHeadphoneEnable), line);
}
}  // namespace

bool I2sAudioSink::initialize_safe_outputs() {
    gpio_config_t enable_config{};
    enable_config.pin_bit_mask =
        (1ULL << hardware::kSpeakerEnable) |
        (1ULL << hardware::kHeadphoneEnable);
    enable_config.mode = GPIO_MODE_OUTPUT;
    enable_config.pull_up_en = GPIO_PULLUP_DISABLE;
    enable_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    enable_config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&enable_config) != ESP_OK) return false;
    set_output_pins(false, false);
    output_ = OutputPath::kMuted;
    return true;
}

AudioSinkStatus I2sAudioSink::configure(const AudioFormat& format) {
    if (!format.supported()) return AudioSinkStatus::kFault;
    stop();
    if (!initialize_safe_outputs()) return AudioSinkStatus::kFault;

    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    channel_config.dma_desc_num = 8;
    channel_config.dma_frame_num = 256;
    i2s_chan_handle_t tx = nullptr;
    if (i2s_new_channel(&channel_config, &tx, nullptr) != ESP_OK) {
        ESP_LOGE(kTag, "i2s_new_channel failed");
        return AudioSinkStatus::kFault;
    }

    i2s_std_config_t standard_config{};
    standard_config.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(format.sample_rate_hz);
    standard_config.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    standard_config.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    standard_config.gpio_cfg.bclk =
        static_cast<gpio_num_t>(hardware::kI2sBitClock);
    standard_config.gpio_cfg.ws =
        static_cast<gpio_num_t>(hardware::kI2sWordSelect);
    standard_config.gpio_cfg.dout =
        static_cast<gpio_num_t>(hardware::kI2sDataOut);
    standard_config.gpio_cfg.din = I2S_GPIO_UNUSED;
    standard_config.gpio_cfg.invert_flags.mclk_inv = false;
    standard_config.gpio_cfg.invert_flags.bclk_inv = false;
    standard_config.gpio_cfg.invert_flags.ws_inv = false;

    if (i2s_channel_init_std_mode(tx, &standard_config) != ESP_OK ||
        i2s_channel_enable(tx) != ESP_OK) {
        i2s_del_channel(tx);
        ESP_LOGE(kTag, "I2S standard-mode initialization failed");
        return AudioSinkStatus::kFault;
    }
    channel_ = tx;
    format_ = format;

    std::array<std::int16_t, 512> silence{};
    std::size_t bytes_written = 0;
    i2s_channel_write(tx, silence.data(), sizeof(silence), &bytes_written,
                      pdMS_TO_TICKS(100));
    ESP_LOGI(kTag, "I2S READY rate=%lu bits=16 channels=2 dma=8x256 output=muted",
             static_cast<unsigned long>(format.sample_rate_hz));
    return AudioSinkStatus::kAccepted;
}

AudioSinkStatus I2sAudioSink::write(const PcmBlock& block) {
    if (channel_ == nullptr || block.interleaved_samples == nullptr) {
        return AudioSinkStatus::kNotReady;
    }
    const std::size_t byte_count =
        block.frame_count * 2U * sizeof(std::int16_t);
    std::size_t bytes_written = 0;
    const esp_err_t error = i2s_channel_write(
        as_channel(channel_), block.interleaved_samples, byte_count,
        &bytes_written, pdMS_TO_TICKS(1000));
    if (error == ESP_ERR_TIMEOUT) return AudioSinkStatus::kWouldBlock;
    return error == ESP_OK && bytes_written == byte_count
               ? AudioSinkStatus::kAccepted
               : AudioSinkStatus::kFault;
}

bool I2sAudioSink::select_output(OutputPath output) {
    if (channel_ == nullptr && output != OutputPath::kMuted) return false;
    set_output_pins(false, false);
    output_ = OutputPath::kMuted;
    if (output == OutputPath::kSpeaker) {
        set_output_pins(true, false);
    } else if (output == OutputPath::kLine) {
        set_output_pins(false, true);
    }
    output_ = output;
    ESP_LOGI(kTag, "output=%s",
             output == OutputPath::kSpeaker
                 ? "speaker"
                 : (output == OutputPath::kLine ? "line" : "muted"));
    return true;
}

void I2sAudioSink::stop() {
    set_output_pins(false, false);
    output_ = OutputPath::kMuted;
    if (channel_ != nullptr) {
        i2s_channel_disable(as_channel(channel_));
        i2s_del_channel(as_channel(channel_));
        channel_ = nullptr;
    }
}

}  // namespace nightwave
