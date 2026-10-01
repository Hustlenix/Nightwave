#pragma once
#include "nightwave/text_frame.h"
namespace nightwave {
class OledDisplay {
 public:
    bool initialize();
    bool show(const TextFrame&);
    bool sleep(bool asleep);
 private:
    bool command(const std::uint8_t* bytes, std::size_t count);
    void* bus_{nullptr};
    void* device_{nullptr};
};
}  // namespace nightwave
