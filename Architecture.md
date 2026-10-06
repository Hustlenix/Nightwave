# Architecture.md

## Current implementation override - 2026-10-06

The separately disclosed AI-authored [reference PCB](hardware/kicad/nightwave-reference/README.md)
uses the selected S3/BM83, Waveshare 24382 and TCA9535 architecture.
Its [numbered GPIO map](hardware/kicad/nightwave-reference/FINAL_GPIO_MAP.md)
is checked against the [reference firmware profile](docs/reference-board-firmware.md).
TFT and expander button transports are implemented but not physically qualified;
Bluetooth still has an unavailable runtime backend. Power monitoring now has
a read-only shared-I2C adapter; charging control/shutdown remain unqualified.
The reference circuit uses BQ25628E, dual TPS63802, TPS22965, MAX17048,
PCM5102A, TPA6132A2 and MAX98357A. Earlier BQ24074/BQ25185 charger options
and old direct-button/OLED maps below are historical alternatives, not its BOM.
See [remaining completion gates](docs/design-completion-status.md). No pack,
fabrication approval or physical runtime is locked by this architecture.

# Nightwave — Technical Architecture

## 2026-10-01 final execution scope override

Local software decoding remains mandatory. New scope includes synced .lrc,
metadata/M3U, playback modes/resume/sleep and Bluetooth audio output. S3 remains
the working target, not an A2DP-capable chip. Choose docs/bluetooth-architecture.md
and docs/display-selection.md before final schematic work. On 2026-10-02 the
builder selected S3 plus BM83SM1-00TA (BD-14). On 2026-10-03 the builder selected
Waveshare 24382 non-touch 240x280 ST7789V2 (BD-15). BD-16 selects TCA9535PWR
input expansion; its combined numbered map and reviewed power hardware/pack
remain open; docs/current-power-model.md is
the current calculated screening model, not physical runtime evidence.
docs/engineering-budgets.md distinguishes
implemented software from remaining library performance, HAL/power and transport
work. Older two-output/OLED references below are historical provisional design.

2026-10-02: `library` now includes LibraryCatalog, with fixed-size SD records,
bounded directory work queue, idle incremental build, per-record integrity/path
checks and PSRAM 16-row pages. PlayerFrontend supplies songs/artist/album views,
full collection/filter playback and cached context resume. No indexing/metadata
work runs on the real-time audio workers. A v2 footer supplies bounded PSRAM
artist/album dictionaries and grouped track IDs; filtered selection uses direct
record access, with legacy/allocation/group-cap slow fallback. Queries/footer
warmup are UI-sliced; SD contention and fallback latency remain acceptance gaps.
See docs/library-index.md for memory budgets, wire format and resume checks.

## 1. Architecture objective

Runtime diagnostics now run from the serialized UI/console owner, using
boot-lifetime accepted-media and fault counters from the audio engine. Reports
distinguish wall time, sampled playback and qualified battery observations;
current unavailable power/untested audio keeps battery proof unestablished.
No logging, allocation or filesystem work is added to the audio worker. See
docs/runtime-test.md for sampling, counter-wrap, gap and physical-evidence limits.

Nightwave is not an ESP32 remote control for a self-contained MP3 module. The ESP32-S3 owns the storage, decode, buffering, playback state, UI, and power policy.

Current product output branches (only one active route):

```text
microSD -> reader -> encoded ring -> Helix/WAV -> PCM ring -> route/gain policy
  +-> I2S -> MAX98360C -> speaker                         [local candidate]
  +-> I2S -> PCM5102A -> TPA6132A2 -> stereo jack        [local candidate]
  +-> rate-qualified I2S -> BM83SM1-00TA -> A2DP sink    [builder-selected; backend pending]
```

BM83 does SBC/radio transport only; S3 retains filesystem/software decoding.
AT Tx supports 44.1/48 kHz. Existing 22.05/32 kHz local playback remains valid,
but Bluetooth at those rates requires a tested converter or explicit rejection.
No implicit rate change, AVRCP Tx support or speaker fallback on BT loss.
Source-mode control and full-text display/power HAL boundaries are portable;
hardware adapters, final GPIO, power rails and acoustic timing remain unqualified.

