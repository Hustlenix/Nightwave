#pragma once
#include <array>
#include <cstring>
#include "nightwave/audio_types.h"
namespace nightwave {
enum class BtState { kUnavailable, kIdle, kDiscovering, kConnecting, kConnected, kStarting, kStreaming, kFault, kPairing, kReconnecting };
enum class BtWrite { kAccepted, kWouldBlock, kUnavailable, kFault };
struct BtDevice { std::array<std::uint8_t, 6> address{}; std::array<char, 48> name{}; };
struct BtCapabilities { bool a2dp_source{false}, avrcp{false}; bool rate_44100{false}, rate_48000{false}; };
class BluetoothBackend {
 public:
    virtual ~BluetoothBackend() = default;
    virtual BtCapabilities capabilities() const = 0;
    // Non-blocking command submission. Completion belongs to the matching epoch.
    virtual bool discover(std::uint32_t epoch) = 0;
    virtual bool connect(const BtDevice&, std::uint32_t epoch) = 0;
    virtual bool start(const AudioFormat&, std::uint32_t epoch) = 0;
    virtual void disconnect(std::uint32_t epoch) = 0;
    virtual BtWrite write(const PcmBlock&) = 0;
};
class UnavailableBluetooth final : public BluetoothBackend {
 public:
    BtCapabilities capabilities() const override { return {}; }
    bool discover(std::uint32_t) override { return false; }
    bool connect(const BtDevice&, std::uint32_t) override { return false; }
    bool start(const AudioFormat&, std::uint32_t) override { return false; }
    void disconnect(std::uint32_t) override {}
    BtWrite write(const PcmBlock&) override { return BtWrite::kUnavailable; }
};
// Single serialized control owner. ISR/vendor callbacks enqueue fixed events;
// adapters must not call this concurrently or access SD/UI from callbacks.
class BluetoothSource {
 public:
    static constexpr std::size_t kDevices = 8;
    explicit BluetoothSource(BluetoothBackend& backend) : backend_(backend), state_(backend.capabilities().a2dp_source ? BtState::kIdle : BtState::kUnavailable) {}
    BtState state() const { return state_; }
    std::uint32_t epoch() const { return epoch_; }
    std::size_t count() const { return count_; }
    const BtDevice* device(std::size_t n) const { return n < count_ ? &devices_[n] : nullptr; }
    // Remembered address is a reconnect preference, not evidence of bonding.
    bool reconnect(const BtDevice& device, std::uint32_t now) {
        if ((state_ != BtState::kIdle && state_ != BtState::kFault) ||
            device.address == std::array<std::uint8_t, 6>{} ||
            !std::memchr(device.name.data(), 0, device.name.size())) return false;
        ++epoch_; started_ = now;
        if (!backend_.connect(device, epoch_)) { fault(); return false; }
        state_ = BtState::kReconnecting; return true;
    }
    bool pairing(std::uint32_t epoch) {
        if (epoch != epoch_ || (state_ != BtState::kConnecting && state_ != BtState::kReconnecting)) return false;
        state_ = BtState::kPairing; return true;
    }
    bool discover(std::uint32_t now) {
        if (state_ != BtState::kIdle && state_ != BtState::kFault) return false;
        ++epoch_; count_ = 0; started_ = now;
        if (!backend_.discover(epoch_)) { state_ = BtState::kFault; return false; }
        state_ = BtState::kDiscovering; return true;
    }
    bool found(const BtDevice& device, std::uint32_t epoch) {
        if (state_ != BtState::kDiscovering || epoch != epoch_ || !std::memchr(device.name.data(), 0, device.name.size()) || device.address == std::array<std::uint8_t, 6>{}) return false;
        for (std::size_t i = 0; i < count_; ++i) if (devices_[i].address == device.address) { devices_[i] = device; return true; }
        if (count_ == kDevices) return false;
        devices_[count_++] = device; return true;
    }
    bool choose(std::size_t n, std::uint32_t now) {
        if (state_ != BtState::kDiscovering || n >= count_) return false;
        ++epoch_; started_ = now;
        if (!backend_.connect(devices_[n], epoch_)) { fault(); return false; }
        state_ = BtState::kConnecting; return true;
    }
    bool connected(std::uint32_t epoch) {
        if ((state_ != BtState::kConnecting && state_ != BtState::kPairing && state_ != BtState::kReconnecting) || epoch != epoch_) return false;
        state_ = BtState::kConnected; return true;
    }
    bool start(const AudioFormat& format, std::uint32_t now) {
        const auto c = backend_.capabilities();
        if (state_ != BtState::kConnected || !format.supported() || format.channel_count != 2 ||
            !(format.sample_rate_hz == 44100 ? c.rate_44100 : format.sample_rate_hz == 48000 && c.rate_48000)) return false;
        format_ = format; started_ = now;
        if (!backend_.start(format, epoch_)) { fault(); return false; }
        state_ = BtState::kStarting; return true;
    }
    bool streaming(std::uint32_t epoch) {
        if (state_ != BtState::kStarting || epoch != epoch_) return false;
        state_ = BtState::kStreaming; return true;
    }
    BtWrite write(const PcmBlock& block) {
        if (state_ != BtState::kStreaming) return BtWrite::kUnavailable;
        if (block.frame_count > 1024 || !valid_stereo_block(block, format_)) {
            fault();
            return BtWrite::kFault;
        }
        const auto result = backend_.write(block);
        if (result == BtWrite::kFault || result == BtWrite::kUnavailable) fault();
        return result;
    }
    void lost(std::uint32_t epoch) { if (epoch == epoch_ && state_ != BtState::kUnavailable && state_ != BtState::kIdle) fault(); }
    void tick(std::uint32_t now) {
        const auto limit = state_ == BtState::kDiscovering ? 15000u :
            (state_ == BtState::kConnecting || state_ == BtState::kReconnecting || state_ == BtState::kPairing) ? 10000u : state_ == BtState::kStarting ? 5000u : 0u;
        if (limit && now - started_ >= limit) fault();
    }
    void disconnect() {
        if (state_ == BtState::kUnavailable) return;
        backend_.disconnect(epoch_); ++epoch_; state_ = BtState::kIdle; stop_requested_ = true;
    }
    bool take_stop_request() { const bool stop = stop_requested_; stop_requested_ = false; return stop; }
 private:
    void fault() { backend_.disconnect(epoch_); ++epoch_; state_ = BtState::kFault; stop_requested_ = true; }
    BluetoothBackend& backend_; std::array<BtDevice, kDevices> devices_{};
    AudioFormat format_{}; std::uint32_t epoch_{0}, started_{0}; std::size_t count_{0};
    BtState state_; bool stop_requested_{false};
};
} // namespace nightwave
