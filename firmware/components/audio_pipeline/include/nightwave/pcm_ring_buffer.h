#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace nightwave {

// Lock-free single-producer/single-consumer ring. One slot is reserved so
// full and empty states remain distinguishable without a mutex.
template <typename T>
class PcmRingBuffer {
  public:
    PcmRingBuffer(T* storage, std::size_t slot_count)
        : storage_(storage), slot_count_(slot_count) {}

    bool push(const T& value) {
        if (storage_ == nullptr || slot_count_ < 2) {
            return false;
        }
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        storage_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& value) {
        if (storage_ == nullptr || slot_count_ < 2) {
            return false;
        }
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        value = storage_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

    std::size_t size() const {
        if (slot_count_ < 2) {
            return 0;
        }
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        return head >= tail ? head - tail : slot_count_ - tail + head;
    }

    std::size_t capacity() const { return slot_count_ > 1 ? slot_count_ - 1 : 0; }
    bool empty() const { return size() == 0; }

    void reset() {
        tail_.store(0, std::memory_order_release);
        head_.store(0, std::memory_order_release);
    }

  private:
    std::size_t increment(std::size_t value) const {
        ++value;
        return value == slot_count_ ? 0 : value;
    }

    T* storage_{nullptr};
    std::size_t slot_count_{0};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

struct StereoFrame {
    std::int16_t left{0};
    std::int16_t right{0};
};

}  // namespace nightwave
