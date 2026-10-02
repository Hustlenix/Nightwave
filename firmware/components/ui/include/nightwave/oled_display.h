#pragma once
#include "nightwave/display_sink.h"
namespace nightwave {
enum class OledController { kSh1106, kSsd1306 };
class OledDisplay : public DisplaySink {
 public:
    bool begin() override { return initialize(); }
    DisplayCapabilities capabilities() const override { return {128, 64, false, false, true}; }
    bool present(const DisplayFrame& frame) override { return show(frame.fallback); }
    bool initialize(OledController controller = OledController::kSh1106);
    bool show(const TextFrame&);
    bool sleep(bool asleep) override;
 private:
    bool command(const std::uint8_t* bytes, std::size_t count);
    void* bus_{nullptr};
    void* device_{nullptr};
    OledController controller_{OledController::kSh1106};
};
}  // namespace nightwave
