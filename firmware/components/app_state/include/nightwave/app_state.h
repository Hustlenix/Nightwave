#pragma once

#include <cstdint>

namespace nightwave {

enum class AppState : std::uint8_t {
    kBooting,
    kNoCard,
    kIndexing,
    kIdle,
    kPlaying,
    kPaused,
    kCharging,
    kError,
    kShuttingDown,
};

enum class ErrorCode : std::uint8_t {
    kNone,
    kStorageUnavailable,
    kFilesystemCorrupt,
    kUnsupportedFormat,
    kDecodeFailure,
    kAudioUnderrun,
    kLowBattery,
    kThermalFault,
};

enum class EventType : std::uint8_t {
    kBootComplete,
    kStorageInserted,
    kStorageRemoved,
    kLibraryReady,
    kPlaybackChanged,
    kHeadphonesChanged,
    kBatteryChanged,
    kFault,
};

struct AppEvent {
    EventType type{EventType::kBootComplete};
    std::uint32_t monotonic_ms{0};
    std::uint32_t value{0};
};

}  // namespace nightwave
