#include <array>
#include <cstdlib>
#include <iostream>
#include "nightwave/hardware_config.h"
#include "nightwave/tca9535_i2c.h"

namespace {
void check(bool good) { if (!good) std::abort(); }
std::array<std::uint8_t, 8> registers{};
unsigned transactions{}, fail_at{}, adds{}, removes{};
bool add_failure{}, remove_failure{}, bad_configuration{}, bad_polarity{};
int bus_token{}, device_token{};
void reset() {
    registers = {0xff, 0xff, 0xff, 0xff, 0, 0, 0xff, 0xff};
    transactions = fail_at = adds = removes = 0;
    add_failure = remove_failure = bad_configuration = bad_polarity = false;
}
}  // namespace

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t* config,
                                  i2c_master_dev_handle_t* out) {
    check(bus == &bus_token && config && out);
    check(config->device_address >= 0x20 && config->device_address <= 0x27);
    check(config->dev_addr_length == I2C_ADDR_BIT_LEN_7 && config->scl_speed_hz == 400000);
    ++adds;
    if (add_failure) return ESP_ERR_TIMEOUT;
    *out = &device_token;
    return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t device) {
    check(device == &device_token);
    ++removes;
    return remove_failure ? ESP_ERR_TIMEOUT : ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t device, const std::uint8_t* packet,
                            std::size_t count, int timeout) {
    check(device == &device_token && packet && count == 3 && timeout == 10);
    check(packet[0] == 4 || packet[0] == 6);  // Never touch output registers.
    ++transactions;
    if (transactions == fail_at) return ESP_ERR_TIMEOUT;
    registers[packet[0]] = packet[1];
    registers[packet[0] + 1] = packet[2];
    return ESP_OK;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t device, const std::uint8_t* command,
    std::size_t command_count, std::uint8_t* data, std::size_t receive_count, int timeout) {
    check(device == &device_token && command && command_count == 1 && data && receive_count == 2 && timeout == 10);
    check(*command == 0 || *command == 4 || *command == 6);
    ++transactions;
    data[0] = registers[*command];  // Deliberately touch output even on failure.
    if (transactions == fail_at) return ESP_ERR_TIMEOUT;
    data[1] = registers[*command + 1];
    if (*command == 6 && bad_configuration) data[1] = 0;
    if (*command == 4 && bad_polarity) data[0] = 1;
    return ESP_OK;
}

int main() {
    using namespace nightwave;
    static_assert(hardware::kTca9535InputExpansionSelected && !hardware::kTca9535PhysicalQualified);
    static_assert(!hardware::kCombinedProductPinsReviewed && !hardware::kBatteryLocked);
    reset();
    {
        Tca9535I2c transport;
        std::uint8_t ports[]{0x12, 0x34};
        check(!transport.read_pair(0, ports) && ports[0] == 0x12 && ports[1] == 0x34);
        check(!transport.write_pair(6, 0xff, 0xff));
        check(!transport.attach(nullptr, 0x20));
        for (unsigned address = 0; address <= 255; ++address) {
            if (address >= 0x20 && address <= 0x27) continue;
            check(!transport.attach(&bus_token, static_cast<std::uint8_t>(address)));
        }
        check(adds == 0);
        add_failure = true;
        check(!transport.attach(&bus_token, 0x20));
        add_failure = false;
        check(transport.attach(&bus_token, 0x27));
        check(!transport.attach(&bus_token, 0x20));
        for (unsigned command = 0; command <= 255; ++command) {
            const auto reg = static_cast<std::uint8_t>(command);
            if (reg != 0 && reg != 4 && reg != 6) check(!transport.read_pair(reg, ports));
            if (reg != 4 && reg != 6) check(!transport.write_pair(reg, 0, 0));
        }
        check(transactions == 0);
        remove_failure = true;
        check(!transport.detach() && !transport.attach(&bus_token, 0x20));
        remove_failure = false;
        check(transport.detach() && transport.detach());
    }
    check(adds == 2 && removes == 2);
    for (unsigned failed_transaction = 1; failed_transaction <= 5; ++failed_transaction) {
        reset();
        Tca9535I2c transport;
        check(transport.attach(&bus_token, 0x20));
        Tca9535Input inputs(transport);
        Tca9535Sample sample{0x1234, 0x5678};
        fail_at = failed_transaction;
        check(!inputs.initialize() && !inputs.ready());
        check(transactions == failed_transaction);
        check(!inputs.poll(sample) && sample.raw_levels == 0x1234 && sample.changed == 0x5678);
        fail_at = 0;
        check(inputs.initialize() && inputs.ready());
        check(inputs.poll(sample) && sample.raw_levels == 0xffff && sample.changed == 0);
    }
    for (unsigned bad_register = 0; bad_register < 2; ++bad_register) {
        reset();
        Tca9535I2c transport;
        check(transport.attach(&bus_token, 0x21));
        Tca9535Input inputs(transport);
        bad_configuration = bad_register == 0;
        bad_polarity = bad_register == 1;
        check(!inputs.initialize() && !inputs.ready());
    }
    reset();
    {
        Tca9535I2c transport;
        check(transport.attach(&bus_token, 0x22));
        Tca9535Input inputs(transport);
        check(inputs.initialize() && transactions == 5);
        Tca9535Sample sample{};
        for (unsigned bit = 0; bit < 16; ++bit) {
            const auto mask = static_cast<std::uint16_t>(1U << bit);
            const auto raw = static_cast<std::uint16_t>(0xffffU ^ mask);
            registers[0] = static_cast<std::uint8_t>(raw);
            registers[1] = static_cast<std::uint8_t>(raw >> 8);
            check(inputs.poll(sample) && sample.raw_levels == raw && sample.changed == mask);
            check(inputs.poll(sample) && sample.changed == 0);
            registers[0] = registers[1] = 0xff;
            check(inputs.poll(sample) && sample.raw_levels == 0xffff && sample.changed == mask);
        }
        const auto preserved = sample;
        fail_at = transactions + 1;
        registers[0] = 0;
        check(!inputs.poll(sample) && !inputs.ready());
        check(sample.raw_levels == preserved.raw_levels && sample.changed == preserved.changed);
        const auto stopped_at = transactions;
        check(!inputs.poll(sample) && transactions == stopped_at);  // Explicit recovery, no unbounded retries.
        fail_at = 0;
        check(inputs.initialize() && inputs.poll(sample) && sample.changed == 0);
        inputs.invalidate();
        check(!inputs.ready() && !inputs.poll(sample));
    }
    check(removes == 1);
    std::cout << "BD-16 expander: all 16 bits, finite I2C, readback/fault/recovery; no physical qualification\n";
    return EXIT_SUCCESS;
}
