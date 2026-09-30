#include "nightwave/wav_parser.h"

#include <array>
#include <cstring>
#include <limits>

namespace nightwave {
namespace {

std::uint16_t le16(const std::uint8_t* value) {
    return static_cast<std::uint16_t>(value[0]) |
           static_cast<std::uint16_t>(value[1] << 8U);
}

std::uint32_t le32(const std::uint8_t* value) {
    return static_cast<std::uint32_t>(value[0]) |
           (static_cast<std::uint32_t>(value[1]) << 8U) |
           (static_cast<std::uint32_t>(value[2]) << 16U) |
           (static_cast<std::uint32_t>(value[3]) << 24U);
}

bool id_is(const std::uint8_t* value, const char* expected) {
    return std::memcmp(value, expected, 4) == 0;
}

bool read_exact(WavReadCallback reader, void* context, std::uint64_t offset,
                std::uint8_t* destination, std::size_t length,
                std::uint64_t file_size) {
    return offset <= file_size && length <= file_size - offset &&
           reader(context, offset, destination, length);
}

}  // namespace

WavParseResult parse_wav(WavReadCallback reader, void* context,
                         std::uint64_t file_size) {
    WavParseResult result{};
    if (reader == nullptr) {
        result.status = WavParseStatus::kIoError;
        return result;
    }

    std::array<std::uint8_t, 12> riff{};
    if (!read_exact(reader, context, 0, riff.data(), riff.size(), file_size)) {
        result.status = WavParseStatus::kTruncated;
        return result;
    }
    if (!id_is(riff.data(), "RIFF") || !id_is(riff.data() + 8, "WAVE")) {
        result.status = WavParseStatus::kNotRiffWave;
        return result;
    }

    bool have_format = false;
    bool have_data = false;
    std::uint64_t offset = 12;
    while (offset + 8 <= file_size) {
        std::array<std::uint8_t, 8> header{};
        if (!read_exact(reader, context, offset, header.data(), header.size(),
                        file_size)) {
            result.status = WavParseStatus::kIoError;
            return result;
        }
        const auto chunk_size = static_cast<std::uint64_t>(le32(header.data() + 4));
        const auto payload_offset = offset + 8;
        if (payload_offset > file_size || chunk_size > file_size - payload_offset) {
            result.status = WavParseStatus::kMalformedChunk;
            return result;
        }

        if (id_is(header.data(), "fmt ")) {
            if (chunk_size < 16) {
                result.status = WavParseStatus::kMalformedChunk;
                return result;
            }
            std::array<std::uint8_t, 16> format{};
            if (!read_exact(reader, context, payload_offset, format.data(),
                            format.size(), file_size)) {
                result.status = WavParseStatus::kIoError;
                return result;
            }
            const auto codec = le16(format.data());
            if (codec != 1) {
                result.status = WavParseStatus::kUnsupportedCodec;
                return result;
            }
            result.info.format.channel_count =
                static_cast<std::uint8_t>(le16(format.data() + 2));
            result.info.format.sample_rate_hz = le32(format.data() + 4);
            result.info.byte_rate = le32(format.data() + 8);
            result.info.block_align = le16(format.data() + 12);
            result.info.format.bits_per_sample =
                static_cast<std::uint8_t>(le16(format.data() + 14));
            if (!result.info.format.supported() ||
                result.info.block_align !=
                    result.info.format.channel_count * sizeof(std::int16_t) ||
                result.info.byte_rate != result.info.format.sample_rate_hz *
                                             result.info.block_align) {
                result.status = WavParseStatus::kUnsupportedFormat;
                return result;
            }
            have_format = true;
        } else if (id_is(header.data(), "data")) {
            result.info.data_offset = payload_offset;
            result.info.data_size = chunk_size;
            have_data = true;
        } else {
            ++result.info.unknown_chunk_count;
        }

        if (have_format && have_data) {
            result.status = WavParseStatus::kOk;
            return result;
        }
        const auto padded_size = chunk_size + (chunk_size & 1U);
        if (padded_size > std::numeric_limits<std::uint64_t>::max() - payload_offset) {
            result.status = WavParseStatus::kMalformedChunk;
            return result;
        }
        offset = payload_offset + padded_size;
    }

    result.status = have_format ? WavParseStatus::kMissingData
                                : WavParseStatus::kMissingFormat;
    return result;
}

const char* wav_status_name(WavParseStatus status) {
    switch (status) {
        case WavParseStatus::kOk: return "ok";
        case WavParseStatus::kIoError: return "io_error";
        case WavParseStatus::kTruncated: return "truncated";
        case WavParseStatus::kNotRiffWave: return "not_riff_wave";
        case WavParseStatus::kMissingFormat: return "missing_format";
        case WavParseStatus::kMissingData: return "missing_data";
        case WavParseStatus::kUnsupportedCodec: return "unsupported_codec";
        case WavParseStatus::kUnsupportedFormat: return "unsupported_format";
        case WavParseStatus::kMalformedChunk: return "malformed_chunk";
    }
    return "unknown";
}

}  // namespace nightwave
