#pragma once
#include "nightwave/text_frame.h"
namespace nightwave {
struct DisplayCapabilities {
    std::uint16_t width{0}, height{0};
    bool color{false}, unicode{false}, provisional{true};
};
// Borrowed text is valid only for the synchronous present call. Drivers must
// copy into bounded owned buffers before asynchronous DMA, never retain pointers.
struct DisplayFrame {
    TextFrame fallback{};
    const char* title{""}; const char* artist{""}; const char* album{""};
    const char* lyric_current{""}; const char* lyric_next{""};
    std::uint32_t elapsed_ms{0}, duration_ms{0};
    bool lyrics_view{false}, playing{false}, paused{false};
};
class DisplaySink {
 public:
    virtual ~DisplaySink() = default;
    virtual bool begin() = 0;
    virtual DisplayCapabilities capabilities() const = 0;
    virtual bool present(const DisplayFrame&) = 0;
    virtual bool sleep(bool asleep) = 0;
};
} // namespace nightwave
