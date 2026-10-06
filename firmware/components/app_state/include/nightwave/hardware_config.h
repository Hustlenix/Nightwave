#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
#include "nightwave/reference_board.h"

namespace nightwave::hardware {
enum class BluetoothArchitecture { kPending, kBm83At };
// Builder selected BD-14 on 2026-10-02. Selection is not adapter/bench proof.
inline constexpr auto kBluetoothArchitecture = BluetoothArchitecture::kBm83At;
inline constexpr bool kBluetoothBackendQualified = false;
// Builder selected BD-15 Waveshare 24382 on 2026-10-03. Not a pin/driver lock.
inline constexpr bool kFinalDisplaySelected = true;
inline constexpr std::uint32_t kFinalDisplaySku = 24382;
inline constexpr bool kFinalDisplayDriverQualified = false;
inline constexpr bool kCombinedProductPinsReviewed = false;
// Builder selected BD-16 on 2026-10-03. Address/port map/IRQ await source review.
inline constexpr bool kTca9535InputExpansionSelected = true;
inline constexpr bool kTca9535PhysicalQualified = false;
inline constexpr bool kBatteryLocked = false;
// Host tests default to the legacy profile unless explicitly compiled for the
// reference board. ESP-IDF selection comes from the explicit Kconfig choice.
#if defined(CONFIG_NIGHTWAVE_REFERENCE_BOARD) && CONFIG_NIGHTWAVE_REFERENCE_BOARD
inline constexpr bool kReferenceBoard = true;
using namespace nightwave::reference_board;
#else
inline constexpr bool kReferenceBoard = false;
inline constexpr std::int8_t kDacMute = -1; // Bench has one enable only.
// Legacy USB-powered bench map; never flash this profile onto the reference PCB.

inline constexpr std::int8_t kSdClk = 12;
inline constexpr std::int8_t kSdCmd = 11;
inline constexpr std::int8_t kSdD0 = 13;
inline constexpr std::int8_t kSdCardDetect = 14;
inline constexpr std::int8_t kI2sBitClock = 5;
inline constexpr std::int8_t kI2sWordSelect = 6;
inline constexpr std::int8_t kI2sDataOut = 7;
inline constexpr std::int8_t kI2cSda = 8;
inline constexpr std::int8_t kI2cScl = 9;
inline constexpr std::int8_t kButtonPrevious = 1;
inline constexpr std::int8_t kButtonPlayPause = 2;
inline constexpr std::int8_t kButtonNext = 4;
inline constexpr std::int8_t kButtonVolumeDown = 10;
inline constexpr std::int8_t kButtonVolumeUp = 15;
inline constexpr std::int8_t kHeadphoneDetect = 16;
inline constexpr std::int8_t kSpeakerEnable = 17;
inline constexpr std::int8_t kHeadphoneEnable = 18;
inline constexpr std::int8_t kChargeStatus = 21;
inline constexpr std::int8_t kPowerHold = 38;
inline constexpr std::int8_t kFuelAlert = 39;
inline constexpr std::int8_t kDisplayReset = 40;

inline constexpr std::array<std::int8_t, 21> kAssignedPins{
    kSdClk,           kSdCmd,           kSdD0,          kSdCardDetect,
    kI2sBitClock,     kI2sWordSelect,   kI2sDataOut,    kI2cSda,
    kI2cScl,          kButtonPlayPause, kButtonNext,    kButtonPrevious,
    kButtonVolumeUp,  kButtonVolumeDown, kHeadphoneDetect, kSpeakerEnable,
    kHeadphoneEnable, kChargeStatus,    kPowerHold,     kFuelAlert,
    kDisplayReset,
};
#endif

constexpr bool pins_are_unique() {
    for (std::size_t left = 0; left < kAssignedPins.size(); ++left) {
        for (std::size_t right = left + 1; right < kAssignedPins.size(); ++right) {
            if (kAssignedPins[left] == kAssignedPins[right]) {
                return false;
            }
        }
    }
    return true;
}

constexpr bool pins_avoid_restricted_set() {
    constexpr std::array<std::int8_t, 18> restricted{
        0, 3, 19, 20, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 45, 46,
    };
    for (const auto assigned : kAssignedPins) {
        for (const auto blocked : restricted) {
            if (assigned == blocked) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace nightwave::hardware
