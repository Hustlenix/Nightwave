#pragma once

#include <atomic>
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

struct StorageBenchmark {
    std::uint64_t bytes_read{0};
    std::uint32_t elapsed_ms{0};
    std::uint32_t average_kib_per_second{0};
    std::uint32_t worst_read_us{0};
    std::uint32_t read_errors{0};
};

class SdStorage {
  public:
    bool mount();
    void unmount();
    bool mounted() const { return mounted_; }
    std::uint32_t errors() const { return errors_.load(); }
    std::size_t enumerate_supported_files() const;
    StorageBenchmark benchmark(const char* path,
                               std::size_t chunk_bytes = 32 * 1024) const;
    const char* mount_point() const { return "/sdcard"; }

  private:
    bool mounted_{false};
    void* card_{nullptr};
    mutable std::atomic<std::uint32_t> errors_{0};
};

}  // namespace nightwave
