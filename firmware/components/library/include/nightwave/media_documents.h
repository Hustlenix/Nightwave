#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace nightwave {
enum class DocumentStatus { kOk, kMissing, kMalformed, kLimit, kIoError };
struct TrackMetadata {
    std::array<char, 96> title{};
    std::array<char, 64> artist{}, album{};
    std::uint32_t duration_ms{0}; // Zero means unknown, not guessed.
};
struct LyricLine { std::uint32_t time_ms{0}; std::array<char, 160> text{}; };
class Lyrics {
 public:
    static constexpr std::size_t kCapacity = 256;
    DocumentStatus load(const char* media_path);
    DocumentStatus parse(std::FILE*);
    void clear() { count_ = 0; }
    std::size_t size() const { return count_; }
    const LyricLine* current(std::uint32_t position_ms) const;
    const LyricLine* next(std::uint32_t position_ms) const;
 private:
    std::array<LyricLine, kCapacity> lines_{};
    std::size_t count_{0};
};
class Playlist {
 public:
    static constexpr std::size_t kCapacity = 128;
    // Local relative paths only. No URL fetching, absolute paths or root escapes.
    DocumentStatus load(const char* path, const char* root);
    std::size_t size() const { return count_; }
    const char* path(std::size_t index) const { return index < count_ ? paths_[index].data() : nullptr; }
 private:
    std::array<std::array<char, 256>, kCapacity> paths_{};
    std::size_t count_{0};
};
DocumentStatus read_metadata(const char* path, TrackMetadata&);
bool local_media_path(const char* base, const char* relative, const char* root, char* result, std::size_t capacity);
}
