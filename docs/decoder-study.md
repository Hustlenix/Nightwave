# MP3 Decoder and WAV Study

Checked: 2026-09-29

## MP3 candidates

| Library | License | API/model | Streaming | Output | Maintenance / ESP32 evidence | Integration risk | Decision |
|---|---|---|---|---|---|---|---|
| [chmorgan/esp-libhelix-mp3](https://github.com/chmorgan/esp-libhelix-mp3) | Apache-2.0 wrapper/component; bundled dependency notices must be preserved | `MP3FindSyncWord`, `MP3Decode`, frame info | Yes; caller owns byte buffer/refill | integer PCM | ESP Component Registry package `1.0.3`; explicit ESP32 component and usage notes | ID3/sync recovery is caller responsibility; confirm ESP-IDF 6.1 build | PROVISIONAL SELECT |
| [lieff/minimp3](https://github.com/lieff/minimp3) | CC0-1.0 | single-header frame decode; optional extended API | Yes; `minimp3_ex.h` provides streaming/seeking | s16 or optional float | widely used, small surface; no official ESP-IDF packaging | SIMD/configuration and embedded benchmarks required | BACKUP |
| [mackron/dr_mp3](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) | public domain or MIT-0 | single-header callback/file/memory API; low-level push API | Yes | s16 or float | maintained dr_libs code; based on minimp3 | larger single header and internal allocation behavior need review | BACKUP |
| libmad | GPL family | fixed-point decode | Yes | fixed-point PCM | mature but not selected | license is a poor fit for this repository; older integration path | REJECT |

The selected library decodes frames only. Nightwave continues to own FAT access, track/file state, compressed and PCM ring buffers, task boundaries, format adaptation, volume/mix, I2S, DMA, error handling, and diagnostics.

## Required MP3 adapter behavior

1. `StorageTask` opens a file and fills the compressed ring buffer.
2. The decoder adapter recognizes and skips ID3v2 data rather than feeding arbitrary metadata as frames.
3. It searches for a valid sync word, decodes one frame, and advances only by the library-reported byte consumption.
4. If a frame spans the current contiguous segment, the adapter requests/refills enough data without blocking `AudioOutputTask`.
5. Format changes are emitted as explicit events carrying sample rate and channel count.
6. Unsupported/free-format/corrupt data yields a typed error and bounded resynchronization attempt; it never loops forever.
7. Decoder timing, failures, and input/output counts feed diagnostics.

Provisional acceptance benchmarks for the prototype are intentionally not pass/fail numbers yet. Measure median, p95, and maximum frame decode time for representative 128/192/320 kbps files and verify sustained real-time decode with margin.

## WAV V1 subset

Nightwave V1 will transparently support:

- RIFF little-endian `WAVE` containers;
- `fmt ` chunk using format code 1 (linear PCM);
- 16-bit samples;
- one or two channels;
- 22,050, 32,000, 44,100, and 48,000 Hz initially;
- extra/unknown chunks skipped by declared chunk length with even-byte padding;
- `data` chunks whose declared length stays inside the file.

The parser must reject, with a visible typed error:

- RIFX/big-endian files;
- compressed WAV formats, IEEE float, extensible format unless deliberately added later;
- unsupported bit depths/channel counts/rates;
- block-align/byte-rate fields inconsistent with PCM geometry;
- chunks or data lengths that overflow or exceed file bounds;
- truncated headers/samples.

No resampler is planned for Phase 1. Between tracks, playback drains/mutes, reconfigures I2S to the supported source rate, pre-fills the PCM buffer, and ramps up.

## Phase 1 outcome

Only interfaces and error taxonomy are scaffolded. No decoder library is vendored and no file is claimed to decode yet. Dependency integration starts after the exact prototype board/config is confirmed.
