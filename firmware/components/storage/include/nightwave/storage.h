#pragma once

#include <cstddef>
#include <cstdint>

namespace nightwave {

enum class StorageStatus : std::uint8_t {
    kOk,
    kNoMedia,
    kBusy,
    kEndOfFile,
    kIoError,
    kInvalidArgument,
};

struct ReadRequest {
    std::uint64_t offset{0};
    std::uint8_t* destination{nullptr};
    std::size_t capacity{0};
};

struct ReadResult {
    StorageStatus status{StorageStatus::kInvalidArgument};
    std::size_t bytes_read{0};
};

class StorageReader {
  public:
    virtual ~StorageReader() = default;
    virtual ReadResult read(const ReadRequest& request) = 0;
    virtual std::uint64_t size_bytes() const = 0;
};

}  // namespace nightwave
