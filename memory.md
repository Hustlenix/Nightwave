# memory.md

# Nightwave — Persistent Project Context

## Project summary

Nightwave is a standalone offline physical music player for the Pixl music-player Trial. It plays the user's own files from microSD through a built-in speaker or 3.5 mm stereo headphones, with physical controls and a rechargeable battery.

The project targets Pixl T4 Nexus depth, but T4 is not guaranteed.

## Current objective

Complete the digital preparation for Phase 2, then stop at the physical prototype gate. Do not order a final PCB, connect a lithium battery, fabricate measurements, or infer breakout pinouts.

## Finalized product decisions

- Offline only for playback.
- No phone/app/cloud/streaming dependency.
- microSD is the primary music storage.
- Dedicated physical controls: previous, play/pause, next, volume-, volume+.
- Built-in speaker.
- 3.5 mm wired stereo headphone output.
- Rechargeable 1-cell lithium battery.
- OLED-class display.
- >=8 h measured "full-night" speaker playback acceptance requirement.
- Custom PCB.
- Editable custom enclosure CAD + STEP.
- Physical demo video.
- Complete reproducible public repository.

## Finalized architecture decisions

- ESP32-S3-class MCU.
- ESP-IDF C/C++.
- Direct filesystem access; no DFPlayer.
- Software MP3 decode.
- WAV PCM playback.
- compressed-data buffering + PCM buffering.
- FreeRTOS task pipeline.
- I²S + DMA output.
- separate headphone and speaker signal paths.
- external stereo DAC for headphone path.
- headphone amplifier after DAC.
- I²S Class-D amplifier for speaker.
- charger with power-path capability.
- battery fuel gauge.
- custom mixed-signal board.

## Candidate parts — NOT yet locked

- PCM5102A stereo DAC.
- TPA6132A2 or equivalent headphone amplifier.
- MAX98360C provisional final I²S speaker amplifier; MAX98357A documented prototype proxy.
- BQ24074/BQ25185-class charger/power-path IC.
- MAX17048-class fuel gauge.

Before lock: verify datasheet, package, footprint, availability, assembly capability, cost, and electrical suitability.

## Core firmware pipeline

```text
microSD
 -> SDMMC/FatFS
 -> StorageTask
 -> compressed ring buffer
 -> DecoderTask
 -> PCM ring buffer
 -> optional DSP/volume
 -> AudioOutputTask
 -> I²S DMA
 -> headphone DAC/amp + speaker amp
```

## Major firmware components

- app_state
- storage
- library
- decoder
- audio_pipeline
- audio_i2s
- playback
- input
- ui
- power
- settings
- diagnostics

## Physical human gates

ChatGPT Work stops and asks the builder for:
- purchasing;
- breadboard wiring;
- soldering;
- multimeter/oscilloscope measurements;
- battery handling;
- fabrication payment/order;
- enclosure print/fit;
- physical playback listening;
- runtime test;
- build photos;
- final demo;
- second-human design sanity check.

## Core T4 evidence

Do not substitute feature bloat for these:

- direct SD filesystem;
- software decode;
- real-time buffered PCM pipeline;
- DMA/I²S;
- task/queue architecture;
- underrun profiling;
- custom audio electronics;
- custom power subsystem;
- custom PCB;
- complete CAD assembly;
- failure handling;
- measured SD/audio/memory/current/runtime performance;
- reproducible repository.

## Pixl integrity rules

- Current Pixl rules override this package.
- AI use must be disclosed.
- The submitted design must be genuinely understood and materially authored/reviewed by the builder.
- Do not submit fabricated measurements, journals, photos, hours, or test results.
- Autonomous AI work is not to be represented as human engineering time.
- If physical bodges are made, update source files to match.

## Repository target

```text
README.md
docs/
firmware/
hardware/BOM.csv
hardware/kicad/
hardware/manufacturing/
enclosure/
renders/
measurements/
media/
```

