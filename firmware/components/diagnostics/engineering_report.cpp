#include "nightwave/engineering_report.h"
#include <cstdio>
namespace nightwave {
bool parse_unsigned(const char* text, std::uint32_t maximum, std::uint32_t& value) {
    if (!text || !*text) return false;
    std::uint32_t result = 0; unsigned digits = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9' || ++digits > 10) return false;
        const auto digit = static_cast<unsigned>(*p - '0');
        if (digit > maximum || result > (maximum - digit) / 10) return false;
        result = result * 10 + digit;
    }
    value = result; return true;
}
bool format_engineering_report(const EngineeringReport& r, char* buffer, std::size_t capacity) {
    if (!buffer || !capacity) return false;
    const auto& t = r.stream;
    const int n = std::snprintf(buffer, capacity,
        "{\"type\":\"nightwave_engineering\",\"schema\":1,\"position_ms\":%lu,\"duration_ms\":%lu,\"indexed_seek\":%s,\"seek_base_samples\":%lu,\"route_changes\":%lu,\"catalog_tracks\":%lu,\"catalog_building\":%s,\"catalog_fast\":%s,\"query_reads\":%lu,\"heap_free_bytes\":%lu,\"heap_min_bytes\":%lu,\"psram_free_bytes\":%lu,\"sd_bytes\":%lu,\"sd_reads\":%lu,\"decode_calls\":%lu,\"underruns\":%lu,\"errors\":%lu,\"bluetooth_backend_ready\":false,\"battery_mv\":null,\"battery_percent\":null,\"physical_pass\":false}",
        static_cast<unsigned long>(r.position_ms), static_cast<unsigned long>(t.duration_ms), t.indexed_seek ? "true" : "false",
        static_cast<unsigned long>(t.seek_base_samples), static_cast<unsigned long>(t.route_changes), static_cast<unsigned long>(r.catalog_tracks),
        r.catalog_building ? "true" : "false", r.catalog_fast ? "true" : "false", static_cast<unsigned long>(r.query_reads),
        static_cast<unsigned long>(r.heap_free), static_cast<unsigned long>(r.heap_min), static_cast<unsigned long>(r.psram_free),
        static_cast<unsigned long>(t.sd_bytes), static_cast<unsigned long>(t.sd_reads), static_cast<unsigned long>(t.decode_calls),
        static_cast<unsigned long>(t.underruns), static_cast<unsigned long>(t.errors));
    if (n < 0 || static_cast<std::size_t>(n) >= capacity) { buffer[0] = 0; return false; }
    return true;
}
}
