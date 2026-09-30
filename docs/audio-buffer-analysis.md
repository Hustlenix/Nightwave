# Audio buffer analysis

Status: implemented defaults, awaiting hardware measurement.

## Current WAV path

```text
FatFS / SDMMC read (StorageTask, priority 5, 4096-byte chunks)
    -> format conversion and Q15 volume
    -> SPSC PCM ring (16,384 stereo frames)
    -> AudioOutputTask (priority 8, 256-frame writes)
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

These are arithmetic capacities, not measured tolerance. The player pre-fills up to the 100 ms low-water target before enabling the selected output. Every empty-ring event while the source is not complete increments and logs `AUDIO UNDERRUN`.

## Compressed-data stage

MP3 remains intentionally gated on physical WAV stability. The portable SPSC ring is type-generic and ready to instantiate as a byte ring between `StorageTask` and the future `DecoderTask`. That stage is not claimed active yet. The eventual minimum compressed capacity will be chosen from measured SD worst-read latency and MP3 bitrate, with margin for metadata and frame boundaries.

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

