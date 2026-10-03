# Nightwave

Nightwave is an in-progress, standalone pocket music player for the Pixl “A Music player for the saloon” Trial. It reads user-owned MP3 and PCM/WAV files directly from microSD, with software decoding, physical controls, wired stereo headphones, a speaker, synchronized LRC and Bluetooth output. No phone or streaming service is required. The builder selected ESP32-S3 plus BM83SM1-00TA (BD-14) and Waveshare 24382 non-touch TFT (BD-15). Their hardware drivers, combined GPIO map and battery sizing are not qualified or locked.

> Status (2026-10-03): WAV/MP3 three-task streaming, bench OLED/five-button interaction,
> lyrics/metadata/M3U, modes, bounded seek/resume, sleep timer and versioned
> persistence are implemented for the provisional hardware. The 2026-10-02
> continuation adds an idle-built SD catalog, paged songs/artists/albums and
> bounded PSRAM lookups for filtered browsing/track changes, MP3 duration/sparse
> seek, full-text display interface, BT/power contracts and engineering telemetry;
> [catalog limits](docs/library-index.md) remain explicit. See the
> [expanded-scope checkpoint](docs/engineering-budgets.md) for remaining software.
> Executed [firmware/host CI evidence](docs/software-validation.md) is revision-specific.
> **Funding package is NOT READY:** final power design,
> complete BOM, builder-owned PCB/CAD, manufacturing files and human reviews are
> missing. No physical playback, runtime, fit, final hardware or tier is claimed.

## Why it exists

Hardware design proceeds in builder-authored mentor/reviewer mode. **Before
final schematic tasks**, review the selected [Bluetooth architecture](docs/bluetooth-architecture.md)
and [display BD-15](docs/display-selection.md). BD-16 selects
[TCA9535PWR input expansion](docs/input-expansion.md); review its [combined GPIO map](docs/pin-budget.md)
and review the [current power model](docs/current-power-model.md), charger/pack
and complete BOM before the first schematic task. Then use
[builder tasks](docs/builder-tasks.md), [subsystem requirements](docs/schematic-requirements.md)
and [your decision log](docs/builder-decisions.md). AI research and review are
disclosed; final schematic, routing and enclosure must be materially authored
by the builder. The reported Pixl clarification does not mark missing work done.

The product goal is to play the builder's own music offline without a phone,
notifications, subscription or hidden playback module. The builder's personal
motivation and genuine work journal must remain their own account; this README
does not invent a first-person build story.

## Architecture

```text
microSD / FAT
      |
      v
StorageTask -> compressed ring buffer -> DecoderTask -> PCM ring buffer
                                                        |
                                                        v
                                                AudioOutputTask
                                                        |
                                                   I2S + DMA
                                      +-----------------+-----------------+
                                      |                                   |
                                PCM5102A DAC                       MAX98360C amp
                                      |                                   |
                              TPA6132A2 headphone amp                mono speaker
                                      |
                               3.5 mm stereo jack
```

The ESP32 owns file enumeration, storage reads, MP3/WAV decoding, buffering, playback state, I2S output, buttons, UI, settings, diagnostics, and power policy. Nightwave does not use DFPlayer or another module that hides the storage/decode/playback pipeline. The diagram above is the current two-local-output software/bench path. The product requires a third mutually exclusive, rate-qualified I2S branch to BM83SM1-00TA for A2DP source output. Its radio adapter, combined playback router and 22.05/32 kHz conversion are not implemented. Host-tested Bluetooth control/framing contracts do not make that output operational.

The bench prototype uses an available MAX98357A breakout as a functional speaker-path proxy; the provisional final-board selection is MAX98360C.

## Current implementation