Implementation boundary, 2026-10-03: `StreamingPlayer` currently owns the local
`I2sAudioSink` and its muted/speaker/line routes. `BluetoothSource` is a separate
host-tested control/PCM contract; `app_main` instantiates `UnavailableBluetooth`.
The three-output diagram specifies the product, not an integrated three-output
implementation. Combined routing, actual UART events/provisioning and rate-correct
PCM transport still require implementation and qualification. Invalid PCM now
faults closed, disconnects and invalidates stale events; this does not enable radio.

Historical two-local-output bench pipeline (not the complete product architecture):

```text
microSD
  │
  ▼
SDMMC + FatFS
  │
  ▼
Storage / reader task
  │ compressed-byte ring buffer
  ▼
MP3/WAV decoder task
  │ PCM frames
  ▼
PCM ring buffer
  │
  ├── optional digital volume / EQ / limiter
  ▼
Audio output task
  │ I²S DMA
  ├──────────────────────┐
  ▼                      ▼
PCM5102A               MAX98360C
stereo DAC             mono Class-D
  │                      │
  ▼                      ▼
headphone amp          speaker
  │
  ▼
3.5 mm stereo jack
```

This architecture is deliberately chosen to expose the systems problems that a dedicated MP3 module hides.

## 2. Chosen stack

### Firmware

**ESP-IDF, C/C++, FreeRTOS**

Rationale:
- native access to ESP32-S3 I²S and DMA;
- native SDMMC/FatFS integration;
- FreeRTOS task primitives;
- detailed memory/task/driver control;
- suitable for profiling and embedded systems work.

Arduino wrappers MAY be used only for isolated libraries if necessary and justified. The main architecture MUST remain ESP-IDF-native.

### EDA

**KiCad**

Deliverables:
- `.kicad_pro`
- `.kicad_sch`
- `.kicad_pcb`
- footprint/model libraries used by project
- Gerber/drill export

### Mechanical CAD

Preferred:
**FreeCAD** with:
- `.FCStd`
- `.STEP`
- `.STL`

A parametric alternative such as Onshape is acceptable only if source remains editable and exportable.

### Rendering

Optional:
- KiCad 3D viewer;
- pcb2blender;
- Blender `.blend`.

Rendering is documentation, not a substitute for CAD source.

## 3. Repository structure

```text
nightwave/
├── README.md
├── LICENSE
├── docs/
│   ├── requirements.md
│   ├── architecture-notes.md
│   ├── decisions.md
│   ├── power-budget.md
│   ├── prototype-wiring.md
│   ├── schematic-review.md
│   ├── pcb-review.md
│   ├── bringup.md
│   ├── validation.md
│   ├── runtime-test.md
│   └── ai-disclosure.md
├── firmware/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   ├── main/
│   ├── components/
│   └── test/
├── hardware/
│   ├── BOM.csv
│   ├── kicad/
│   ├── libraries/
│   ├── manufacturing/
│   └── datasheets/
├── enclosure/
│   ├── nightwave.FCStd
│   ├── nightwave.step
│   ├── nightwave.stl
│   └── drawings/
├── renders/
├── measurements/
│   ├── current-draw.csv
│   ├── runtime.csv
│   └── notes.md
└── media/
    ├── build/
    └── final/
```

## 4. Major firmware modules

### app_state
Owns global product state:
- booting;
- ready;
- playing;
- paused;
- error;
- charging;
- low-battery shutdown.

No arbitrary module may mutate global state directly.

### storage
Responsibilities:
- mount/unmount microSD;
- enumerate folders/files;
- provide sequential reads;
- report throughput/errors;
- detect card changes when hardware allows.

### library
Responsibilities:
- filter supported media;
- parse/display file names;
- metadata extraction if implemented;
- stable track identifiers;
- navigation.

### decoder
Responsibilities:
- consume compressed bytes;
- identify format;
- decode MP3;
- decode WAV PCM;
- output normalized PCM frame representation;
- report decode errors/end-of-stream.

Candidate lightweight libraries should be researched and license-checked before lock. Avoid large framework adoption that bypasses the core architecture.

### audio_pipeline
Responsibilities:
- PCM ring buffer;
- sample-format normalization;
- volume ramp;
- mono mix for speaker;
- optional DSP;
- underrun/overflow counters.

