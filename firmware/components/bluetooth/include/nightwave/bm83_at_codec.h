#pragma once
#include "nightwave/bm83_uart.h"
namespace nightwave {
// Manufacturer AudioUARTCommandSet_v2.08, 5.1.63 / 7.65. Pure wire codec,
// not a qualified AT-v1.0 transport. Verify the actual module version first.
enum class Bm83AtQuery : std::uint8_t { kMode = 4, kInput = 5, kRate = 7 };
enum class Bm83AtReportKind { kDiscoveryComplete, kInput, kMode, kRate };
struct Bm83AtReport { Bm83AtReportKind kind{Bm83AtReportKind::kMode}; std::uint8_t value{0}; };
class Bm83AtCodec {
 public:
    // Standby only. 10*1.28 s discovery fits the control model's 15 s timeout.
    static bool discover(Bm83Packet& p, std::uint8_t units = 10, std::uint8_t responses = 8) {
        p.size = 0;
        if (!units || units > 0x30 || !responses || responses > 8) return false;
        p.body[0] = 0x44; p.body[1] = 0; p.body[2] = units; p.body[3] = responses;
        p.body[4] = 1; p.body[5] = 1; p.body[6] = 0; p.size = 7; return true;
    }
    static void cancel_discovery(Bm83Packet& p) { two(p, 1); }
    static bool query(Bm83Packet& p, Bm83AtQuery q) {
        p.size = 0;
        if (q != Bm83AtQuery::kMode && q != Bm83AtQuery::kInput && q != Bm83AtQuery::kRate) return false;
        two(p, static_cast<std::uint8_t>(q)); return true;
    }
    // Requires a subsequent reset/reverification; never called automatically.
    static void select_tx(Bm83Packet& p) { three(p, 3, 0); }
    // Requires TX mode; changing input/rate restarts DSP and interrupts audio.
    static void select_i2s(Bm83Packet& p) { three(p, 2, 1); }
    static bool select_rate(Bm83Packet& p, std::uint32_t rate) {
        p.size = 0;
        if (rate != 44100 && rate != 48000) return false;
        three(p, 6, rate == 44100 ? 1 : 0); return true;
    }
    // Packet gating only, NOT an AVDTP START/SUSPEND operation.
    static void block_packets(Bm83Packet& p, bool block) { three(p, 8, block ? 1 : 0); }
    static bool report(const Bm83Packet& p, Bm83AtReport& out) {
        if (p.size != 3 || p.body[0] != 0x5a) return false;
        Bm83AtReportKind kind{};
        switch (p.body[1]) {
            case 1: if (p.body[2] > 0x44) return false; kind = Bm83AtReportKind::kDiscoveryComplete; break;
            case 2: kind = Bm83AtReportKind::kInput; break;
            case 3: kind = Bm83AtReportKind::kMode; break;
            case 4: kind = Bm83AtReportKind::kRate; break;
            default: return false;
        }
        if (p.body[1] != 1 && p.body[2] > 1) return false;
        out = {kind, p.body[2]}; return true;
    }
 private:
    static void two(Bm83Packet& p, std::uint8_t sub) { p.body[0] = 0x44; p.body[1] = sub; p.size = 2; }
    static void three(Bm83Packet& p, std::uint8_t sub, std::uint8_t value) { two(p, sub); p.body[2] = value; p.size = 3; }
};
}
