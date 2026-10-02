#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "nightwave/runtime_recorder.h"
namespace {
int failures = 0;
#define CHECK(v) do { if (!(v)) { ++failures; std::cerr << __LINE__ << ": " << #v << '\n'; } } while (false)
nightwave::RuntimeObservation fixture(std::uint64_t now, std::uint32_t frames) {
    nightwave::RuntimeObservation o;
    o.monotonic_ms = now; o.media_frames = frames; o.sample_rate_hz = 48000;
    o.playing = true; o.route = nightwave::RuntimeRoute::kSpeaker; o.volume = 25;
    o.power_adapter_qualified = o.acoustic_route_verified = true;
    o.power.sampled_ms = static_cast<std::uint32_t>(now);
    o.power.voltage_valid = o.power.charging_valid = true;
    o.power.state.source = nightwave::PowerSource::kBattery;
    o.power.state.battery_millivolts = 3800;
    return o;
}
}
int main() {
    using namespace nightwave;
    RuntimeRecorder r; auto o = fixture(0, 0);
    o.playing = false; CHECK(!r.begin(o));
    o.playing = true; o.volume = 0; CHECK(!r.begin(o));
    o.volume = 25; o.paused = true; CHECK(!r.begin(o));
    o.paused = false; o.route = RuntimeRoute::kMuted; CHECK(!r.begin(o));
    o.route = RuntimeRoute::kSpeaker; o.sample_rate_hz = 96000; CHECK(!r.begin(o));
    o = fixture(0, 0); CHECK(r.begin(o)); CHECK(!r.begin(o));
    // Fast synthetic eight-hour timeline, NOT an elapsed hardware experiment.
    for (std::uint32_t second = 1; second <= 28800; ++second)
        r.observe(fixture(second * 1000ULL, second * 48000U));
    CHECK(r.summary().elapsed_ms == 28800000 && r.summary().battery_play_ms == 28800000);
    CHECK(r.summary().gaps == 0 && r.summary().interruptions == 0);
    std::array<char, 768> json{};
    CHECK(format_runtime_report(r.summary(), json.data(), json.size()));
    CHECK(std::strstr(json.data(), "\"physical_pass\":false") && std::strstr(json.data(), "\"human_review_required\":true"));
    std::array<char, 8> short_buffer{};
    CHECK(!format_runtime_report(r.summary(), short_buffer.data(), short_buffer.size()) && !short_buffer[0]);
    CHECK(!format_runtime_report(r.summary(), nullptr, 0));
    r.stop(fixture(28800000, 28800U * 48000U)); CHECK(!r.summary().active);
    r.observe(fixture(28801000, 28801U * 48000U)); CHECK(r.summary().elapsed_ms == 28800000);

    // Unknown, stale, USB, charging, invalid-voltage and unqualified sources.
    for (unsigned scenario = 0; scenario < 7; ++scenario) {
        RuntimeRecorder x; o = fixture(10000, 0);
        if (scenario == 0) o.power.state.source = PowerSource::kUnknown;
        if (scenario == 1) o.power.state.source = PowerSource::kUsb;
        if (scenario == 2) o.power.state.charging = true;
        if (scenario == 3) o.power.sampled_ms = 0;
        if (scenario == 4) o.power.state.battery_millivolts = 1000;
        if (scenario == 5) o.power_adapter_qualified = false;
        if (scenario == 6) o.acoustic_route_verified = false;
        CHECK(x.begin(o)); o.monotonic_ms += 1000; o.media_frames += 48000;
        x.observe(o); CHECK(x.summary().observed_play_ms == 1000 && x.summary().battery_play_ms == 0);
    }
    RuntimeRecorder interrupted; CHECK(interrupted.begin(fixture(0, 0)));
    o = fixture(1000, 48000); o.paused = true; interrupted.observe(o);
    interrupted.observe(fixture(2000, 96000)); // Resume boundary is not credited.
    interrupted.observe(fixture(3000, 144000));
    CHECK(interrupted.summary().observed_play_ms == 1000 && interrupted.summary().interruptions == 2);
    o = fixture(4000, 144000); interrupted.observe(o); // Stalled engine, no media.
    o = fixture(5000, 144001); interrupted.observe(o); // One frame is not a second of playback.
    CHECK(interrupted.summary().observed_play_ms == 1000);
    o = fixture(6000, 192001); o.underruns = 1; interrupted.observe(o);
    CHECK(interrupted.summary().fault_seen && interrupted.summary().observed_play_ms == 1000);
    o = fixture(7000, 240001); o.volume = 30; o.underruns = 1; interrupted.observe(o);
    o = fixture(8000, 288001); o.volume = 30; o.route = RuntimeRoute::kWired; o.underruns = 1; interrupted.observe(o);
    CHECK(interrupted.summary().configuration_changes == 2);
    o = fixture(11000, 432001); interrupted.observe(o);
    CHECK(interrupted.summary().gaps == 1 && interrupted.summary().elapsed_ms == 11000);
    interrupted.observe(fixture(10999, 432001));
    CHECK(interrupted.summary().clock_fault && !interrupted.summary().active);

    RuntimeRecorder wrapped;
    CHECK(wrapped.begin(fixture(UINT32_MAX - 500ULL, UINT32_MAX - 1000)));
    wrapped.observe(fixture(UINT32_MAX + 500ULL, 46999));
    CHECK(wrapped.summary().battery_play_ms == 1000); // Counter and power timestamp wrap.
    wrapped.observe(fixture(UINT32_MAX + 500ULL, 46999));
    CHECK(wrapped.summary().battery_play_ms == 1000); // Duplicate cannot inflate time.
    wrapped.observe(fixture(UINT64_MAX, 50000)); // Very large gap cannot overflow credit.
    CHECK(wrapped.summary().gaps == 1 && wrapped.summary().battery_play_ms == 1000);
    std::cout << "Synthetic runtime recorder checks complete; no physical test performed\n";
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