## Design direction

Compact intentional consumer device:
- understated retro-digital;
- high-contrast OLED;
- dedicated tactile controls;
- solid screwed enclosure;
- useful custom silkscreen;
- no copied PocketPlayer branding/art;
- no gimmicks that compromise engineering.

## Non-goals

- Bluetooth.
- Wi-Fi streaming.
- companion app.
- cloud accounts.
- Spotify/YouTube.
- touch screen.
- feature padding before core engineering is stable.

## Definition of "full night"

Acceptance test is >=8 hours continuous playback using the built-in speaker at a declared fixed volume with normal display policy.

Engineering target is >=10 hours speaker playback and >=12 hours headphone playback, but only measured results may be claimed.

## Current implementation status

Phase 0, the digital portion of Phase 1, and the digital preparation for Phase 2 were completed on 2026-09-29/30.

- The live Pixl project 1200 and current public Trial, hardware, ship, energy, and first-project pages were checked. The Trial is a physical local-file music player; Nightwave's eight-hour acceptance target is an internal requirement, not quoted Pixl wording.
- Hackatime configuration and the installed editor extension were detected without exposing or changing the API key. Attribution to the new repository still requires a genuine human editor heartbeat and Pixl-link verification.
- The public GitHub repository is `https://github.com/Hustlenix/Nightwave`; `main` has meaningful milestone commits and its ESP-IDF 6.1 CI build passes.
- Phase 1 artifacts now include source-linked research, a provisional component matrix, pin budget, power tree, task/buffer architecture, decoder study, preliminary BOM, risks, and prototype plan.
- The selected final-board direction is ESP32-S3-WROOM-1-N16R8, 1-bit SDMMC, PCM5102A plus TPA6132A2 for headphones, MAX98360C for the speaker, BQ25185 power-path charging, TPS63802 3.3 V buck-boost regulation, and MAX17048 fuel gauging. These selections remain provisional until schematic review and bench validation.
- A compile-oriented ESP-IDF interface scaffold exists; it deliberately starts no hardware drivers and claims no playback behavior.
- Phase 2 now has an exact prototype BOM, supplier alternatives, machine-readable pin-by-pin wiring, voltage-domain/current-planning tables, deterministic WAV generator, MP3 conversion recipe, and staged bring-up checklist.
- Repository checks cross-validate the prototype wiring against firmware GPIO constants. This caught and corrected swapped playback and volume button constants before physical wiring.
- The priced portion of the prototype BOM is USD 67.60. It excludes the unavailable/unpriced TPA6132A2EVM2, builder-supplied microSD/headphones/test equipment, tax, and shipping.

No physical prototype, purchased final parts, measured runtime, finished schematic/PCB, enclosure, demo, submission, funding approval, or Pixl tier is claimed.

## Next recommended action

HUMAN GATE before board-specific Phase 3 firmware:
1. make a genuine human edit in the tracked editor, verify the resulting Hackatime project, and link it to Pixl project 1200;
2. review/approve the Phase 1 architecture and confirm understanding of direct SD, software decoding, buffering, I²S, the separate headphone/speaker paths, and charger/fuel-gauge roles;
3. obtain the exact parts in `hardware/prototype-BOM.csv` or report substitutions before wiring;
4. return clear photos of both sides of every module, the DevKitC revision, microSD and headphone models, power-source/test-equipment details, and unavailable parts;
5. only after wiring review, follow `docs/prototype-bringup.md` without a lithium battery and return logs/measurements.

## Session-start instruction for ChatGPT Work

At the start of every session:

1. Read `PRD.md`, `Architecture.md`, `rules.md`, `phases.md`, `design.md`, and this file.
2. Inspect the repository and recent commits.
3. Identify the current phase.
4. Continue the current phase without re-planning finalized architecture.
5. Stop only at a genuine HUMAN GATE, a safety issue, or a rule conflict.
6. Update this file when durable state changes.
