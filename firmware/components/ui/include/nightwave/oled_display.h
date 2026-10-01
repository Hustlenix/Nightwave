#pragma once
#include "nightwave/text_frame.h"
namespace nightwave {
enum class OledController { kSh1106, kSsd1306 };
class OledDisplay {
 public:
    bool initialize(OledController controller = OledController::kSh1106);
    bool show(const TextFrame&);
    bool sleep(bool asleep);
 private:
    bool command(const std::uint8_t* bytes, std::size_t count);
    void* bus_{nullptr};
    void* device_{nullptr};
    OledController controller_{OledController::kSh1106};
};
}  // namespace nightwave
