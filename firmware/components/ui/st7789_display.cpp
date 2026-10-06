#include "nightwave/st7789_display.h"
#include <algorithm>
#include <cstdio>

namespace nightwave {
namespace {
struct Init { std::uint8_t command, size; std::array<std::uint8_t, 14> data; };
// Panel-specific register values: Waveshare LCD_1in69.c, V1.0 2023-03-09.
// See docs/reference-board-firmware.md for the source and qualification limits.
constexpr Init init[]{
    {0x36,1,{0x00}}, {0x3a,1,{0x05}}, {0xb2,5,{0x0b,0x0b,0,0x33,0x35}},
    {0xb7,1,{0x11}}, {0xbb,1,{0x35}}, {0xc0,1,{0x2c}}, {0xc2,1,{0x01}},
    {0xc3,1,{0x0d}}, {0xc4,1,{0x20}}, {0xc6,1,{0x13}}, {0xd0,2,{0xa4,0xa1}},
    {0xd6,1,{0xa1}},
    {0xe0,14,{0xf0,0x06,0x0b,0x0a,0x09,0x26,0x29,0x33,0x41,0x18,0x16,0x15,0x29,0x2d}},
    {0xe1,14,{0xf0,0x04,0x08,0x08,0x07,0x03,0x28,0x32,0x40,0x3b,0x19,0x18,0x2a,0x2e}},
    {0xe4,3,{0x25,0,0}}, {0x21,0,{}},
};
using Lines = std::array<std::array<char, 18>, 13>;
void wrap(Lines& lines, std::size_t first, std::size_t rows, const char* text) {
    if (!text) return;
    std::size_t at = 0;
    for (std::size_t row = first; row < lines.size() && row < first + rows; ++row) {
        std::size_t column = 0;
        while (column < 17 && at < 256 && text[at] && text[at] != '\n') {
            const auto c = static_cast<unsigned char>(text[at++]);
            if (c >= 0x80 && c < 0xc0) continue; // One '?' per UTF-8 codepoint.
            lines[row][column++] = c >= 32 && c < 127 ? static_cast<char>(c) : '?';
        }
        if (at == 256) break;
        if (text[at] == '\n') ++at;
        if (!text[at]) break;
    }
}
} // namespace

bool St7789Display::fail() {
    bus_.backlight(false); bus_.disconnect(); ready_ = false; asleep_ = false;
    return false;
}
bool St7789Display::begin() {
    if (ready_) return true;
    if (!bus_.connect() || !bus_.backlight(false)) return fail();
    for (const bool high : {true, false, true}) {
        if (!bus_.reset(high)) return fail();
        bus_.delay_ms(100);
    }
    for (const auto& op : init)
        if (!bus_.command(op.command, op.data.data(), op.size)) return fail();
    if (!bus_.command(0x11, nullptr, 0)) return fail();
    bus_.delay_ms(120);
    if (!bus_.command(0x29, nullptr, 0)) return fail();
    ready_ = true;
    // Clear uninitialized GRAM before lighting the display.
    return present(DisplayFrame{});
}
bool St7789Display::sleep(bool asleep) {
    if (!ready_) return false;
    if (asleep == asleep_) return true;
    if (!bus_.backlight(false)) return fail();
    if (!bus_.command(asleep ? 0x10 : 0x11, nullptr, 0)) return fail();
    bus_.delay_ms(120);
    asleep_ = asleep;
    // Wake stays dark until a complete fresh frame is transferred.
    return true;
}
bool St7789Display::present(const DisplayFrame& frame) {
    if (!ready_ || asleep_) return false;
    Lines lines{};
    if (frame.lyrics_view) {
        wrap(lines,0,2,frame.title);
        wrap(lines,2,1,frame.artist);
        wrap(lines,4,3,frame.lyric_current);
        wrap(lines,8,3,frame.lyric_next);
        std::snprintf(lines[12].data(), lines[12].size(), "%s %lu:%02lu",
            frame.paused ? "PAUSE" : "PLAY", static_cast<unsigned long>(frame.elapsed_ms/60000),
            static_cast<unsigned long>((frame.elapsed_ms/1000)%60));
    } else {
        for (std::size_t row=0; row<frame.fallback.rows.size(); ++row)
            wrap(lines,row,1,frame.fallback.rows[row].data());
    }
    using P = SelectedDisplayProfile;
    for (std::uint16_t y = 0; y < P::height; y += P::max_tile_rows) {
        const auto end = static_cast<std::uint16_t>(std::min<unsigned>(P::height, y + P::max_tile_rows));
        PortraitWindow window{};
        if (!selected_display_tile(0,y,P::width,end,window)) return fail();
        for (unsigned row = y; row < end; ++row) {
            for (unsigned x=0; x<P::width; ++x) {
                std::uint16_t color = 0x0841; // dark neutral background
                if (row >= 12 && x >= P::text_inset && x < P::text_inset + 17*12) {
                    const unsigned line=(row-12)/20, gy=((row-12)%20)/2;
                    const unsigned letter=(x-P::text_inset)/12, gx=((x-P::text_inset)%12)/2;
                    if (line < lines.size() && gy<5 && gx<5 && lines[line][letter]) {
                        const auto shape = glyph(lines[line][letter]);
                        if (shape[gy] & (1U<<(4-gx)))
                            color = frame.lyrics_view && line>=4 && line<=6 ? 0xffe0 : 0xffff;
                    }
                }
                const auto offset = ((row-y)*P::width+x)*2;
                tile_[offset]=static_cast<std::uint8_t>(color>>8);
                tile_[offset+1]=static_cast<std::uint8_t>(color);
            }
        }
        const std::uint8_t columns[]{0,0,0,239};
        const std::uint8_t rows[]{static_cast<std::uint8_t>(window.row_first>>8),
            static_cast<std::uint8_t>(window.row_first), static_cast<std::uint8_t>(window.row_last>>8),
            static_cast<std::uint8_t>(window.row_last)};
        if (!bus_.command(0x2a,columns,4) || !bus_.command(0x2b,rows,4) ||
            !bus_.command(0x2c,nullptr,0) || !bus_.pixels(tile_.data(),window.bytes)) return fail();
    }
    if (!bus_.backlight(true)) return fail();
    return true;
}
} // namespace nightwave
