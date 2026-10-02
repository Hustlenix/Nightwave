#include "nightwave/mp3_decoder.h"
#include "mp3dec.h"

namespace nightwave {
Mp3Decoder::Mp3Decoder() { handle_ = MP3InitDecoder(); }
Mp3Decoder::~Mp3Decoder() { if (handle_) MP3FreeDecoder(handle_); }
void Mp3Decoder::reset() {
    if (handle_) MP3FreeDecoder(handle_);
    handle_ = MP3InitDecoder();
    framer_.reset();
}
DecodeResult Mp3Decoder::decode(EncodedBytes input, bool eof) {
    frame_samples_ = frame_rate_ = 0;
    if (!handle_) return {DecodeStatus::kInternalError};
    const auto scan = framer_.scan(input.data, input.size, eof);
    switch (scan.status) {
        case Mp3ScanStatus::kNeedInput: return {};
        case Mp3ScanStatus::kEnd: return {DecodeStatus::kEndOfStream};
        case Mp3ScanStatus::kUnsupported: return {DecodeStatus::kUnsupported};
        case Mp3ScanStatus::kInvalid: return {DecodeStatus::kMalformedStream};
        case Mp3ScanStatus::kSkip: return {DecodeStatus::kNeedInput, scan.bytes};
        case Mp3ScanStatus::kFrame: break;
    }
    frame_samples_ = ((input.data[1] >> 3) & 3) == 3 ? 1152u : 576u;
    frame_rate_ = scan.rate;
    auto* cursor = const_cast<unsigned char*>(input.data);
    int remaining = static_cast<int>(scan.bytes);
    const int error = MP3Decode(handle_, &cursor, &remaining, decoded_.data(), 0);
    if (error != ERR_MP3_NONE) {
        // Reservoir warm-up and corrupt frames both consume a bounded budget.
        // Never retry the same frame indefinitely or publish undefined PCM.
        if (!framer_.charge_recovery(scan.bytes)) return {DecodeStatus::kMalformedStream};
        return {DecodeStatus::kNeedInput, scan.bytes};
    }
    MP3FrameInfo info{};
    MP3GetLastFrameInfo(handle_, &info);
    if (remaining != 0 || info.layer != 3 || info.bitsPerSample != 16 ||
        info.samprate != static_cast<int>(scan.rate) || info.nChans != scan.channels ||
        info.outputSamps <= 0 || info.outputSamps > 2304 ||
        info.outputSamps % info.nChans != 0) return {DecodeStatus::kMalformedStream};
    const auto frames = static_cast<std::size_t>(info.outputSamps / info.nChans);
    if (frames > 1152) return {DecodeStatus::kMalformedStream};
    for (std::size_t i = 0; i < frames; ++i) {
        stereo_[i * 2] = decoded_[i * scan.channels];
        stereo_[i * 2 + 1] = decoded_[i * scan.channels + (scan.channels == 2 ? 1 : 0)];
    }
    return {DecodeStatus::kFrameReady, scan.bytes,
            {stereo_.data(), frames, {scan.rate, 2, 16}, 0}};
}
}  // namespace nightwave
