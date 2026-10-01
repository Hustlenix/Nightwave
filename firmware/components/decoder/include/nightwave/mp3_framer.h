#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace nightwave {
enum class Mp3ScanStatus { kFrame, kSkip, kNeedInput, kEnd, kInvalid, kUnsupported };
struct Mp3ScanResult {
    Mp3ScanStatus status{Mp3ScanStatus::kNeedInput};
    std::size_t bytes{0};
    std::uint32_t rate{0};
    std::uint8_t channels{0};
};

// Caller retains unconsumed bytes across refills. Each skip makes progress;
// garbage/decode recovery shares a track-wide byte budget, never reset by sync.
class Mp3Framer {
 public:
    static constexpr std::size_t kRecoveryBudget = 8192;
    static constexpr std::size_t kMaxTagBytes = 16 * 1024 * 1024;
    void reset() { initial_ = true; tag_left_ = 0; recovered_ = 0; }
    std::size_t recovery_bytes() const { return recovered_; }
    bool charge_recovery(std::size_t bytes) {
        if (bytes > kRecoveryBudget - recovered_) return false;
        recovered_ += bytes;
        return true;
    }
    Mp3ScanResult scan(const std::uint8_t* data, std::size_t size, bool eof) {
        if (data == nullptr && size != 0) return {Mp3ScanStatus::kInvalid};
        if (tag_left_) {
            const auto skipped = std::min(size, tag_left_);
            tag_left_ -= skipped;
            if (eof && tag_left_ && skipped == 0) return {Mp3ScanStatus::kInvalid};
            return {skipped ? Mp3ScanStatus::kSkip : Mp3ScanStatus::kNeedInput, skipped};
        }
        if (initial_) {
            if (size < 3 && !eof) return {};
            if (size >= 3 && std::memcmp(data, "ID3", 3) == 0) {
                if (size < 10) return {eof ? Mp3ScanStatus::kInvalid : Mp3ScanStatus::kNeedInput};
                const unsigned version = data[3];
                if (version < 2 || version > 4 || data[4] == 255)
                    return {Mp3ScanStatus::kUnsupported};
                const unsigned reserved = version == 2 ? 0x3f : (version == 3 ? 0x1f : 0x0f);
                if (data[5] & reserved) return {Mp3ScanStatus::kInvalid};
                std::size_t length = 0;
                for (unsigned i = 6; i < 10; ++i) {
                    if (data[i] & 0x80) return {Mp3ScanStatus::kInvalid};
                    length = (length << 7U) | data[i];
                }
                if (length > kMaxTagBytes) return {Mp3ScanStatus::kUnsupported};
                tag_left_ = length + ((version == 4 && (data[5] & 0x10)) ? 10 : 0);
                initial_ = false;
                return {Mp3ScanStatus::kSkip, 10};
            }
            initial_ = false;
        }
        if (size == 0) return {eof ? Mp3ScanStatus::kEnd : Mp3ScanStatus::kNeedInput};
        if (eof && size == 128 && std::memcmp(data, "TAG", 3) == 0)
            return {Mp3ScanStatus::kSkip, size};
        if (size < 4) return {eof ? Mp3ScanStatus::kInvalid : Mp3ScanStatus::kNeedInput};
        const unsigned version = (data[1] >> 3) & 3;
        const unsigned layer = (data[1] >> 1) & 3;
        const unsigned bitrate = data[2] >> 4;
        const unsigned rate_index = (data[2] >> 2) & 3;
        if (data[0] != 255 || (data[1] & 0xe0) != 0xe0 || version == 1 ||
            layer != 1 || bitrate == 15 || rate_index == 3 || (data[3] & 3) == 2) {
            return charge_recovery(1) ? Mp3ScanResult{Mp3ScanStatus::kSkip, 1}
                                      : Mp3ScanResult{Mp3ScanStatus::kInvalid};
        }
        if (bitrate == 0 || version == 0) return {Mp3ScanStatus::kUnsupported};
        constexpr unsigned rates[] = {44100, 48000, 32000};
        constexpr unsigned mpeg1[] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
        constexpr unsigned mpeg2[] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160};
        const unsigned rate = rates[rate_index] / (version == 3 ? 1 : 2);
        if (rate != 22050 && rate != 32000 && rate != 44100 && rate != 48000)
            return {Mp3ScanStatus::kUnsupported};
        const unsigned kbps = version == 3 ? mpeg1[bitrate] : mpeg2[bitrate];
        const std::size_t length = (version == 3 ? 144000U : 72000U) * kbps / rate
                                   + ((data[2] >> 1) & 1);
        if (size < length) return {eof ? Mp3ScanStatus::kInvalid : Mp3ScanStatus::kNeedInput};
        return {Mp3ScanStatus::kFrame, length, rate,
                static_cast<std::uint8_t>((data[3] >> 6) == 3 ? 1 : 2)};
    }
 private:
    bool initial_{true};
    std::size_t tag_left_{0}, recovered_{0};
};
}  // namespace nightwave