### audio_i2s
Responsibilities:
- configure I²S;
- configure DMA buffers;
- stream PCM;
- start/stop cleanly;
- expose playback diagnostics.

### playback
Responsibilities:
- play/pause;
- next/previous;
- end-of-track;
- seek if implemented;
- queue;
- repeat/shuffle if implemented.

### input
Responsibilities:
- button GPIO;
- debounce;
- press/release events;
- long press only where explicitly defined.

### ui
Responsibilities:
- display rendering;
- screen transitions;
- error state;
- battery status;
- no blocking storage/audio operations.

### power
Responsibilities:
- fuel gauge;
- charger status;
- low-battery behavior;
- idle/display sleep policy;
- controlled shutdown.

### settings
Responsibilities:
- NVS-backed persistent volume;
- last track/path;
- preferences;
- schema/version handling.

### diagnostics
Responsibilities:
- structured logging;
- task high-water marks;
- underrun count;
- SD throughput;
- heap/PSRAM use;
- reset reason.

## 5. FreeRTOS task architecture

Initial tasks:

| Task | Priority intent | Primary responsibility |
|---|---|---|
| AudioOutputTask | highest application priority | keep DMA supplied |
| DecoderTask | high | compressed → PCM |
| StorageTask | medium-high | prefetch compressed data |
| InputTask | medium | button events |
| UITask | medium-low | screen refresh |
| PowerTask | low | SOC/charging/thermal policy |
| DiagnosticsTask | low | periodic metrics in debug builds |

Exact priorities/core affinity MUST be measured and tuned, not guessed permanently.

## 6. Buffering architecture

Two-stage buffering:

```text
SD card
  │
  ▼
compressed ring buffer
  │
  ▼
decoder
  │
  ▼
PCM ring buffer
  │
  ▼
I²S DMA
```

Why two buffers:
- isolates SD latency from decode;
- isolates decode-time variance from real-time audio output;
- makes underruns measurable;
- permits controlled prefetch.

Work MUST benchmark:
- SD sequential throughput;
- worst observed read latency;
- decode throughput;
- PCM consumption rate;
- heap/PSRAM budget.

Buffer sizes MUST be derived from measurements and recorded in `docs/architecture-notes.md`.

## 7. Memory architecture

Baseline:
- internal SRAM for latency-critical queues/driver structures;
- PSRAM, if the selected ESP32-S3 module includes it, for larger media buffers/library cache;
- NVS for settings only;
- microSD for music and optional cache/index.

Do not make playback depend on a huge in-RAM library database.

## 8. Storage architecture

Preferred final interface:
- SDMMC rather than SPI when pin budget/layout permit.

Reasons:
- higher bandwidth headroom;
- stronger systems depth;
- avoids unnecessary contention with display/peripherals.

Fallback:
- SDSPI if board constraints materially justify it.

Filesystem:
- FAT32/exFAT only if supported and licensing/toolchain implications are acceptable;
- FAT32 baseline is sufficient for V1.

## 9. Audio data model

Internal canonical PCM representation should be documented, e.g.:

```c
struct pcm_frame {
    int16_t left;
    int16_t right;
};
```

or packed buffers equivalent to 16-bit stereo PCM.

Decoder output that differs in sample rate/channel count MUST pass through an explicit format-adaptation stage.

## 10. Sample-rate strategy

V1 should support the common music rates used by the selected decoder library.

Preferred strategy:
- reconfigure I²S clock between tracks when source rate changes;
- avoid implementing arbitrary software resampling unless needed.

The transition MUST mute/ramp to avoid pops.

## 11. DSP architecture

Required:
- digital volume;
- saturation-safe stereo-to-mono mix.

Optional after stable core:
- parametric or multi-band EQ;
- limiter;
- balance.

DSP MUST not jeopardize buffer deadlines.

## 12. Audio hardware architecture

### Headphone path

```text
ESP32-S3 I²S
   │
   ▼
PCM5102A
   │ stereo analog
   ▼
headphone amplifier
   │
   ▼
3.5 mm TRS jack
```

PCM5102A is a candidate because it accepts I²S, provides stereo output, and does not require MCLK in the common configuration. Final schematic MUST follow the authoritative datasheet/reference design.

The DAC line output MUST NOT be assumed safe for arbitrary low-impedance headphones without the headphone amplifier stage.

### Speaker path

