#pragma once
#include <array>
#include <cstdint>

namespace nightwave::reference_board {
// AI reference PCB, not bench-qualified. Checked against design-manifest.json.
inline constexpr std::int8_t kSdClk = 12;
inline constexpr std::int8_t kSdCmd = 11;
inline constexpr std::int8_t kSdD0 = 13;
inline constexpr std::int8_t kI2sBitClock = 5;
inline constexpr std::int8_t kI2sWordSelect = 6;
inline constexpr std::int8_t kI2sDataOut = 7;
inline constexpr std::int8_t kI2cSda = 8;
inline constexpr std::int8_t kI2cScl = 9;
inline constexpr std::int8_t kHeadphoneDetect = 16;
inline constexpr std::int8_t kSpeakerEnable = 17;
inline constexpr std::int8_t kDacMute = 18; // PCM5102A XSMT: low mutes.
inline constexpr std::int8_t kHeadphoneEnable = 39;
inline constexpr std::int8_t kTcaInterrupt = 14;
inline constexpr std::int8_t kBtMfb = 4;
inline constexpr std::int8_t kBtReset = 15;
inline constexpr std::int8_t kBtWake = 10; // MCU input: BM83 UART_TX_IND, not a wake output.
inline constexpr std::int8_t kBtMclk = 21; // MCU input/reserved: BM83 MCLK1 is an output.
inline constexpr std::int8_t kBtRx = 2;
inline constexpr std::int8_t kBtTx = 1;
inline constexpr std::int8_t kDisplayMosi = 47;
inline constexpr std::int8_t kDisplayClock = 48;
inline constexpr std::int8_t kDisplayBacklight = 38;
inline constexpr std::int8_t kDisplayReset = 40;
inline constexpr std::int8_t kDisplayDc = 41;
inline constexpr std::int8_t kDisplayCs = 42;
inline constexpr std::uint8_t kTcaAddress = 0x20;
inline constexpr std::array<std::uint8_t, 5> kButtonBits{0, 1, 2, 3, 4};
inline constexpr std::uint8_t kSdDetectBit = 5;
inline constexpr std::array<std::int8_t, 25> kAssignedPins{
    kSdClk, kSdCmd, kSdD0, kI2sBitClock, kI2sWordSelect, kI2sDataOut,
    kI2cSda, kI2cScl, kHeadphoneDetect, kSpeakerEnable, kDacMute,
    kHeadphoneEnable, kTcaInterrupt, kBtMfb, kBtReset, kBtWake, kBtMclk,
    kBtRx, kBtTx, kDisplayMosi, kDisplayClock, kDisplayBacklight,
    kDisplayReset, kDisplayDc, kDisplayCs};
} // namespace nightwave::reference_board
