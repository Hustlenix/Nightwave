#include <cstdlib>
#include <cstring>
#include <iostream>
#include "nightwave/bluetooth_source.h"
#include "nightwave/bm83_uart.h"
#include "nightwave/power_hal.h"
#include "nightwave/engineering_report.h"
namespace {
int failures = 0;
#define CHECK(v) do { if (!(v)) { ++failures; std::cerr << __LINE__ << ": " << #v << '\n'; } } while(false)
struct FakeBt : nightwave::BluetoothBackend {
    bool fail{false}; unsigned disconnects{0};
    nightwave::BtWrite result{nightwave::BtWrite::kAccepted};
    nightwave::BtCapabilities capabilities() const override { return {true, false, true, true}; }
    bool discover(std::uint32_t) override { return !fail; }
    bool connect(const nightwave::BtDevice&, std::uint32_t) override { return !fail; }
    bool start(const nightwave::AudioFormat&, std::uint32_t) override { return !fail; }
    void disconnect(std::uint32_t) override { ++disconnects; }
    nightwave::BtWrite write(const nightwave::PcmBlock&) override { return result; }
};
}
int main() {
    using namespace nightwave;
    Bm83Uart uart; Bm83Packet packet;
    const std::array<std::uint8_t, 2> body{0x14, 0x33}; std::array<std::uint8_t, 260> wire{};
    CHECK(Bm83Uart::encode(body.data(), body.size(), wire.data(), wire.size()) == 6);
    CHECK(wire[5] == 0xb7); // Manufacturer protocol example.
    for (unsigned cycle = 0; cycle < 1000; ++cycle) {
        for (unsigned i = 0; i < 5; ++i) CHECK(uart.push(wire[i], i, packet) == Bm83Parse::kWaiting);
        CHECK(uart.push(wire[5], 5, packet) == Bm83Parse::kPacket && packet.size == 2 && packet.body[1] == 0x33);
        for (unsigned i = 0; i < 5; ++i) uart.push(wire[i], i, packet);
        CHECK(uart.push(0, 5, packet) == Bm83Parse::kDropped);
    }
    uart.push(0xaa, 0, packet); uart.push(0xff, 1, packet); CHECK(uart.push(0xff, 2, packet) == Bm83Parse::kDropped);
    uart.push(0xaa, 0, packet); CHECK(uart.push(0, 101, packet) == Bm83Parse::kWaiting && packet.size == 0);
    for (unsigned i = 0; i < 10000; ++i) CHECK(uart.push(0x55, i + 200, packet) == Bm83Parse::kWaiting);
    CHECK(!Bm83Uart::encode(body.data(), body.size(), wire.data(), 5));
    UnavailableBluetooth unavailable; BluetoothSource absent(unavailable);
    CHECK(absent.state() == BtState::kUnavailable && !absent.discover(0));
    FakeBt backend; BluetoothSource source(backend);
    BtDevice device; device.address[0] = 1; std::strcpy(device.name.data(), "original test endpoint");
    std::array<std::int16_t, 2048> pcm{}; PcmBlock block{pcm.data(), 1024, {44100, 2, 16}, 0};
    for (unsigned i = 0; i < 1000; ++i) {
        CHECK(source.discover(0)); const auto old = source.epoch();
        for (unsigned n = 1; n <= 8; ++n) { device.address[0] = static_cast<std::uint8_t>(n); CHECK(source.found(device, old)); }
        device.address[0] = 9; CHECK(!source.found(device, old) && source.count() == 8);
        CHECK(!source.choose(8, 1)); CHECK(source.choose(0, 1));
        CHECK(!source.connected(old)); CHECK(source.connected(source.epoch()));
        CHECK(!source.start({32000, 2, 16}, 2)); CHECK(source.start({44100, 2, 16}, 2));
        CHECK(source.streaming(source.epoch())); CHECK(source.write(block) == BtWrite::kAccepted);
        backend.result = BtWrite::kWouldBlock; CHECK(source.write(block) == BtWrite::kWouldBlock && source.state() == BtState::kStreaming);
        backend.result = BtWrite::kAccepted; source.lost(source.epoch());
        CHECK(source.state() == BtState::kFault && source.take_stop_request() && !source.take_stop_request());
        CHECK(source.write(block) == BtWrite::kUnavailable); source.disconnect();
    }
    CHECK(source.discover(UINT32_MAX - 100)); source.tick(15000); CHECK(source.state() == BtState::kFault);
    UnavailablePower power; const auto unknown = power.sample(123);
    CHECK(!unknown.voltage_valid && !unknown.charge_percent_valid && unknown.state.source == PowerSource::kUnknown && !power.request_shutdown());
    BatteryPolicy pending; CHECK(!pending.update(unknown, 123).available);
    BatteryPolicy policy({3300, 3100, 100}); PowerReading reading; reading.voltage_valid = true;
    reading.sampled_ms = 123; reading.state.battery_millivolts = 3299; CHECK(policy.update(reading, 123).low);
    reading.state.battery_millivolts = 3350; CHECK(policy.update(reading, 124).low);
    reading.state.battery_millivolts = 3400; CHECK(!policy.update(reading, 125).low);
    reading.state.battery_millivolts = 3100; CHECK(policy.update(reading, 126).stop_playback);
    CHECK(!policy.update(reading, 6000).available); reading.state.battery_millivolts = 65535; CHECK(!policy.update(reading, 127).available);
    std::uint32_t value = 7;
    for (const char* text : {"", "-1", "+1", " 1", "1x", "4294967296", "00000000001"}) CHECK(!parse_unsigned(text, UINT32_MAX, value) && value == 7);
    CHECK(parse_unsigned("4294967295", UINT32_MAX, value) && value == UINT32_MAX);
    CHECK(!parse_unsigned("101", 100, value)); CHECK(parse_unsigned("0", 100, value) && value == 0);
    EngineeringReport report; report.stream.indexed_seek = true; report.query_reads = 16;
    std::array<char, 1024> json{}; CHECK(format_engineering_report(report, json.data(), json.size()));
    CHECK(std::strstr(json.data(), "\"indexed_seek\":true") && std::strstr(json.data(), "\"battery_mv\":null") && std::strstr(json.data(), "\"physical_pass\":false"));
    std::array<char, 4> tiny{}; CHECK(!format_engineering_report(report, tiny.data(), tiny.size()) && tiny[0] == 0);
    std::cout << "HAL tests passed: source state/epochs/capacity/backpressure/timeouts, unknown power, battery policy, bounded diagnostics/parser\n";
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