- repository and required engineering-document structure;
- current Pixl/Trial rules verification and honest AI-disclosure strategy;
- datasheet-backed provisional component comparison, BOM, pin budget, power tree, risk register, prototype plan, and decoder study;
- ESP-IDF 6.1 C++ firmware with safe-muted boot, chip/reset/heap/PSRAM diagnostics, debounced five-button logging, 1-bit SDMMC mount/enumeration/benchmarking, I2S DMA output, and a low-level tone test;
- robust host-tested WAV parsing for 16-bit mono/stereo PCM at 22.05/32/44.1/48 kHz, including unknown-chunk skipping and explicit errors;
- three-task WAV/MP3 streaming through a 32 KiB encoded ring and 16,384-frame
  SPSC PCM ring, pause/resume, cancellation, gain ramps and queue/error telemetry;
- SH1106 OLED driver (SSD1306 option), file/folder browser, now-playing, five
  buttons, headphone indicator, no-SD/error screens, sleep and diagnostics;
- bounded LRC, ID3v2.3/v2.4/ID3v1/WAV INFO metadata and local M3U/M3U8 loading;
- SD-backed incremental catalog, 16-row songs/artists/albums views, full indexed
  collection/filter queues, bounded artist/album lookup and checked ordinal-hint
  filter/folder resume; low-memory/over-cap caches use a slower fallback;
- normal/shuffle/repeat-all/repeat-track; accepted-sample lyric clock; WAV direct
  seek and idle-built sparse MP3 seek checkpoints with decoder preroll, safe
  decode-from-start fallback and a 30-minute resume/seek cap;
- versioned checked NVS volume/mode/output/resume/playlist/folder persistence,
  explicit saved-track resume and 15/30/45/60-minute/end-track sleep timers;
- JSON boot/stream/performance/self-test/engineering records, with unmeasured hardware marked;
- injected display frame interface carrying full bounded metadata/LRC strings;
  tested Bluetooth source-state/timeout/backpressure boundary and unknown-safe
  power HAL, without pretending unqualified adapters are operational;
- portable tests for WAV parsing, generated fixtures, ring wrap/full behavior, stereo-to-mono arithmetic, volume scaling, button debounce, and playback state;
- CI definitions for both ESP32-S3 firmware and portable host tests;
- exact Phase 2 prototype BOM, pin-by-pin wiring table, voltage/current plan, deterministic test-media generator, and staged bring-up checklist.

The firmware does **not** prove that a card mounts, a waveform reaches a physical
module, a speaker/headphone produces correct audio, or buffering survives real
card latency. OLED/controller wiring, jack detection and NVS behavior still need
bench validation. Battery reads, power-latch shutdown and automatic SD hotplug
reinitialization are not implemented. Retry/error handling is not a claim that
removing a real card during playback has been tested.

## Controls

The [Trial acceptance matrix](docs/trial-acceptance.md) maps every requirement to
source and missing physical proof. The [runtime recorder](docs/runtime-test.md)
logs sampled elapsed/playback windows and faults; it cannot certify battery life.

Previous/Next navigate the browser or change playing tracks. Play opens a folder
or starts/toggles playback. Volume +/- changes gain. Hold Play for the browser,
hold Previous for the parent folder, hold Volume + for diagnostics, hold Next
for lyrics and hold Volume - for mode/sleep/resume/output settings. First input
after the 30 s display sleep wakes without changing playback.
See [complete controls and limitations](docs/player-controls.md).

## Audio architecture

Helix decodes MP3 in software on ESP32-S3; WAV uses the validated PCM parser.
Storage, decode and I2S have separate tasks. PCM is stereo 16-bit at supported
22.05/32/44.1/48 kHz rates. ID3 skipping and resynchronization are bounded;
unsupported or corrupt media produces a recoverable error. The Class-D path
must select averaged-stereo mono in the reviewed final hardware; headphone
output uses the separate stereo DAC/amp path. Neither path is physically proven.

## Power architecture

