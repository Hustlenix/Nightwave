#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include "nightwave/mp3_framer.h"
namespace nightwave {
enum class SeekIndexStatus { kIdle, kBuilding, kReady, kInvalid, kLimit, kIo };
struct Mp3SeekPoint { std::uint32_t offset{0}, samples{0}, rate{0}, duration_ms{0}; };
// Header-only idle scanner. Does not establish decodability, gapless trim or
// acoustically correct timing. Fixed checkpoints; no full-track RAM allocation.
class Mp3SeekIndex {
 public:
    static constexpr std::uint32_t kStride = 128, kPoints = 1024;
    ~Mp3SeekIndex() { cancel(); }
    bool begin(const char* path);
    void step(unsigned frames = 4); // Hard cap eight reads, each <=1536 bytes.
    void cancel();
    SeekIndexStatus status() const { return status_; }
    bool ready() const { return status_ == SeekIndexStatus::kReady; }
    std::uint32_t duration_ms() const;
    std::uint32_t frame_count() const { return frames_; }
    bool save(const char* root); // Reserved .nightwave cache only; <=8.6 KiB.
    static bool cached_seek(const char* path, const char* root, std::uint32_t ms, Mp3SeekPoint&);
    static std::uint32_t probe_duration(std::FILE*, std::uint32_t length);
 private:
    struct Point { std::uint32_t offset{0}, samples{0}; };
    std::array<Point, kPoints> points_{};
    std::array<std::uint8_t, 1536> buffer_{};
    std::array<char, 256> path_{};
    Mp3Framer framer_{};
    std::FILE* file_{nullptr};
    std::size_t buffered_{0};
    std::uint32_t length_{0}, cursor_{0}, frames_{0}, samples_{0}, rate_{0}, count_{0}, fingerprint_{0};
    std::uint64_t modified_{0};
    SeekIndexStatus status_{SeekIndexStatus::kIdle};
};
static_assert(sizeof(Mp3SeekIndex) <= 12 * 1024, "Seek scanner must remain bounded in UI PSRAM");
}
