#pragma once
#include <cstddef>
#include <cstdint>
namespace nightwave {
enum class PlayerScreen { kBoot, kBrowser, kNowPlaying, kNoSd, kCorrupt, kDiagnostics, kLyrics, kSettings };
class PlayerNavigation {
 public:
    PlayerScreen screen{PlayerScreen::kBoot};
    std::size_t cursor{0}, count{0};
    std::uint32_t last_input_ms{0};
    bool asleep{false};
    // First interaction wakes without accidentally changing track/volume.
    bool interact(std::uint32_t now) {
        last_input_ms = now;
        const bool execute = !asleep; asleep = false; return execute;
    }
    void tick(std::uint32_t now) { asleep = now - last_input_ms >= 30000; }
    void populate(std::size_t value) { count = value; cursor = 0; }
    void move(int direction) {
        if (!count) return;
        cursor = direction > 0 ? (cursor + 1) % count : (cursor + count - 1) % count;
    }
};
}  // namespace nightwave
