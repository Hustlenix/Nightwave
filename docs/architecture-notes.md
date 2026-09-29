# Firmware Architecture Notes

## Data flow

```text
FAT file
  |
  v
StorageTask -- compressed-byte ring --> DecoderTask -- PCM-frame ring --> AudioOutputTask
    |                                      |                                  |
 SD metrics                        format/error events                   I2S DMA + underruns
                                                                               |
                                                         +---------------------+------------------+
                                                         |                                        |
                                                  stereo DAC/HP amp                      mono Class-D
```

Control events travel through a bounded application event queue. Bulk audio never travels through the control queue. No UI, library scan, filesystem operation, logging sink, or allocation is allowed to block the audio output loop.

## Provisional tasks

Exact priority, stack size, core affinity, and period are **not locked** until profiling.

| Task | Responsibility | Input | Output | Queues/events | Timing sensitivity | Blocking restrictions |
|---|---|---|---|---|---|---|
| AudioOutputTask | keep I2S DMA supplied; apply final volume/ramp/mix policy; count underruns | PCM ring; playback state | DMA buffers; diagnostics | state/format events | Hardest application deadline | no filesystem, display, logging flush, or unbounded wait |
| DecoderTask | decode MP3 frames or parse/copy WAV PCM; emit format/error/EOF | compressed ring; track commands | PCM ring; format events | command queue + ring waits | High; must stay ahead of consumption | no UI; bounded waits only; avoid steady-state allocation |
| StorageTask | mount/enumerate/read SD; prefetch sequential bytes; recover card faults | open/read/cancel requests; card detect | compressed ring; storage status | request and event queues | Medium-high; latency is buffered | never runs in audio task; read chunks are bounded |
| InputTask | debounce five buttons and jack detect | GPIO/ISR notifications | input events | event queue | Low latency, tiny work | ISR only timestamps/notifies; no business logic |
| UITask | render boot/library/playback/error/diagnostic screens | immutable UI model/events | display bus updates | coalesced UI queue | Soft real-time | capped refresh rate; no storage scans |
| PowerTask | read gauge/charger state; enforce low-battery policy | I2C/status GPIO | power events/UI data | event queue | Low | bounded I2C timeout; never gates audio directly in ISR |
| DiagnosticsTask | snapshot counters/watermarks and print on debug channel | atomic counters/snapshots | logs/engineering screen | timer/event | Low | no expensive sampling in high-priority context |

## Buffering model

```text
SD read chunks -> compressed ring -> decoder frame -> PCM ring -> I2S DMA descriptors
```

Initial compile-time placeholders are 32 KiB compressed and 32 KiB PCM capacity, preferably in PSRAM for bulk storage while DMA buffers and queue structures remain internal. These are hypotheses, not final sizes.

Sizing procedure:

1. measure maximum PCM byte rate among V1 formats (48 kHz × 2 channels × 2 bytes = 192,000 B/s);
2. measure p95 and maximum SD read latency using several cards and file fragmentation states;
3. measure p95 and maximum MP3 frame decode time;
4. include DMA duration and task-scheduling jitter;
5. choose a reserve with documented margin and repeat under UI/SD/error stress;
6. compare internal-memory versus PSRAM ring access and ensure DMA-capable buffers stay internal;
7. expose current fill, minimum watermark, high watermark, overflow, and underrun counts.

## Audio format contract

Canonical PCM is signed 16-bit interleaved stereo. Mono decoder output is duplicated to L/R in the adaptation stage. Speaker mono uses `((int32_t)L + (int32_t)R) / 2` before saturation, never 16-bit addition. Format metadata includes sample rate, channels, and bits per sample. Unsupported combinations fail explicitly.

At a sample-rate change: ramp down/mute, stop and drain I2S, configure the clock, prefill, restart DMA, and ramp up. Arbitrary software resampling is not a V1 requirement.

## Events and ownership

- `AppStateStore` (future implementation) is the sole writer of application state.
- Producers send typed `AppEvent` values through a bounded queue.
- Playback commands are distinct from resulting state events.
- Track ownership is represented by a stable ID/path owned by the library component; UI holds a copied view model, not live filesystem objects.
- Stop/skip increments a stream generation so stale decoder/storage data is discarded deterministically.

## Diagnostics contract

The scaffold exposes fields for reset reason, SD bytes/throughput/worst latency, decoder frames/time/failures, compressed and PCM buffer levels/minimums, underruns, current sample rate, free/minimum heap, PSRAM use, per-task stack watermarks, battery percentage/voltage, and charger state. Unknown values remain explicitly unavailable; the UI must not invent them.

## Fault behavior

- SD absent/removal: cancel stream, mute, close/unmount where possible, remain responsive, show factual error, and allow remount.
- corrupt/unsupported file: stop only that stream, report typed error, permit skip/back.
- underrun: count it, output silence or controlled mute for the missing segment, and continue/recover according to severity.
- unexpected reset: report reset reason; never claim resume correctness until tested.
