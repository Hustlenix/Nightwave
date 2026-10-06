#pragma once
#include <array>
#include "nightwave/display_sink.h"
#include "nightwave/selected_display_profile.h"

namespace nightwave {
class St7789Bus {
 public:
    virtual ~St7789Bus() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool reset(bool high) = 0;
    virtual void delay_ms(std::uint32_t ms) = 0;
    virtual bool command(std::uint8_t code, const std::uint8_t* data, std::size_t count) = 0;
    virtual bool pixels(const std::uint8_t* data, std::size_t count) = 0;
    virtual bool backlight(bool on) = 0;
};

// Synchronous, single UiTask owner. Owns a small tile, never a frame buffer or
// borrowed string beyond present(). ASCII glyphs only; no Unicode claim.
class St7789Display final : public DisplaySink {
 public:
    explicit St7789Display(St7789Bus& bus) : bus_(bus) {}
    bool begin() override;
    DisplayCapabilities capabilities() const override { return {240, 280, true, false, true}; }
    bool present(const DisplayFrame&) override;
    bool sleep(bool asleep) override;
 private:
    bool fail();
    St7789Bus& bus_;
    alignas(4) std::array<std::uint8_t, SelectedDisplayProfile::tile_bytes> tile_{};
    bool ready_{false}, asleep_{false};
};
} // namespace nightwave