```text
ESP32-S3 I²S
   │
   ▼
MAX98360C-class Class-D
   │ differential speaker output
   ▼
4–8 ohm speaker
```

The speaker amplifier can consume the stereo stream and use a documented mono/channel selection mode.

The speaker path MUST support hardware/software shutdown for power savings and headphone insertion behavior.

### Headphone detection

Use a jack with a reliable detect/switch contact or a dedicated detector.

Event:
- headphone inserted → mute/shutdown speaker, enable headphone path;
- headphone removed → ramp/mute transition then restore speaker if user policy allows.

## 13. Control hardware

Five dedicated buttons:
- PREV;
- PLAY/PAUSE;
- NEXT;
- VOL-;
- VOL+.

Buttons:
- must be debounced in software;
- must not float;
- should use documented pull-up/pull-down strategy;
- must have ergonomic spacing in CAD.

## 14. Display hardware

Final product: Waveshare 24382 non-touch 240x280 SPI display selected in BD-15. The small
monochrome OLED is a legacy bench adapter, not a final default.

SelectedDisplayProfile defines portrait 240x280 geometry, 20-row GRAM offset,
RGB565 and bounded 16-row tiles (7,680 bytes). Maker example register choices
are recorded, not a functioning ESP-IDF driver. GPIO, initialization, PWM/sleep,
full lyric wrapping/glyphs and physical panel tests remain unfinished. Do not
run the historical OLED adapter against the TFT or infer a reviewed final map.

Selection criteria:
- low current;
- strong library support;
- readable outdoors/indoors;
- easy mechanical integration;
- availability.

SPI DMA tiles and backlight power need bounded budgets. I2C remains available
for the fuel gauge/control devices; check capacitance and pull-ups separately.

## 15. Power architecture

High-level:

```text
USB-C 5V
  │
  ├─ CC resistors / protection
  ▼
1-cell charger with power path
  │
  ├────────► VSYS
  │
  └────────► LiPo
               │
               └─ fuel gauge → I²C → ESP32-S3

VSYS
  ├─ qualified regulator → ESP32/DAC/readable display/logic
  ├─ speaker amp supply or validated derived rail
  └─ BM83 qualified rail + shutdown/backfeed isolation
```

Candidate charger:
- BQ24074-class or BQ25185-class power-path charger.

Candidate fuel gauge:
- MAX17048-class gauge.

The final choice MUST be based on:
- battery chemistry;
- charge current;
- thermal limits;
- package assembly;
- availability;
- protection requirements.

## 16. USB-C architecture

USB-C is used for power/charging in V1.

MUST:
- correctly advertise sink behavior using CC resistors if using USB-C receptacle;
- include ESD/protection as appropriate;
- avoid claiming USB data functionality unless implemented;
- keep connector mechanically supported.

Optional later:
- native USB firmware update or mass storage if electrical design supports data lines.

## 17. Battery sizing

Do not choose capacity from guesswork.

Process:

1. Prototype current measurement for:
   - idle;
   - headphone playback;
   - speaker playback at multiple volume levels;
   - display on/off.
2. Set >=8 h acceptance target and >=10 h engineering target for speaker profile.
3. Include regulator/charger losses and reasonable capacity derating.
4. Choose protected cell whose dimensions fit CAD.
5. Verify thermal and charge-current compatibility.
6. Validate with full discharge test.

## 18. PCB architecture

The final board is a real mixed-signal PCB, not a breakout-module motherboard.

Functional zones:

```text
┌─────────────────────────────────────┐
│ USB/charger/power     speaker amp   │
│                                     │
│ battery/fuel gauge                  │
│                                     │
│ ESP32-S3 + SD         I²S DAC       │
│                       headphone amp │
│ display/buttons       audio jack    │
└─────────────────────────────────────┘
```

Layout principles:
- respect ESP32 antenna keepout if using module with PCB antenna;
- short decoupling paths;
- controlled high-current Class-D return path;
- keep switching power/Class-D nodes away from low-level analog path;
- continuous reference planes where appropriate;
- avoid splitting return paths blindly;
- provide test points;
- clearly mark polarity/connectors;
- verify connector edge placement against CAD before fab.

## 19. Schematic hierarchy

Sheets/blocks:

