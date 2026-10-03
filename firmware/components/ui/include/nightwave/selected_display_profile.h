#pragma once
#include <cstddef>
#include <cstdint>

namespace nightwave {
// Builder-selected Waveshare 24382, not touch 27057. Geometry/transport contract
// only: no operational TFT driver or GPIO assignment. Bench OLED stays active.
// Basis: maker specifications and LCD_1in69.c portrait window example (2023-03-09).
struct SelectedDisplayProfile {
    static constexpr std::uint32_t sku = 24382;
    static constexpr std::uint16_t width = 240, height = 280;
    static constexpr std::uint16_t gram_height = 320, portrait_row_offset = 20;
    static constexpr std::uint16_t max_tile_rows = 16, text_inset = 14;
    static constexpr std::size_t framebuffer_bytes = std::size_t(width) * height * 2;
    static constexpr std::size_t tile_bytes = std::size_t(width) * max_tile_rows * 2;
    static constexpr bool driver_qualified = false;
    static constexpr std::uint8_t portrait_madctl = 0x00, maker_rgb565_colmod = 0x05;
    static constexpr std::uint16_t reset_phase_ms = 100, sleep_out_wait_ms = 120;
};

struct PortraitWindow {
    std::uint16_t x_first{0}, x_last{0}, row_first{0}, row_last{0};
    std::size_t bytes{0};
};

// Inputs use exclusive ends; output controller addresses use inclusive ends.
// Failed validation never mutates caller's window. No allocation, pointer or I/O.
constexpr bool selected_display_tile(std::uint16_t x, std::uint16_t y,
                                     std::uint16_t end_x, std::uint16_t end_y,
                                     PortraitWindow& out) {
    using P = SelectedDisplayProfile;
    if (x >= end_x || y >= end_y || end_x > P::width || end_y > P::height ||
        end_y - y > P::max_tile_rows) return false;
    out = {x, static_cast<std::uint16_t>(end_x - 1),
           static_cast<std::uint16_t>(y + P::portrait_row_offset),
           static_cast<std::uint16_t>(end_y - 1 + P::portrait_row_offset),
           std::size_t(end_x - x) * (end_y - y) * 2};
    return true;
}
} // namespace nightwave
