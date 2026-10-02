#include "nightwave/runtime_recorder.h"
#include <cstdio>
#include <limits>

namespace nightwave {
namespace {
bool playing(const RuntimeObservation& o) {
    return o.playing && !o.paused && o.volume > 0 && o.volume <= 100 &&
        o.route >= RuntimeRoute::kSpeaker && o.route <= RuntimeRoute::kBluetooth &&
        (o.sample_rate_hz == 22050 || o.sample_rate_hz == 32000 ||
         o.sample_rate_hz == 44100 || o.sample_rate_hz == 48000);
}
bool battery(const RuntimeObservation& o) {
    const auto& p = o.power;
    return o.power_adapter_qualified && p.voltage_valid && p.charging_valid &&
        static_cast<std::uint32_t>(o.monotonic_ms) - p.sampled_ms <= 5000 &&
        p.state.source == PowerSource::kBattery && !p.state.charging &&
        p.state.battery_millivolts >= 2000 && p.state.battery_millivolts <= 4500;
}
void increment(std::uint32_t& count) {
    if (count != std::numeric_limits<std::uint32_t>::max()) ++count;
}
}
void RuntimeRecorder::flags(const RuntimeObservation& o) {
    summary_.fault_seen = summary_.fault_seen || o.errors != previous_.errors || o.underruns != previous_.underruns;
    summary_.unqualified_power_seen = summary_.unqualified_power_seen || !battery(o);
    summary_.unverified_audio_seen = summary_.unverified_audio_seen || !o.acoustic_route_verified;
}
bool RuntimeRecorder::begin(const RuntimeObservation& o) {
    if (summary_.active || !playing(o)) return false;
    summary_ = {};
    summary_.started = true; summary_.active = true;
    summary_.started_ms = o.monotonic_ms; summary_.route = o.route; summary_.volume = o.volume;
    previous_ = o; flags(o); // Pre-test historical faults are not this session's faults.
    return true;
}
void RuntimeRecorder::observe(const RuntimeObservation& o) {
    if (!summary_.active) return;
    if (o.monotonic_ms < previous_.monotonic_ms) {
        summary_.clock_fault = true; summary_.active = false; return;
    }
    const auto elapsed = o.monotonic_ms - previous_.monotonic_ms;
    if (elapsed == 0) { flags(o); return; } // Duplicates cannot add runtime.
    summary_.elapsed_ms = o.monotonic_ms - summary_.started_ms;
    const bool fault = o.errors != previous_.errors || o.underruns != previous_.underruns;
    const bool changed = o.route != previous_.route || o.volume != previous_.volume ||
        o.sample_rate_hz != previous_.sample_rate_hz;
    flags(o);
    if (changed) increment(summary_.configuration_changes);
    if (elapsed > kMaximumGapMs) increment(summary_.gaps);
    // Unsigned subtraction preserves progress across the media counter wrap.
    const auto frames = static_cast<std::uint32_t>(o.media_frames - previous_.media_frames);
    // 48 kHz is the supported maximum; allow a bounded DMA queue lead.
    const bool progress = elapsed <= kMaximumGapMs && frames > 0 &&
        frames <= elapsed * 48ULL + 4096 &&
        frames + 4096ULL >= elapsed * o.sample_rate_hz / 1000;
    const bool observed = elapsed <= kMaximumGapMs && !fault && !changed &&
        playing(previous_) && playing(o) && progress;
    if (observed) {
        summary_.observed_play_ms += elapsed;
        if (battery(previous_) && battery(o) && previous_.acoustic_route_verified && o.acoustic_route_verified)
            summary_.battery_play_ms += elapsed;
    } else increment(summary_.interruptions);
    previous_ = o;
}
void RuntimeRecorder::stop(const RuntimeObservation& o) {
    observe(o); summary_.active = false;
}
bool format_runtime_report(const RuntimeSummary& r, char* buffer, std::size_t capacity) {
    if (!buffer || !capacity) return false;
    const int n = std::snprintf(buffer, capacity,
        "{\"type\":\"nightwave_runtime\",\"schema\":1,\"started\":%s,\"active\":%s,\"started_ms\":%llu,\"elapsed_ms\":%llu,\"observed_play_ms\":%llu,\"battery_play_ms\":%llu,\"initial_route\":%u,\"initial_volume\":%u,\"gaps\":%lu,\"interruptions\":%lu,\"configuration_changes\":%lu,\"clock_fault\":%s,\"fault_seen\":%s,\"unqualified_power_seen\":%s,\"unverified_audio_seen\":%s,\"target_ms\":28800000,\"physical_pass\":false,\"human_review_required\":true}",
        r.started ? "true" : "false", r.active ? "true" : "false",
        static_cast<unsigned long long>(r.started_ms), static_cast<unsigned long long>(r.elapsed_ms),
        static_cast<unsigned long long>(r.observed_play_ms), static_cast<unsigned long long>(r.battery_play_ms),
        static_cast<unsigned>(r.route), static_cast<unsigned>(r.volume), static_cast<unsigned long>(r.gaps),
        static_cast<unsigned long>(r.interruptions), static_cast<unsigned long>(r.configuration_changes),
        r.clock_fault ? "true" : "false", r.fault_seen ? "true" : "false",
        r.unqualified_power_seen ? "true" : "false", r.unverified_audio_seen ? "true" : "false");
    if (n < 0 || static_cast<std::size_t>(n) >= capacity) { buffer[0] = 0; return false; }
    return true;
}
}  // namespace nightwave