USB-C and a protected 1S pack feed a charger/power path; SYS powers the speaker
and a buck-boost supplies 3.3 V. This direction is **provisional**, not a finished
circuit. The [selected-product screening model](docs/current-power-model.md)
calculates MCU, SD, selected TFT, speaker/wired/BT, idle, transients and charging.
Its continuously lit typical speaker case needs an estimated 7.08 Ah for eight
hours under the documented margins; a hypothetical 6.6 Ah gives 7.46 h, not a
passing measurement. Dimming/gating scenarios are conditional, not implemented
power savings. The [historical OLED model](docs/power-budget.md) is superseded.
[Charger timer, source-current and peak-load conflicts](docs/power-assumptions.md)
must be resolved before locking hardware. Estimated runtime is not measured
runtime, and charge-while-play safety is not verified.

## PCB and enclosure

There is no completed KiCad schematic/board, routed four-layer PCB, editable
enclosure, full STEP assembly or manufacturing archive yet. No dummy CAD files,
fake assembly photographs or render-only substitutes are presented as evidence.
An [AI-assisted KiCad starter](hardware/kicad/nightwave-starter/START_HERE.md)
now supplies a root hierarchy, six empty subsystem sheets and a blank board so
the builder can begin quickly. It is scaffolding only: it contains no finished
circuit, footprint assignment, GPIO map, placement or routing and is not funding
evidence by itself.
The board, battery, speaker, display, controls and ports must all have verified
mounting and clearances in the builder-owned design before funding submission.

## BOM

[Preliminary BOM CSV](hardware/BOM.csv): captured-price core subtotal **$53.16**,
including selected BM83 and Waveshare 24382, excluding the pending battery. This is
not a finished BOM total or funding/order cost. USB-C, support parts, PCB,
assembly, enclosure, delivery and taxes also remain excluded/pending. Critical
rows now include package, datasheet, source, footprint and electrical roles;
pending rows are explicit, not approved substitutes. The spreadsheet review
preserved the CSV and checked its selected/pending fields. Older prices remain
dated snapshots; the selected display was checked via official indexed content
2026-10-03 (direct fetch refused; stock/delivery unverified).
No parts have been ordered by this run.

## Build the firmware

Target toolchain: ESP-IDF `v6.1`, target `esp32s3`.

```sh
cd firmware
idf.py set-target esp32s3
idf.py build
```

The development baseline is `ESP32-S3-DevKitC-1-N8R8`; the provisional final module is `ESP32-S3-WROOM-1-N16R8`. Pins and component choices remain provisional until prototype validation.

Use `idf.py -p PORT flash monitor` after building, with the documented DevKit
only and verified wiring. The console adds `play`, `pause`, `resume`, and
`volume` to the bench commands; `wav` remains an alias. Follow
[Phase 3 firmware bring-up](docs/phase3-firmware-bringup.md) before enabling an
output. Production flashing/power instructions depend on the unfinished PCB.

## Load music and build the prototype

Copy supported `.wav` or `.mp3` files to a FAT-formatted microSD, optionally in
folders. Paths must be under 256 bytes; the browser displays at most 128 entries
per folder. No streaming or network is used. Generated test tones are described
in [test media](docs/test-tracks.md). Free-format/MPEG2.5 MP3, unsupported rates,
24-bit WAV and full Unicode display are not implemented features. Bounded WAV
and MP3 seek and shuffle are implemented; their limits and fallback behavior are
documented in [player controls](docs/player-controls.md) and [software evidence](docs/software-validation.md).

For the USB-powered human-assembled prototype, use the
[prototype BOM](hardware/prototype-BOM.csv), [wiring](docs/prototype-wiring.md)
and [bring-up checklist](docs/prototype-bringup.md). Do not connect a lithium
pack or place a final-PCB order using this preliminary package.

## Current validation and funding status

[Software evidence](docs/software-validation.md) separates actual CI runs from
host simulations and pending board measurements. The regression suite exercises
1000 start/cancel/control transitions, corrupt media, fragmented MP3, WAV,
concurrent ring transfers and real folder-navigation logic with fake devices.

