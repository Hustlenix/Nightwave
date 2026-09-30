#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "nightwave/audio_math.h"
#include "nightwave/button_debouncer.h"
#include "nightwave/pcm_ring_buffer.h"
#include "nightwave/playback_state_machine.h"
#include "nightwave/wav_parser.h"

namespace {

int failures = 0;

#define CHECK(expression)                                                     \
    do {                                                                      \
        if (!(expression)) {                                                  \
            std::cerr << __FILE__ << ':' << __LINE__ << " CHECK failed: "    \
                      << #expression << '\n';                                 \
            ++failures;                                                       \
        }                                                                     \
    } while (false)

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void append_id(std::vector<std::uint8_t>& bytes, const char* id) {
    bytes.insert(bytes.end(), id, id + 4);
}

std::vector<std::uint8_t> make_wav(bool unknown_chunk = true) {
    std::vector<std::uint8_t> body;
    if (unknown_chunk) {
        append_id(body, "JUNK");
        append_u32(body, 3);
        body.insert(body.end(), {1, 2, 3, 0});
    }
    append_id(body, "fmt ");
    append_u32(body, 16);
    append_u16(body, 1);
    append_u16(body, 2);
    append_u32(body, 44100);
    append_u32(body, 44100 * 4);
    append_u16(body, 4);
    append_u16(body, 16);
    append_id(body, "data");
    append_u32(body, 8);
    body.insert(body.end(), 8, 0);

    std::vector<std::uint8_t> result;
    append_id(result, "RIFF");
    append_u32(result, static_cast<std::uint32_t>(body.size() + 4));
    append_id(result, "WAVE");
    result.insert(result.end(), body.begin(), body.end());
    return result;
}

bool vector_reader(void* context, std::uint64_t offset,
                   std::uint8_t* destination, std::size_t length) {
    const auto& bytes = *static_cast<std::vector<std::uint8_t>*>(context);
    if (offset > bytes.size() || length > bytes.size() - offset) return false;
    std::copy_n(bytes.data() + offset, length, destination);
    return true;
}

void test_wav_parser() {
    auto bytes = make_wav();
    const auto result = nightwave::parse_wav(vector_reader, &bytes, bytes.size());
    CHECK(result.status == nightwave::WavParseStatus::kOk);
    CHECK(result.info.format.sample_rate_hz == 44100);
    CHECK(result.info.format.channel_count == 2);
    CHECK(result.info.format.bits_per_sample == 16);
    CHECK(result.info.data_size == 8);
    CHECK(result.info.unknown_chunk_count == 1);

    auto truncated = bytes;
    truncated.pop_back();
    CHECK(nightwave::parse_wav(vector_reader, &truncated, truncated.size()).status ==
          nightwave::WavParseStatus::kMalformedChunk);

    auto unsupported = make_wav(false);
    unsupported[34] = 24;
    CHECK(nightwave::parse_wav(vector_reader, &unsupported, unsupported.size()).status ==
          nightwave::WavParseStatus::kUnsupportedFormat);
}

void test_generated_fixture(const char* path) {
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    const auto result = nightwave::parse_wav(vector_reader, &bytes, bytes.size());
    CHECK(result.status == nightwave::WavParseStatus::kOk);
    CHECK(result.info.format.sample_rate_hz == 44100);
    CHECK(result.info.data_size == 44100U * 6U * 4U);
}

void test_ring_buffer() {
    std::array<int, 5> storage{};
    nightwave::PcmRingBuffer<int> ring(storage.data(), storage.size());
    CHECK(ring.capacity() == 4);
    for (int value = 0; value < 4; ++value) CHECK(ring.push(value));
    CHECK(!ring.push(4));
    int value = -1;
    CHECK(ring.pop(value) && value == 0);
    CHECK(ring.pop(value) && value == 1);
    CHECK(ring.push(4));
    CHECK(ring.push(5));
    for (int expected = 2; expected <= 5; ++expected) {
        CHECK(ring.pop(value) && value == expected);
    }
    CHECK(!ring.pop(value));
}

void test_audio_math() {
    CHECK(nightwave::stereo_to_mono(32767, 32767) == 32767);
    CHECK(nightwave::stereo_to_mono(-32768, -32768) == -32768);
    CHECK(nightwave::stereo_to_mono(32767, -32768) == 0);
    CHECK(nightwave::scale_sample_q15(32767, nightwave::percent_to_q15(100)) ==
          32766);
    CHECK(nightwave::scale_sample_q15(20000, nightwave::percent_to_q15(50)) ==
          9999);
}

void test_button_debounce() {
    nightwave::ButtonDebouncer button(25);
    CHECK(!button.update(true, 10));
    CHECK(!button.update(false, 15));
    CHECK(!button.update(true, 20));
    CHECK(!button.update(true, 44));
    CHECK(button.update(true, 45));
    CHECK(button.pressed());
    CHECK(!button.update(false, 60));
    CHECK(button.update(false, 85));
    CHECK(!button.pressed());
}

void test_playback_state() {
    nightwave::PlaybackStateMachine state;
    CHECK(state.state() == nightwave::AppState::kIdle);
    CHECK(state.apply({nightwave::PlaybackCommandType::kPlay, 0, 0}));
    CHECK(state.state() == nightwave::AppState::kPlaying);
    CHECK(state.apply({nightwave::PlaybackCommandType::kTogglePause, 0, 0}));
    CHECK(state.state() == nightwave::AppState::kPaused);
    CHECK(state.apply({nightwave::PlaybackCommandType::kSetVolume, 150, 0}));
    CHECK(state.volume_percent() == 100);
    CHECK(state.apply({nightwave::PlaybackCommandType::kStop, 0, 0}));
    CHECK(state.state() == nightwave::AppState::kIdle);
}

}  // namespace

int main(int argc, char** argv) {
    test_wav_parser();
    test_ring_buffer();
    test_audio_math();
    test_button_debounce();
    test_playback_state();
    if (argc > 1) test_generated_fixture(argv[1]);
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "Nightwave host tests passed\n";
    return EXIT_SUCCESS;
}
