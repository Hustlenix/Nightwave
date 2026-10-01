#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "driver/i2s_std.h"
#include "nightwave/audio_sink.h"

namespace {
int failures = 0;
bool fail_new = false, fail_init = false, fail_enable = false;
bool fail_preload = false, partial_preload = false, partial_write = false;
int write_error = ESP_OK, deletes = 0, writes = 0;
std::uint32_t last_timeout = 0;
std::size_t last_bytes = 0;
std::string calls;
std::array<int, 64> levels{};
int channel_token = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; \
    std::cerr << __LINE__ << ": " << #expr << '\n'; } } while (false)
}

esp_err_t gpio_config(const gpio_config_t*) { return ESP_OK; }
esp_err_t gpio_set_level(gpio_num_t pin, int value) {
    levels.at(static_cast<std::size_t>(pin)) = value;
    return ESP_OK;
}
esp_err_t i2s_new_channel(const i2s_chan_config_t* config,
                         i2s_chan_handle_t* tx, void*) {
    CHECK(config->dma_desc_num == 8 && config->dma_frame_num == 256);
    CHECK(config->auto_clear_after_cb);
    calls += 'N';
    if (fail_new) return 2;
    *tx = &channel_token;
    return ESP_OK;
}
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t, const i2s_std_config_t*) {
    calls += 'I'; return fail_init ? 2 : ESP_OK;
}
esp_err_t i2s_channel_preload_data(i2s_chan_handle_t, const void* data,
                                   std::size_t size, std::size_t* loaded) {
    calls += 'P';
    CHECK(size == 8192);
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < size; ++i) CHECK(bytes[i] == 0);
    *loaded = partial_preload ? size - 4 : size;
    return fail_preload ? 2 : ESP_OK;
}
esp_err_t i2s_channel_enable(i2s_chan_handle_t) {
    calls += 'E'; return fail_enable ? 2 : ESP_OK;
}
esp_err_t i2s_channel_disable(i2s_chan_handle_t) { calls += 'S'; return ESP_OK; }
esp_err_t i2s_del_channel(i2s_chan_handle_t) { calls += 'D'; ++deletes; return ESP_OK; }
esp_err_t i2s_channel_write(i2s_chan_handle_t, const void*, std::size_t size,
                            std::size_t* written, std::uint32_t timeout) {
    ++writes; last_timeout = timeout; last_bytes = size;
    *written = partial_write ? size - 1 : size;
    return write_error;
}

int main() {
    using namespace nightwave;
    I2sAudioSink sink;
    AudioFormat format{44100, 2, 16};
    std::array<std::int16_t, 4> pcm{};
    PcmBlock block{pcm.data(), 2, format, 0};
    CHECK(sink.write(block) == AudioSinkStatus::kNotReady);
    CHECK(sink.configure({44100, 1, 16}) == AudioSinkStatus::kFault);
    CHECK(sink.configure(format) == AudioSinkStatus::kAccepted);
    CHECK(calls == "NIPE");
    CHECK(levels[17] == 0 && levels[18] == 0);
    CHECK(sink.select_output(OutputPath::kSpeaker));
    CHECK(levels[17] == 1 && levels[18] == 0);
    CHECK(sink.select_output(OutputPath::kLine));
    CHECK(levels[17] == 0 && levels[18] == 1);
    CHECK(sink.write(block) == AudioSinkStatus::kAccepted);
    CHECK(last_timeout == 1000 && last_bytes == 8);
    block.format.sample_rate_hz = 48000;
    const auto before = writes;
    CHECK(sink.write(block) == AudioSinkStatus::kFault && writes == before);
    block.format = format;
    partial_write = true;
    CHECK(sink.write(block) == AudioSinkStatus::kFault);
    partial_write = false; write_error = ESP_ERR_TIMEOUT;
    CHECK(sink.write(block) == AudioSinkStatus::kWouldBlock);
    write_error = ESP_OK;
    CHECK(sink.drain() == AudioSinkStatus::kAccepted && last_bytes == 8192);
    sink.stop();
    CHECK(!sink.ready() && levels[17] == 0 && levels[18] == 0);
    const auto deleted = deletes;
    sink.stop(); CHECK(deletes == deleted);
    for (int stage = 0; stage < 5; ++stage) {
        fail_new = stage == 0; fail_init = stage == 1;
        fail_preload = stage == 2; partial_preload = stage == 3;
        fail_enable = stage == 4;
        const auto old_deletes = deletes;
        CHECK(sink.configure(format) == AudioSinkStatus::kFault);
        CHECK(!sink.ready() && levels[17] == 0 && levels[18] == 0);
        CHECK(deletes == old_deletes + (stage == 0 ? 0 : 1));
    }
    if (failures) return EXIT_FAILURE;
    std::cout << "Nightwave I2S adapter mock tests passed\n";
}