- `POWER`
- `MCU`
- `STORAGE`
- `DISPLAY_INPUT`
- `AUDIO_DIGITAL`
- `AUDIO_ANALOG`
- `CONNECTORS_TEST`

Every rail and bus must be named consistently.

## 20. Mechanical architecture

Parametric enclosure with:
- base;
- lid;
- PCB standoffs;
- threaded inserts or screw bosses;
- battery pocket/cradle;
- speaker mount;
- acoustic grille;
- display window;
- button plungers/caps;
- USB-C opening;
- headphone opening;
- optional SD service opening;
- strain/clearance management.

PCB edge/connector placement and enclosure must be co-designed, not designed sequentially in isolation after PCB freeze.

## 21. Physical prototype architecture

Before final custom PCB:

```text
ESP32-S3 dev board
+ microSD breakout
+ I²S DAC breakout
+ headphone amp breakout
+ I²S speaker amp breakout
+ legacy bench OLED (final BD-15 screen is separate)
+ buttons
+ bench/USB power
```

Purpose:
- validate firmware architecture;
- measure current;
- validate headphone/speaker UX;
- expose software issues before fabrication.

The prototype is NOT the final submitted hardware.

## 22. Bring-up architecture

Final PCB bring-up sequence:

1. visual inspection;
2. continuity/short tests;
3. current-limited USB/bench power without battery when possible;
4. validate 3.3 V and VSYS;
5. validate MCU programming;
6. validate I²C devices;
7. validate SD;
8. validate DAC clocks/output;
9. validate headphone amp;
10. validate speaker amp;
11. validate buttons/display;
12. validate charger;
13. validate battery/fuel gauge;
14. full integrated playback.

Battery insertion comes only after power subsystem validation.

## 23. Testing strategy

### Firmware tests
- unit tests for file filtering;
- WAV parser;
- playback state transitions;
- settings validation;
- ring-buffer behavior where practical.

### Hardware validation
- rail voltage;
- current consumption;
- SD throughput;
- I²S clocks;
- headphone left/right;
- speaker output;
- jack detection;
- charging behavior;
- battery SOC;
- runtime.

### System tests
- 2+ hour continuous playback;
- full-night runtime;
- corrupted files;
- SD removal;
- repeated track changes;
- low-battery shutdown;
- headphones hot-plug;
- charge while playing.

## 24. Observability

Debug build MUST expose:

- boot/reset reason;
- current state;
- heap free/minimum;
- PSRAM use;
- stack high-water marks;
- SD throughput;
- decoder failures;
- compressed-buffer level;
- PCM-buffer level;
- audio underruns;
- output sample rate;
- battery SOC/voltage;
- charger state.

Release UI does not need to show all diagnostics.

## 25. CI

GitHub Actions SHOULD run:
- firmware format/lint if configured;
- ESP-IDF build;
- host-side unit tests where possible;
- file presence validation for release artifacts.

CI MUST NOT pretend to validate physical electronics.

## 26. Security/privacy

No user accounts and no network services.

Firmware MUST not transmit telemetry.

If USB data is later enabled, expose only intended interfaces.

## 27. Important tradeoffs

### Why not DFPlayer?
It hides filesystem/audio decode/buffering and removes much of the system depth.

### Why ESP32-S3?
It provides sufficient compute, I²S, DMA, FreeRTOS, SD support, and optional PSRAM while still fitting a handheld embedded device.

### Why separate DAC and speaker amp?
The headphone and speaker loads have different requirements and deserve explicit signal paths.

### Why a power-path charger?
The player must behave predictably while charging and playing.

### Why a custom PCB?
The goal is a complete physical product and a deeper hardware build, not a permanent breadboard/module stack.

## 28. Architectural freeze rule

The following are architecture-level invariants after Phase 2:

- ESP32-S3-class MCU;
- direct microSD access;
- software decode;
- ring-buffered PCM pipeline;
- DMA-driven I²S;
- separate headphone, speaker and Bluetooth source output paths;
- BM83SM1-00TA with source-capable AT firmware (builder-selected BD-14);
- larger readable display selected through BD-15, not locked to OLED;
- rechargeable 1-cell battery;
- fuel gauge;
- custom PCB;
- editable custom enclosure.

Changing one requires updating PRD.md, Architecture.md, phases.md, rules.md, design.md, and memory.md where affected.
