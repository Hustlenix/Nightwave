#pragma once

#include <cstdint>

namespace nightwave {

struct DiagnosticSnapshot {
    std::uint32_t uptime_ms{0};
    std::uint32_t audio_underruns{0};
    std::uint32_t storage_errors{0};
    std::uint32_t decode_errors{0};
    std::uint32_t minimum_free_heap_bytes{0};
    std::uint32_t pcm_queue_depth_ms{0};
};

}  // namespace nightwave
