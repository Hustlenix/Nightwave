#include <cstdlib>
#include <iostream>
#include "nightwave/hardware_config.h"
#include "nightwave/selected_display_profile.h"
int main() {
    using namespace nightwave;
    static_assert(hardware::kFinalDisplaySelected && !hardware::kFinalDisplayDriverQualified);
    static_assert(SelectedDisplayProfile::framebuffer_bytes == 134400);
    static_assert(SelectedDisplayProfile::tile_bytes == 7680);
    static_assert(SelectedDisplayProfile::height + SelectedDisplayProfile::portrait_row_offset <= SelectedDisplayProfile::gram_height);
    PortraitWindow window;
    unsigned count = 0;
    for (std::uint16_t y = 0; y < SelectedDisplayProfile::height; ++y) {
        const auto end = static_cast<std::uint16_t>(y + 1);
        if (!selected_display_tile(0, y, 240, end, window) || window.row_first != y + 20 ||
            window.row_last != y + 20 || window.bytes != 480) return EXIT_FAILURE;
        ++count;
    }
    if (!selected_display_tile(0, 264, 240, 280, window) || window.bytes != 7680 || window.row_last != 299) return EXIT_FAILURE;
    const auto preserved = window;
    for (const auto bad : {0, 281, 65535}) {
        if (selected_display_tile(0, 0, 240, static_cast<std::uint16_t>(bad), window)) return EXIT_FAILURE;
        if (window.row_last != preserved.row_last || window.bytes != preserved.bytes) return EXIT_FAILURE;
    }
    if (selected_display_tile(0, 0, 240, 17, window) || selected_display_tile(0, 0, 241, 1, window) ||
        selected_display_tile(10, 1, 10, 2, window) || selected_display_tile(0, 280, 240, 280, window)) return EXIT_FAILURE;
    std::cout << "Selected 24382 geometry/tile contract: " << count << " rows; no physical driver proof\n";
    return EXIT_SUCCESS;
}
