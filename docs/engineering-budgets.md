# Expanded-scope engineering budgets

2026-10-01. Planning limits/calculated scenarios, not device measurements.
Bluetooth/display choices are open. Reject a final design that violates these
budgets; do not use a larger battery to conceal uncontrolled load.

## Memory / scheduling

| Consumer | Current/planned allocation | Placement / acceptance |
| --- | --- | --- |
| Encoded SPSC ring | 32 KiB + sentinel | Current static internal memory |
| Stereo PCM ring | 64 KiB + frame sentinel | Current static internal memory |
| Read/staging | 8 KiB | Current static internal memory |
| Lyrics | 256 × 164 bytes ≈ 41 KiB | Explicit PSRAM UiAssets on S3; host heap in tests |
| Browser + playback queue | ≈32 KiB each | Browser static internal; queue explicit PSRAM; keeps playback independent of browsing |
| Helix state/output | Verify dependency/link map + runtime allocation | Not measured; reserve 40 KiB envelope until profiled |
| Worker/UI stacks | 4+8+4+6 KiB, plus console/input/IDF | Actual minimum high-water marks required, ≥25% spare target |
| New TFT framebuffer | 112.5–131.25 KiB + 15 KiB DMA tiles | Explicit PSRAM framebuffer; internal DMA tiles |
| New full-card index/cache | ≤256 KiB cache in PSRAM, SD-backed index | NOT IMPLEMENTED; current folder limit is not a scalable library |
| Settings | 788-byte versioned record + bounded strings | Commit after quiet interval/checkpoint, no per-frame flash writes |
| BT | Dedicated module allowance or measured ESP32 stack/SBC envelope | Architecture-dependent; do not assume S3 BLE supplies A2DP |

S3's nominal 512 KiB SRAM is not all application heap. Initial new-feature static
allocations exceeded 200 KiB; lyrics/queue were moved to an explicit ~73 KiB
PSRAM allocation at initialization. Failure disables UI playback safely, leaving
console diagnostics available. PSRAM is configured for
capability allocation, **not automatic malloc fallback**. A successful link is
not enough: measure free/largest internal block, minimum heap, stacks and cache
pressure while all tasks run. Preserve ≥64 KiB free internal headroom as an
initial acceptance target, then justify it from measured worst allocations.

Executed link-size report at 50df90c: DIRAM used 204,658 bytes of 341,760;
link-time remainder 137,102 bytes; .bss 144,736 bytes; image 426,724 bytes.
[Firmware build/size report](https://github.com/Hustlenix/Nightwave/actions/runs/36882011549).
These are compiler/linker figures, **not runtime heap measurements**. Runtime
decoder/stacks/filesystem/driver allocations still consume that headroom.

Current priorities: audio 8, decode 6, storage 5, UI 3. CPU targets (not results):
audio work <25% of a 256-frame deadline (5.33 ms at 48 kHz), decode worst compute
<50% of represented frame time, combined audio/storage/decode <70% one-core
equivalent. UI parse/scan ≤10 ms slices is a future requirement, not currently
proven by synchronous bounded loaders. Instrument task runtime on board.

## Latency / media clock

PCM capacity: 341.33 ms at 48 kHz, 371.52 ms at 44.1 kHz. Startup prefill 100 ms
unless short-file EOF; sink DMA 8×256 frames adds up to 42.67/46.44 ms accepted
lead. Position counts only media frames accepted by I2S, not underrun zeros.
Paused clock freezes after fade/drain. Lyric lookup follows this clock at 5 Hz;
current OLED updates can add up to 200 ms plus bus latency. Acoustic lyric
alignment has not passed acceptance. Subtract measured queued output/BT delay,
calibrate user offset and target ≤100 ms alignment before claiming synced
physical playback. Do not use an independent wall-clock timer.

SD worst read target <20 ms per 4 KiB; alarm >50 ms. Decoder timing currently
covers instrumented decode/framing calls, not every WAV copy/wait; label it
accordingly. PCM low-water/underruns distinguish starvation from output errors.
MP3 seek/resume decodes/discards from start to preserve reservoir, capped at
30 minutes and cancellable. It may be slow; a seek index is still required for
fast long-track resume. WAV seeks directly on frame alignment.

## Storage / bounded documents

Direct user-owned MP3/WAV; .lrc sidecar with same stem. LRC ≤64 KiB, physical
line ≤511 bytes, ≤256 retained timestamps, ≤16 tags/line, text ≤159 UTF-8 bytes;
timestamps sorted stably, positive/negative offset bounded ±600 s, duplicate
time uses last line. Invalid/oversized documents clear lyric state without
stopping music. ID3v2.3/2.4 text metadata ≤64 KiB tag/256 frames, UTF-8/UTF-16/
Latin-1; ID3v1 fallback; WAV INFO and exact PCM duration. Unsupported tag flags
are not interpreted. MP3 duration is unknown until a trustworthy scan/index.

M3U/M3U8 ≤64 KiB/128 local paths, each ≤255 bytes, no schemes/absolute/root
escapes. FAT has no symlinks; path validation is lexical, not a general desktop
filesystem sandbox. Missing tracks become recoverable errors, not infinite
auto-skip. Browser retains ≤128 entries and examines ≤512 per scan; the limit
is explicit. Whole-card songs/artists/albums, incremental indexing, cache
invalidation and paged large-library navigation remain open work.

## Power / eight-hour requirement

Run `tools/power_budget.py`. Preserve 85% regulator/80% amp efficiency, 80%
usable capacity, 80% aging and 20% load margin. TFT scenarios replace the old
5 mA display with an assumed 60/100 mA, not a measured panel current. BT 50 mA
cell-side is an allowance. Do not add it to 3.3 V load and count conversion twice.

| Scenario | Cell-equivalent W | Required Ah for 8 h | 6.6 Ah model hours |
| --- | ---: | ---: | ---: |
| Old OLED / speaker 0.25 W | 1.4168 | 5.744 | 9.19 |
| TFT 60 mA / speaker 0.25 W / BT off | 1.6303 | 6.609 | 7.99 |
| TFT 60 mA / speaker 0.50 W / BT off | 1.9428 | 7.876 | 6.70 |
| TFT 100 mA / speaker 0.25 W / BT off | 1.7856 | 7.239 | 7.29 |
| TFT 60 mA / BT output / speaker muted | 1.5028 | 6.093 | 8.67 |
| TFT 60 mA / speaker + BT scanning | 1.8153 | 7.359 | 7.17 |

The 6600 mAh pack no longer has a defensible blanket eight-hour claim. Define
speaker RMS level, backlight timeout/dimming and BT power-off policy; profile
first. Existing 69×54×18 mm/155 g candidate already dominates pocket volume
and weight. Evaluate lighter documented high-current packs and lower loads,
not an automatically larger cell. Charge current/timer/source current/NTC and
thermal checks in power-options.md remain mandatory. No lithium connection or
charge-while-play testing before reviewed power hardware and bench gate.

## Completion status

Implemented: bounded lyric/metadata/M3U loaders, accepted-sample clock, modes,
sleep fade/stop/display sleep, versioned resume/settings, provisional OLED
views and machine-readable diagnostics. Pending: chosen BT transport, TFT UI,
full-card indexed library, MP3 duration/fast seek, fully separated hardware HAL,
fuel gauge/charging/low-battery policy and physical timing/power acceptance.
This is an honest software checkpoint, not a completed production/funding build.
