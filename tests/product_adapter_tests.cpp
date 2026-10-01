#include <cstdlib>
#include <iostream>
#include <vector>
#include "driver/i2c_master.h"
#include "nvs.h"
#include "nightwave/oled_display.h"
#include "nightwave/settings.h"
namespace {
int failures = 0, i2c_fail = 0, removed = 0, deleted = 0;
int nvs_error = 0, closes = 0, commits = 0;
std::uint8_t stored = 8;
std::vector<std::vector<std::uint8_t>> packets;
#define CHECK(value) do { if (!(value)) { ++failures; std::cerr << __LINE__ << ": " << #value << '\n'; } } while(false)
}
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t* config, i2c_master_bus_handle_t* bus) {
    CHECK(config->sda_io_num == 8 && config->scl_io_num == 9);
    if (i2c_fail == 1) return 2;
    *bus = reinterpret_cast<void*>(1); return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t, const i2c_device_config_t* config, i2c_master_dev_handle_t* device) {
    CHECK(config->device_address == 0x3c && config->scl_speed_hz == 400000);
    if (i2c_fail == 2) return 2;
    *device = reinterpret_cast<void*>(2); return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t) { ++removed; return ESP_OK; }
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t) { ++deleted; return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const std::uint8_t* data, std::size_t size, int timeout) {
    CHECK(timeout == 100);
    packets.emplace_back(data, data + size); return i2c_fail == 3 ? 2 : ESP_OK;
}
esp_err_t nvs_flash_init() { return nvs_error == 1 ? 2 : ESP_OK; }
esp_err_t nvs_open(const char*, int, nvs_handle_t* handle) {
    *handle = 1; return nvs_error == 2 ? 2 : ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t, const char*, std::uint8_t* value) {
    *value = stored; return nvs_error == 3 ? 2 : ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t, const char*, std::uint8_t value) {
    if (nvs_error == 4) return 2;
    stored = value; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) { ++commits; return nvs_error == 5 ? 2 : ESP_OK; }
void nvs_close(nvs_handle_t) { ++closes; }
int main() {
    using namespace nightwave;
    TextFrame text; text.line(0, "NIGHTWAVE");
    for (const auto controller : {OledController::kSh1106, OledController::kSsd1306}) {
        packets.clear(); OledDisplay display;
        CHECK(!display.show(text)); CHECK(display.initialize(controller));
        CHECK(packets[0][0] == 0 && packets[0][1] == 0xae);
        packets.clear(); CHECK(display.show(text));
        const auto pixels = rasterize(text);
        const std::size_t first_data = controller == OledController::kSh1106 ? 1 : 1;
        CHECK(packets[first_data][0] == 0x40);
        CHECK(packets[first_data].size() == 129);
        CHECK(std::equal(pixels.begin(), pixels.begin() + 128, packets[first_data].begin() + 1));
        if (controller == OledController::kSh1106) {
            CHECK(packets.size() == 16);
            for (std::size_t page = 0; page < 8; ++page)
                CHECK(packets[page * 2] == std::vector<std::uint8_t>({0, static_cast<std::uint8_t>(0xb0 + page), 2, 0x10}));
        } else CHECK(packets.size() == 9 && packets[0] == std::vector<std::uint8_t>({0,0x21,0,127,0x22,0,7}));
        CHECK(display.sleep(true)); CHECK(packets.back() == std::vector<std::uint8_t>({0,0xae}));
        CHECK(display.sleep(false)); CHECK(packets.back() == std::vector<std::uint8_t>({0,0xaf}));
        i2c_fail = 3; CHECK(!display.show(text)); i2c_fail = 0;
    }
    for (int failed = 1; failed <= 3; ++failed) {
        const int before_deleted = deleted, before_removed = removed;
        i2c_fail = failed; OledDisplay display; CHECK(!display.initialize());
        CHECK(deleted == before_deleted + (failed >= 2 ? 1 : 0));
        CHECK(removed == before_removed + (failed == 3 ? 1 : 0));
        CHECK(!display.sleep(false));
    }
    i2c_fail = 0;
    Settings value; CHECK(initialize_settings()); CHECK(load_settings(value) && value.volume_percent == 8);
    value.volume_percent = 14; CHECK(save_settings(value)); value.volume_percent = 8;
    CHECK(load_settings(value) && value.volume_percent == 14);
    stored = 255; value.volume_percent = 8; CHECK(!load_settings(value)); CHECK(value.volume_percent == 8);
    for (int error = 1; error <= 5; ++error) {
        nvs_error = error;
        if (error == 1) CHECK(!initialize_settings());
        else if (error == 2) { const int before = closes; CHECK(!load_settings(value)); CHECK(closes == before); }
        else if (error == 3) { const int before = closes; CHECK(!load_settings(value)); CHECK(closes == before + 1); }
        else { const int before = closes, committed = commits; CHECK(!save_settings(value)); CHECK(closes == before + 1); CHECK(commits == committed + (error == 5 ? 1 : 0)); }
    }
    nvs_error = 0; value.volume_percent = 255; CHECK(!save_settings(value));
    if (failures) return EXIT_FAILURE;
    std::cout << "Actual OLED and NVS adapters passed mocked-bus error/addressing/persistence tests\n";
}
