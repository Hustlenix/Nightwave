# Audio buffer analysis

Status: implemented defaults, awaiting hardware measurement.

## Current WAV/MP3 path (2026-10-01)

```text
FatFS / SDMMC read (StorageTask, priority 5, 4096-byte chunks)
    -> 32768-byte SPSC encoded ring
    -> DecoderTask (priority 6, WAV conversion or Helix MP3)
    -> SPSC PCM ring (16,384 stereo frames)
    -> AudioOutputTask (priority 8, ramped Q15 volume, 256-frame writes)
    -> ESP-IDF I2S standard driver (8 DMA descriptors x 256 frames)
```

The file-reading task never writes I2S. The audio task never performs filesystem calls. The single-producer/single-consumer ring uses acquire/release atomics and performs no steady-state allocation.

## Implemented capacity

The PCM ring stores 16,384 usable stereo frames. Each frame is two signed 16-bit samples, so it consumes 65,536 bytes plus object metadata.

| Source rate | PCM ring duration | 256-frame write duration | 8 x 256 DMA-frame envelope |
|---:|---:|---:|---:|
| 22,050 Hz | 743 ms | 11.61 ms | 92.88 ms |
| 32,000 Hz | 512 ms | 8.00 ms | 64.00 ms |
| 44,100 Hz | 371 ms | 5.80 ms | 46.44 ms |
| 48,000 Hz | 341 ms | 5.33 ms | 42.67 ms |

These are arithmetic capacities, not measured tolerance. The player pre-fills
to 100 ms before configuring/enabling output, unless a short source has already
finished decoding. Empty-ring events before decoder completion increment the
underrun counter; live status and session-exit logs expose it.

## Compressed-data stage

The 2026-10-01 funding request supersedes the old physical-before-MP3 gate.
The active byte ring is 32 KiB, with a separate 4096-byte decoder staging area
for contiguous frame/refill input. Capacities remain conservative starting
allocations, not measurement-tuned final requirements.

For illustration only, a 32 KiB compressed ring holds about 2.05 seconds at 128 kbit/s or 0.82 seconds at 320 kbit/s. These figures are capacity calculations, not observed performance.

## Measurement-driven tuning

At the bench gate, capture:

- SD benchmark average KiB/s, worst individual 32 KiB read time, and read errors;
- queue minimum/maximum depth during each fixture;
- underrun count;
- task stack high-water marks;
- free and minimum heap;
- MP3 median, p95, and maximum frame decode time after integration.

The current 16,384-frame ring and task priorities are safe starting values, not final values. Reduce memory or change priority/core affinity only after the worst-latency tests show margin.