[Funding audit](docs/funding-readiness.md),
[current official requirements](docs/pixl-funding-readiness.md),
[builder design/provenance review](docs/human-design-review.md), and
[independent sanity-check packet](docs/sanity-check.md) record the exact gaps.
Project 1200 was inspected in the authenticated builder session on 2026-10-02.
It is a hardware funding draft, with finished-build status unchecked. The Trial
requires local files, real speakers, physical playback/volume controls and a
full-night battery with the achieved runtime stated. Design funding and finished
build evidence are separate stages; see [the current project audit](docs/project-1200-audit.md).
No funding request has been submitted.

Run the [shipping artifact inventory](docs/shipping-audit.md) to expose unresolved
costs and missing design sources. A passing development validator or green form
checkbox does not establish a complete funding design or T4 eligibility.

## Finished-device acceptance

The [finished-device evidence inventory](docs/trial-acceptance.md#finished-device-evidence-inventory)
tracks own-file standalone speaker playback, physical play/pause/skip/volume,
actual full-night speaker battery runtime, final schematic/CAD/STEP, parts/build
instructions, build photos and a real demo video. The pending record is
[trial-evidence.json](measurements/trial-evidence.json); **achieved runtime is
unknown**, not an estimate or a simulated eight-hour result. No demo video exists
yet. Only populate evidence after the corresponding real work and human review.

```sh
python3 tools/check_trial_readiness.py
```

Exit 2 lists missing evidence. Exit 0 only inventories builder-reported evidence
for human review; it never certifies playback, safety, runtime, authorship or Pixl
acceptance. Keep the full [systems scope](docs/trial-acceptance.md) through release;
the hardware integrations must work, not just appear in the feature list.

## Engineering documents

- [Pixl requirements](docs/pixl-requirements.md)
- [Architecture notes](docs/architecture-notes.md)
- [Component comparison](docs/component-comparison.md)
- [Decoder study](docs/decoder-study.md)
- [Decision log](docs/decisions.md)
- [Pin budget](docs/pin-budget.md)
- [Power tree](docs/power-tree.md)
- [Prototype plan](docs/prototype-plan.md)
- [Prototype BOM](hardware/prototype-BOM.csv)
- [Prototype wiring](docs/prototype-wiring.md)
- [Prototype bring-up checklist](docs/prototype-bringup.md)
- [Deterministic test media](docs/test-tracks.md)
- [Phase 3 firmware bring-up](docs/phase3-firmware-bringup.md)
- [Audio buffer analysis](docs/audio-buffer-analysis.md)
- [Headphone prototype options](docs/headphone-prototype-options.md)
- [MP3 integration plan](docs/mp3-integration-plan.md)
- [Risk register](docs/risks.md)
- [Preliminary BOM](hardware/BOM.csv)
- [AI disclosure](docs/ai-disclosure.md)

## Physical validation still required

Direct-SD playback, decoder timing, buffer sizing, audio quality, channel correctness, output power, noise, thermal behavior, current draw, charge-while-play behavior, battery capacity, full-night runtime, PCB manufacturability, and enclosure fit all require physical evidence. The project-defined acceptance test is at least eight hours of continuous speaker playback under a declared profile; the current Pixl Trial itself says “a full night” and requires the achieved runtime to be stated, but does not publish an exact hour count.

## AI disclosure

AI assistance is being used for research synthesis, repository scaffolding, interface design, documentation, and software checks. The builder must understand and materially review the electrical and mechanical design, physically assemble and test the device, maintain honest journals and photos, and obtain a second-person sanity check before hardware submission. Fully AI-generated hardware design files are not acceptable under the current Pixl hardware rules.

## License

Nightwave's original repository content is provided under the MIT License. Third-party libraries keep their own licenses and notices.
The MP3 wrapper's Apache-2.0 license does not relicense the RealNetworks Helix
decoder. Read [dependency provenance and terms](docs/decoder-licenses.md);
notices are preserved in [licenses](licenses/). Original tone fixtures and the
small text font are generated within this repository, not downloaded media.
