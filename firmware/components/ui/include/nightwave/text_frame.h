#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace nightwave {
struct TextFrame {
    std::array<std::array<char, 22>, 8> rows{};
    void line(std::size_t row, const char* text) {
        if (row >= rows.size()) return;
        rows[row].fill(0);
        if (text) for (std::size_t i = 0; i < rows[row].size() - 1 && text[i]; ++i)
            rows[row][i] = text[i];
    }
};
// Original small 5x5 letterforms. Lowercase displays uppercase, other UTF-8
// bytes show '?'; full UTF-8 typography is not claimed by this ASCII renderer.
inline std::array<std::uint8_t, 5> glyph(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    switch (c) {
        case 'A': return {14,17,31,17,17}; case 'B': return {30,17,30,17,30};
        case 'C': return {15,16,16,16,15}; case 'D': return {30,17,17,17,30};
        case 'E': return {31,16,30,16,31}; case 'F': return {31,16,30,16,16};
        case 'G': return {15,16,19,17,15}; case 'H': return {17,17,31,17,17};
        case 'I': return {31,4,4,4,31}; case 'J': return {7,2,2,18,12};
        case 'K': return {17,18,28,18,17}; case 'L': return {16,16,16,16,31};
        case 'M': return {17,27,21,17,17}; case 'N': return {17,25,21,19,17};
        case 'O': return {14,17,17,17,14}; case 'P': return {30,17,30,16,16};
        case 'Q': return {14,17,21,18,13}; case 'R': return {30,17,30,18,17};
        case 'S': return {15,16,14,1,30}; case 'T': return {31,4,4,4,4};
        case 'U': return {17,17,17,17,14}; case 'V': return {17,17,17,10,4};
        case 'W': return {17,17,21,27,17}; case 'X': return {17,10,4,10,17};
        case 'Y': return {17,10,4,4,4}; case 'Z': return {31,2,4,8,31};
        case '0': return {14,19,21,25,14}; case '1': return {4,12,4,4,14};
        case '2': return {14,17,2,4,31}; case '3': return {30,1,14,1,30};
        case '4': return {2,6,10,31,2}; case '5': return {31,16,30,1,30};
        case '6': return {14,16,30,17,14}; case '7': return {31,1,2,4,4};
        case '8': return {14,17,14,17,14}; case '9': return {14,17,15,1,14};
        case ' ': return {}; case '.': return {0,0,0,0,4};
        case ':': return {0,4,0,4,0}; case '-': return {0,0,31,0,0};
        case '/': return {1,2,4,8,16}; case '>': return {8,4,2,4,8};
        case '[': return {14,8,8,8,14}; case ']': return {14,2,2,2,14};
        case '_': return {0,0,0,0,31}; case '+': return {0,4,31,4,0};
        case '%': return {17,2,4,8,17}; default: return {14,1,6,0,4};
    }
}
inline std::array<std::uint8_t, 1024> rasterize(const TextFrame& text) {
    std::array<std::uint8_t, 1024> pixels{};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t letter = 0; letter < 21 && text.rows[row][letter]; ++letter) {
            const auto shape = glyph(text.rows[row][letter]);
            for (std::size_t x = 0; x < 5; ++x)
                for (std::size_t y = 0; y < 5; ++y)
                    if (shape[y] & (1U << (4 - x)))
                        pixels[row * 128 + letter * 6 + x] |= static_cast<std::uint8_t>(1U << (y + 1));
        }
    }
    return pixels;
}
}  // namespace nightwave
