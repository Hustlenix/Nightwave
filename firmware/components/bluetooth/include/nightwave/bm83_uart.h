#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace nightwave {
// DS50002896A §5.2–5.3 framing only. AT command/event semantics must be
// qualified against the selected AT package, not inferred from MSPK commands.
struct Bm83Packet { std::array<std::uint8_t, 256> body{}; std::size_t size{0}; };
enum class Bm83Parse { kWaiting, kPacket, kDropped };
class Bm83Uart {
 public:
    void reset() { position_ = length_ = 0; sum_ = 0; }
    Bm83Parse push(std::uint8_t byte, std::uint32_t now, Bm83Packet& packet) {
        packet.size = 0;
        if (position_ && now - last_ > 100) reset();
        last_ = now;
        if (!position_) { if (byte == 0xaa) position_ = 1; return Bm83Parse::kWaiting; }
        if (position_ == 1) { length_ = static_cast<std::size_t>(byte) << 8; sum_ = byte; ++position_; return Bm83Parse::kWaiting; }
        if (position_ == 2) {
            length_ |= byte; sum_ = static_cast<std::uint8_t>(sum_ + byte); ++position_;
            if (!length_ || length_ > packet.body.size()) { reset(); return Bm83Parse::kDropped; }
            return Bm83Parse::kWaiting;
        }
        sum_ = static_cast<std::uint8_t>(sum_ + byte);
        if (position_ < length_ + 3) { body_[position_++ - 3] = byte; return Bm83Parse::kWaiting; }
        const auto length = length_; const bool valid = sum_ == 0;
        if (valid) { std::memcpy(packet.body.data(), body_.data(), length); packet.size = length; }
        reset(); return valid ? Bm83Parse::kPacket : Bm83Parse::kDropped;
    }
    static std::size_t encode(const std::uint8_t* body, std::size_t size, std::uint8_t* out, std::size_t capacity) {
        if (!body || !out || !size || size > 256 || capacity < size + 4) return 0;
        out[0] = 0xaa; out[1] = static_cast<std::uint8_t>(size >> 8); out[2] = static_cast<std::uint8_t>(size);
        std::uint8_t sum = static_cast<std::uint8_t>(out[1] + out[2]);
        for (std::size_t i = 0; i < size; ++i) { out[3 + i] = body[i]; sum = static_cast<std::uint8_t>(sum + body[i]); }
        out[size + 3] = static_cast<std::uint8_t>(0u - sum); return size + 4;
    }
 private:
    std::array<std::uint8_t, 256> body_{};
    std::size_t position_{0}, length_{0}; std::uint32_t last_{0}; std::uint8_t sum_{0};
};
}
