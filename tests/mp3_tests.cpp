#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>
#include "nightwave/mp3_decoder.h"

namespace {
int failures = 0;
#define CHECK(expr) do { if (!(expr)) { ++failures; \
    std::cerr << __LINE__ << ": " << #expr << '\n'; } } while (false)
void framing() {
    using namespace nightwave;
    Mp3Framer framer;
    std::vector<std::uint8_t> frame(417, 0);
    frame[0] = 255; frame[1] = 0xfb; frame[2] = 0x90;
    auto result = framer.scan(frame.data(), frame.size(), true);
    CHECK(result.status == Mp3ScanStatus::kFrame && result.bytes == 417);
    CHECK(result.rate == 44100 && result.channels == 2);
    CHECK(framer.scan(frame.data(), 416, false).status == Mp3ScanStatus::kNeedInput);
    CHECK(framer.scan(frame.data(), 416, true).status == Mp3ScanStatus::kInvalid);
    frame[2] = 0;
    CHECK(framer.scan(frame.data(), frame.size(), true).status == Mp3ScanStatus::kUnsupported);
    std::array<std::uint8_t, 10> tag{'I','D','3',4,0,0x10,0,0,0,2};
    framer.reset();
    CHECK(framer.scan(tag.data(), 9, false).status == Mp3ScanStatus::kNeedInput);
    CHECK(framer.scan(tag.data(), 10, false).bytes == 10);
    std::array<std::uint8_t, 12> tag_payload{};
    CHECK(framer.scan(tag_payload.data(), 12, false).bytes == 12);
    CHECK(framer.scan(nullptr, 0, true).status == Mp3ScanStatus::kEnd);
    framer.reset(); tag[6] = 0x80;
    CHECK(framer.scan(tag.data(), 10, false).status == Mp3ScanStatus::kInvalid);
    framer.reset(); tag[6] = 0;
    CHECK(framer.scan(tag.data(), 10, true).status == Mp3ScanStatus::kSkip);
    CHECK(framer.scan(nullptr, 0, true).status == Mp3ScanStatus::kInvalid);
    framer.reset();
    std::array<std::uint8_t, 4> garbage{};
    for (std::size_t i = 0; i < Mp3Framer::kRecoveryBudget; ++i)
        CHECK(framer.scan(garbage.data(), garbage.size(), false).bytes == 1);
    CHECK(framer.scan(garbage.data(), garbage.size(), false).status == Mp3ScanStatus::kInvalid);
    CHECK(framer.recovery_bytes() == Mp3Framer::kRecoveryBudget);
}

void decode_file(const char* path, std::size_t fragment, bool mono) {
    using namespace nightwave;
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    std::vector<std::uint8_t> source((std::istreambuf_iterator<char>(input)), {});
    CHECK(!source.empty());
    Mp3Decoder decoder;
    CHECK(decoder.ready());
    std::array<std::uint8_t, 4096> pending{};
    std::size_t cursor = 0, used = 0, frames = 0, iterations = 0;
    bool ended = false;
    while (++iterations < source.size() * 2 + 100) {
        const auto take = std::min({fragment, pending.size() - used, source.size() - cursor});
        std::copy_n(source.data() + cursor, take, pending.data() + used);
        cursor += take; used += take;
        const auto result = decoder.decode({pending.data(), used}, cursor == source.size());
        CHECK(result.bytes_consumed <= used);
        if (result.bytes_consumed > used) break;
        if (result.status == DecodeStatus::kFrameReady) {
            CHECK(result.pcm.format.supported() && result.pcm.format.channel_count == 2);
            CHECK(result.pcm.frame_count > 0 && result.pcm.frame_count <= 1152);
            frames += result.pcm.frame_count;
            if (mono) for (std::size_t i = 0; i < result.pcm.frame_count; ++i)
                CHECK(result.pcm.interleaved_samples[i * 2] == result.pcm.interleaved_samples[i * 2 + 1]);
        }
        if (result.status == DecodeStatus::kEndOfStream) { ended = true; break; }
        if (result.status != DecodeStatus::kNeedInput && result.status != DecodeStatus::kFrameReady) {
            std::cerr << "Decode failure in " << path << " cursor=" << cursor << '\n';
            CHECK(false); break;
        }
        std::move(pending.begin() + result.bytes_consumed, pending.begin() + used, pending.begin());
        used -= result.bytes_consumed;
        if (result.bytes_consumed == 0 && take == 0) { CHECK(false); break; }
    }
    CHECK(ended && frames > 0);
    decoder.reset(); CHECK(decoder.ready());
    CHECK(decoder.decode({nullptr, 0}, true).status == DecodeStatus::kEndOfStream);
    std::cout << "Decoded " << path << " frames=" << frames << " fragment=" << fragment << '\n';
}
void corrupt_corpus() {
    using namespace nightwave;
    std::mt19937 random(0x4e574156);
    Mp3Decoder decoder;
    for (std::size_t size = 0; size < 16384; size += 67) {
        decoder.reset();
        std::vector<std::uint8_t> bytes(size);
        for (auto& byte : bytes) byte = static_cast<std::uint8_t>(random());
        std::size_t consumed = 0;
        bool terminal = false;
        for (std::size_t step = 0; step <= bytes.size() + 1; ++step) {
            const auto result = decoder.decode({bytes.empty() ? nullptr : bytes.data() + consumed,
                bytes.size() - consumed}, true);
            CHECK(result.bytes_consumed <= bytes.size() - consumed);
            if (result.status == DecodeStatus::kEndOfStream || result.status == DecodeStatus::kMalformedStream ||
                result.status == DecodeStatus::kUnsupported) { terminal = true; break; }
            if (!result.bytes_consumed) break;
            consumed += result.bytes_consumed;
        }
        CHECK(terminal);
    }
    decoder.reset();
    std::array<std::uint8_t, 10> overlarge{'I','D','3',4,0,0,0x7f,0x7f,0x7f,0x7f};
    CHECK(decoder.decode({overlarge.data(), overlarge.size()}, true).status == DecodeStatus::kUnsupported);
    decoder.reset();
    std::array<std::uint8_t, 20> truncated{};
    truncated[0] = 255; truncated[1] = 0xfb; truncated[2] = 0x90;
    CHECK(decoder.decode({truncated.data(), truncated.size()}, true).status == DecodeStatus::kMalformedStream);
}
}
int main(int argc, char** argv) {
    framing();
    corrupt_corpus();
    for (int i = 1; i < argc; ++i) {
        decode_file(argv[i], 1, i > 1);
        decode_file(argv[i], 127, i > 1);
        decode_file(argv[i], 4096, i > 1);
    }
    if (failures) return EXIT_FAILURE;
    std::cout << "Nightwave MP3 tests passed\n";
}
