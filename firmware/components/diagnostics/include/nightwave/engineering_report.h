#pragma once
#include <cstddef>
#include <cstdint>
#include "nightwave/streaming_player.h"
namespace nightwave {
struct EngineeringReport {
    StreamTelemetry stream{};
    std::uint32_t position_ms{0}, catalog_tracks{0}, query_reads{0};
    std::uint32_t heap_free{0}, heap_min{0}, psram_free{0};
    bool catalog_building{false}, catalog_fast{false};
};
// Fixed-capacity JSON, no input strings or allocation. Failure yields an empty
// string, never a truncated report that could be mistaken for valid JSON.
bool format_engineering_report(const EngineeringReport&, char*, std::size_t);
bool parse_unsigned(const char*, std::uint32_t maximum, std::uint32_t&);
}
