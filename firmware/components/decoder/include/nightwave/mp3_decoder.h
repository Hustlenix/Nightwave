#pragma once
#include <array>
#include "nightwave/decoder.h"
#include "nightwave/mp3_framer.h"

namespace nightwave {
class Mp3Decoder final : public AudioDecoder {
 public:
    Mp3Decoder();
    ~Mp3Decoder() override;
    Mp3Decoder(const Mp3Decoder&) = delete;
    Mp3Decoder& operator=(const Mp3Decoder&) = delete;
    DecodeResult decode(EncodedBytes input) override { return decode(input, false); }
    DecodeResult decode(EncodedBytes input, bool eof);
    void reset() override;
    bool ready() const { return handle_ != nullptr; }
    std::size_t recovery_bytes() const { return framer_.recovery_bytes(); }
 private:
    void* handle_{nullptr};
    Mp3Framer framer_;
    std::array<std::int16_t, 2304> decoded_{};
    std::array<std::int16_t, 2304> stereo_{};
};
}  // namespace nightwave
