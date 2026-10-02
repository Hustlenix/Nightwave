#pragma once
#include <cstddef>
#include <cstdint>
#include "nightwave/power_hal.h"

namespace nightwave {
enum class RuntimeRoute : std::uint8_t { kMuted, kSpeaker, kWired, kBluetooth };
struct RuntimeObservation {
    std::uint64_t monotonic_ms{0};
    std::uint32_t media_frames{0}, errors{0}, underruns{0};
    std::uint32_t sample_rate_hz{0};
    RuntimeRoute route{RuntimeRoute::kMuted};
    std::uint8_t volume{0};
    bool playing{false}, paused{false};
    PowerReading power{};
    // Set only by a separately reviewed/qualified hardware integration.
    bool power_adapter_qualified{false}, acoustic_route_verified{false};
};
struct RuntimeSummary {
    std::uint64_t started_ms{0}, elapsed_ms{0}, observed_play_ms{0}, battery_play_ms{0};
    std::uint32_t gaps{0}, interruptions{0}, configuration_changes{0};
    bool started{false}, active{false}, clock_fault{false}, fault_seen{false};
    bool unqualified_power_seen{false}, unverified_audio_seen{false};
    RuntimeRoute route{RuntimeRoute::kMuted};
    std::uint8_t volume{0};
};
// Sampled software observations, NOT an acoustic test or battery certification.
// One serialized owner. No allocation, filesystem work or audio-task logging.
class RuntimeRecorder {
 public:
    static constexpr std::uint64_t kMaximumGapMs = 2000;
    bool begin(const RuntimeObservation&);
    void observe(const RuntimeObservation&);
    void stop(const RuntimeObservation&);
    const RuntimeSummary& summary() const { return summary_; }
 private:
    RuntimeSummary summary_{};
    RuntimeObservation previous_{};
    void flags(const RuntimeObservation&);
};
bool format_runtime_report(const RuntimeSummary&, char*, std::size_t);
}  // namespace nightwave
