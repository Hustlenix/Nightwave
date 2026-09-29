#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nightwave::hardware {

inline constexpr std::int8_t kSdClk = 12;
inline constexpr std::int8_t kSdCmd = 11;
inline constexpr std::int8_t kSdD0 = 13;
inline constexpr std::int8_t kSdCardDetect = 14;
inline constexpr std::int8_t kI2sBitClock = 5;
inline constexpr std::int8_t kI2sWordSelect = 6;
inline constexpr std::int8_t kI2sDataOut = 7;
inline constexpr std::int8_t kI2cSda = 8;
inline constexpr std::int8_t kI2cScl = 9;
inline constexpr std::int8_t kButtonPlayPause = 1;
inline constexpr std::int8_t kButtonNext = 2;
inline constexpr std::int8_t kButtonPrevious = 4;
inline constexpr std::int8_t kButtonVolumeUp = 10;
inline constexpr std::int8_t kButtonVolumeDown = 15;
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
