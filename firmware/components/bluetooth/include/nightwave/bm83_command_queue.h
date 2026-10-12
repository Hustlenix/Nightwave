#pragma once
#include "nightwave/bm83_uart.h"
namespace nightwave {
// Serialized owner only. ACK interpretation belongs to the version-specific
// adapter. Never retry a possibly non-idempotent command after an ambiguous ACK.
class Bm83CommandQueue {
 public:
    static constexpr std::size_t kCapacity=4;
    enum class State { kIdle, kSending, kAwaitingAck, kFault };
    struct View { const std::uint8_t* data; std::size_t size; };
    bool enqueue(const Bm83Packet& packet, std::uint32_t generation) {
        if (state_==State::kFault || generation!=generation_ || count_==kCapacity ||
            !packet.size || packet.size>packet.body.size()) return false;
        auto& slot=slots_[(head_+count_)%kCapacity];
        slot.size=Bm83Uart::encode(packet.body.data(),packet.size,slot.bytes.data(),slot.bytes.size());
        ++count_; return true;
    }
    View pending(std::uint32_t now) {
        tick(now);
        if (state_==State::kFault || state_==State::kAwaitingAck || !count_) return {nullptr,0};
        if (state_==State::kIdle) { state_=State::kSending; started_=now; offset_=0; }
        const auto& slot=slots_[head_];
        return {slot.bytes.data()+offset_,slot.size-offset_};
    }
    bool sent(std::size_t bytes, std::uint32_t now) {
        tick(now);
        if (state_!=State::kSending || bytes>slots_[head_].size-offset_) { fail(); return false; }
        offset_+=bytes;
        if (offset_==slots_[head_].size) { state_=State::kAwaitingAck; started_=now; }
        return true;
    }
    bool acknowledge(std::uint8_t opcode, bool success, std::uint32_t generation, std::uint32_t now) {
        tick(now);
        if (state_!=State::kAwaitingAck || generation!=generation_ ||
            opcode!=slots_[head_].bytes[3]) return false;
        if (!success) { fail(); return false; }
        head_=(head_+1)%kCapacity; --count_; offset_=0; state_=State::kIdle; return true;
    }
    void tick(std::uint32_t now) {
        if ((state_==State::kSending || state_==State::kAwaitingAck) && now-started_>=1000) fail();
    }
    void fail() { state_=State::kFault; count_=0; offset_=0; }
    // Call only after transport drain/module resynchronization. Epoch cannot
    // distinguish two indistinguishable wire ACKs from the same command opcode.
    void reset_after_resync() { ++generation_; head_=count_=offset_=0; state_=State::kIdle; }
    std::uint32_t generation() const { return generation_; }
    std::size_t size() const { return count_; }
    State state() const { return state_; }
 private:
    struct Slot { std::array<std::uint8_t,260> bytes{}; std::size_t size{0}; };
    std::array<Slot,kCapacity> slots_{};
    std::size_t head_{0},count_{0},offset_{0};
    std::uint32_t generation_{0},started_{0};
    State state_{State::kIdle};
};
}
