#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "freertos/task.h"
#include "nightwave/streaming_player.h"
#include "nightwave/gain_ramp.h"
#include "nightwave/player_navigation.h"
#include "nightwave/playback_state_machine.h"
#include "nightwave/text_frame.h"

// Real StreamingPlayer and real decoder, simulated RTOS scheduling/I2S only.
// These counters/timings are NOT board measurements or DMA verification.
struct FakeTask {
    std::thread thread;
    std::mutex mutex;
    std::condition_variable changed;
    bool notified{false}, cancelled{false};
};
namespace {
thread_local FakeTask* current = nullptr;
std::vector<std::unique_ptr<FakeTask>> tasks;
int create_count = 0, fail_create = 0, failures = 0;
std::atomic<bool> fault{false}, muted{true};
std::atomic<unsigned> writes{0};
#define CHECK(value) do { if (!(value)) { ++failures; std::cerr << __LINE__ << ": " << #value << '\n'; } } while(false)
void join_all() { for (auto& t : tasks) if (t->thread.joinable()) t->thread.join(); tasks.clear(); }
bool await(const std::function<bool()>& predicate) {
    for (int i = 0; i < 10000; ++i) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    return predicate();
}
}
BaseType_t xTaskCreate(void (*entry)(void*), const char*, std::uint32_t,
                      void* ctx, unsigned, TaskHandle_t* handle) {
    if (++create_count == fail_create) return 0;
    auto task = std::make_unique<FakeTask>();
    auto* raw = task.get(); *handle = raw;
    raw->thread = std::thread([raw, entry, ctx]() {
        current = raw;
        std::unique_lock<std::mutex> lock(raw->mutex);
        raw->changed.wait(lock, [raw] { return raw->notified || raw->cancelled; });
        if (raw->cancelled) return;
        lock.unlock(); entry(ctx);
    });
    tasks.push_back(std::move(task)); return pdPASS;
}
void vTaskDelete(TaskHandle_t task) {
    if (!task) return;
    { std::lock_guard<std::mutex> lock(task->mutex); task->cancelled = true; }
    task->changed.notify_all();
    if (task->thread.joinable()) task->thread.join();
}
void xTaskNotifyGive(TaskHandle_t task) {
    { std::lock_guard<std::mutex> lock(task->mutex); task->notified = true; }
    task->changed.notify_all();
}
std::uint32_t ulTaskNotifyTake(int, TickType_t) {
    std::unique_lock<std::mutex> lock(current->mutex);
    current->changed.wait(lock, [] { return current->notified; }); return 1;
}
void vTaskDelay(TickType_t) { std::this_thread::sleep_for(std::chrono::microseconds(100)); }
unsigned uxTaskGetStackHighWaterMark(TaskHandle_t) { return 0; }
std::int64_t esp_timer_get_time() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
namespace nightwave {
bool I2sAudioSink::initialize_safe_outputs() { muted = true; return true; }
AudioSinkStatus I2sAudioSink::configure(const AudioFormat& format) {
    if (!format.supported()) return AudioSinkStatus::kFault;
    format_ = format; channel_ = this; return AudioSinkStatus::kAccepted;
}
AudioSinkStatus I2sAudioSink::write(const PcmBlock& block) {
    if (fault || !channel_ || !valid_stereo_block(block, format_)) return AudioSinkStatus::kFault;
    ++writes; std::this_thread::sleep_for(std::chrono::microseconds(100));
    return AudioSinkStatus::kAccepted;
}
void I2sAudioSink::stop() { channel_ = nullptr; output_ = OutputPath::kMuted; muted = true; }
bool I2sAudioSink::select_output(OutputPath output) {
    output_ = output; muted = output == OutputPath::kMuted; return true;
}
AudioSinkStatus I2sAudioSink::drain() {
    return channel_ ? AudioSinkStatus::kAccepted : AudioSinkStatus::kNotReady;
}
}
int main(int argc, char** argv) {
    using namespace nightwave;
    if (argc != 3) return EXIT_FAILURE;
    auto player = std::make_unique<StreamingPlayer>();
    I2sAudioSink sink;
    CHECK(!player->start("missing-file", sink, OutputPath::kLine, 8));
    for (int failed = 1; failed <= 3; ++failed) {
        create_count = 0; fail_create = failed;
        CHECK(!player->start(argv[1], sink, OutputPath::kLine, 8));
        CHECK(!player->playing() && muted); join_all();
    }
    fail_create = 0;
    for (int i = 0; i < 1000; ++i) {
        CHECK(player->start(argv[1 + i % 2], sink, OutputPath::kLine, 8));
        CHECK(!player->start(argv[1], sink, OutputPath::kLine, 8));
        player->pause(true); CHECK(player->paused());
        player->pause(false); CHECK(!player->paused());
        CHECK(player->stop()); join_all(); CHECK(muted && !player->playing());
    }
    CHECK(player->start(argv[1], sink, OutputPath::kLine, 8));
    CHECK(await([] { return writes > 0 && !muted; }));
    player->pause(true);
    CHECK(await([] { return muted.load(); }));
    const auto paused_writes = writes.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(writes == paused_writes);
    player->pause(false); CHECK(await([] { return !muted; }));
    fault = true; CHECK(await([&] { return !player->playing(); }));
    join_all(); CHECK(player->errors() > 0 && muted); fault = false;
    for (int i = 1; i <= 2; ++i) {
        CHECK(player->start(argv[i], sink, OutputPath::kLine, 8));
        CHECK(await([&] { return !player->playing(); }));
        join_all(); CHECK(player->errors() == 0 && muted);
    }
    GainRamp ramp;
    for (int i = 0; i < 1024; ++i) ramp.step(32767);
    CHECK(ramp.value() == 32767);
    for (int i = 0; i < 1024; ++i) ramp.step(0);
    CHECK(ramp.value() == 0);
    PlayerNavigation nav; nav.populate(128);
    std::array<unsigned, 65> ring_memory{};
    PcmRingBuffer<unsigned> ring(ring_memory.data(), ring_memory.size());
    std::thread producer([&] {
        for (unsigned i = 0; i < 100000; ++i)
            while (!ring.push(i)) std::this_thread::yield();
    });
    for (unsigned expected = 0; expected < 100000; ++expected) {
        unsigned value;
        while (!ring.pop(value)) std::this_thread::yield();
        CHECK(value == expected);
    }
    producer.join(); CHECK(ring.empty());
    for (int i = 0; i < 1000; ++i) { nav.move(1); CHECK(nav.cursor < 128); nav.move(-1); CHECK(nav.cursor == 0); }
    nav.tick(30000); CHECK(nav.asleep); CHECK(!nav.interact(30001)); CHECK(nav.interact(30002));
    nav.last_input_ms = UINT32_MAX - 10; nav.tick(20); CHECK(!nav.asleep);
    TextFrame text; text.line(0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    CHECK(text.rows[0][21] == 0); text.line(100, "ignored");
    const auto bitmap = rasterize(text); CHECK(bitmap[0] != 0);
    PlaybackStateMachine state;
    for (int i = 0; i < 1000; ++i) {
        CHECK(state.apply({PlaybackCommandType::kPlay, 0, 0}));
        CHECK(state.apply({PlaybackCommandType::kPause, 0, 0}));
        CHECK(state.apply({PlaybackCommandType::kTogglePause, 0, 0}));
        CHECK(state.apply({PlaybackCommandType::kStop, 0, 0}));
    }
    if (failures) return EXIT_FAILURE;
    std::cout << "Streaming stress passed: 1000 alternating starts/cancels and pause/resume control cycles; observed pause hold/resume, faults, EOF, rollback\n";
}
