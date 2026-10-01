# memory.md

# Nightwave — Persistent Project Context

## Project summary

Nightwave is a standalone offline physical music player for the Pixl music-player Trial. It plays the user's own files from microSD through a built-in speaker or 3.5 mm stereo headphones, with physical controls and a rechargeable battery.

The project targets Pixl T4 Nexus depth, but T4 is not guaranteed.

## Current objective

Complete the digital software/design preparation for Pixl funding under the
2026-10-01 request. The old physical-before-digital gate is superseded; physical
validation remains pending. Do not order hardware, connect lithium, invent
measurements or human authorship, or call an incomplete package funding-ready.
Current Pixl rules require an original builder-owned hardware design, not fully
AI-generated hardware files, and independent human sanity checking.

## Funding-stage checkpoint — 2026-10-01

- RESUMED IN MENTOR/REVIEWER MODE: user reports direct Pixl clarification that
  AI tutoring/review is permitted with real builder engineering decisions and
  authoring. This is user-reported, not independently verified correspondence.
  Research/software/checklists continue while editable human sources are pending.
- Do not generate the final schematic, PCB routing or enclosure wholesale.
  Use docs/builder-tasks.md and docs/builder-decisions.md; leave personal choices
  and reasons pending until supplied. Do not inspect Hackatime secrets.
- Mentor packet now covers 15 electrical subsystems and PCB/CAD constraints;
  docs/builder-tasks.md has 17 staged tasks, starting with the MCU block.
- New tools/engineering_calculations.py screens recharge/source power,
  feedback divider and headphone gain. Local self-tests/project validator pass.
  No screening calculation is actual charging or hearing-safety validation.
- Exact MAX98360CEFB+T is 10-pin FC2QFN (CENL+T is 9-ball WLP); use corrected
  manufacturer OUTP/OUTN pin map. MAX17048 VDD is battery sense. Jack audio
  switches cannot be assumed an isolated digital detector. Builder choices open.

- DIGITAL DESIGN: INCOMPLETE (software implemented; physical hardware sources absent).
- PHYSICAL BUILD: NOT VERIFIED / no returned prototype measurements.
- FUNDING READINESS: NOT READY; exact checklist in docs/funding-readiness.md.
- Software now uses StorageTask -> 32 KiB encoded ring -> DecoderTask ->
  16384-frame PCM ring -> AudioOutputTask. Helix wrapper pinned 1.0.3; upstream
  RPSL/RCSL terms preserved. WAV/MP3, cancellation, pause/resume, gain ramps,
  rate change between tracks, telemetry, OLED/browser/buttons and NVS volume
  are present, with explicit limitations in docs/player-controls.md.
- 88adbaa firmware + four host suites passed. Expanded corruption/frontend tests
  passed at 13ed6be Project quality run 36864360149; firmware run 36864360208
  passed too. No host result is a physical measurement.
- 6600 mAh Adafruit product 353 is a provisional model candidate, not locked
  hardware. Updated priced-core subtotal $55.97 is NOT a funding total.
- Power review found BQ25185 Rev. B six-hour charge-timer conflict and candidate
  peak-load/source-current/NTC issues; power design is not final.
- PCB revision: NONE. CAD revision: NONE. Manufacturing archive: NONE.
- Human builder hardware provenance: pending. Independent sanity check: pending.
- Project 1200 browser reached login gate; no fields edited or funding submitted.
- Latest complete device build: a2aa741 / run 36865138056, success.
- Latest expanded test evidence: 6349cd0 / run 36865602237, six suites passed in
  Release and six passed with AddressSanitizer + leak detection. Firmware source
  is unchanged since the successful device build. Worktree changes are committed
  and pushed; no physical outputs or measurements have been inferred.

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

Phase 0, the digital portion of Phase 1, the digital preparation for Phase 2, and the first Phase 3 firmware implementation were completed on 2026-09-29/30.

- The live Pixl project 1200 and current public Trial, hardware, ship, energy, and first-project pages were checked. The Trial is a physical local-file music player; Nightwave's eight-hour acceptance target is an internal requirement, not quoted Pixl wording.
- Hackatime configuration and the installed editor extension were detected without exposing or changing the API key. Attribution to the new repository still requires a genuine human editor heartbeat and Pixl-link verification.
- The public GitHub repository is `https://github.com/Hustlenix/Nightwave`; `main` has meaningful milestone commits and its ESP-IDF 6.1 CI build passes.
- Phase 1 artifacts now include source-linked research, a provisional component matrix, pin budget, power tree, task/buffer architecture, decoder study, preliminary BOM, risks, and prototype plan.
- The selected final-board direction is ESP32-S3-WROOM-1-N16R8, 1-bit SDMMC, PCM5102A plus TPA6132A2 for headphones, MAX98360C for the speaker, BQ25185 power-path charging, TPS63802 3.3 V buck-boost regulation, and MAX17048 fuel gauging. These selections remain provisional until schematic review and bench validation.
- The Phase 1 interface scaffold has been replaced by ESP-IDF components for safe-muted startup, chip/reset/heap/PSRAM diagnostics, debounced buttons, 1-bit SDMMC mount/enumeration/benchmarking, I2S DMA, a low-level ramped tone, robust WAV parsing, and two-task buffered WAV playback with underrun telemetry.
- Portable host tests cover generated WAV media, RIFF chunk/error parsing, ring behavior, volume, overflow-safe stereo-to-mono mixing, debounce, and playback state. The ESP-IDF build and host suite are CI gates.
- The PCM ring holds 16,384 usable stereo frames (341 ms at 48 kHz); that is an unmeasured starting value documented in `docs/audio-buffer-analysis.md`.
- The Adafruit 6309 TLV320DAC3100 breakout is the provisional bench headphone alternative because the TI TPA6132A2EVM2 remains unavailable for direct order. This does not change the provisional final PCM5102A + TPA6132A2 architecture.
- Phase 2 now has an exact prototype BOM, supplier alternatives, machine-readable pin-by-pin wiring, voltage-domain/current-planning tables, deterministic WAV generator, MP3 conversion recipe, and staged bring-up checklist.
- Repository checks cross-validate the prototype wiring against firmware GPIO constants. This caught and corrected swapped playback and volume button constants before physical wiring.
- The captured priced subtotal is USD 74.55 including the optional Adafruit 6309 headphone alternative. It excludes the unavailable/unpriced TPA6132A2EVM2, builder-supplied microSD/headphones/test equipment, tax, and shipping.

No physical prototype, purchased final parts, measured runtime, finished schematic/PCB, enclosure, demo, submission, funding approval, or Pixl tier is claimed.

## Next recommended action

### Phase 4 request audit, 2026-09-30

Phase 3 repair follow-up, 2026-10-01: RIFF bounds/padding, full-width WAV format
validation, duplicate chunks, and frame alignment are now checked. I2S preloads
all DMA buffers with silence before enable, auto-clears sent buffers, validates
stereo PCM blocks, and uses millisecond timeouts. Natural EOF flushes queued
audio; cancellation mutes before waiting for producer cleanup. Tone frequency
validation and output error reporting were added. New host parser/PCM regression
tests and an actual-adapter fake-driver test cover these paths. Local project
validation and whitespace checks passed; no local C++ toolchain is on PATH.
Device behavior, MP3, and physical Phase 3 exit criteria are still unverified.
Repair commit `4e19872` passed Firmware CI `36851574808` and Project quality
CI `36851574841`; both host-test executables passed (2/2). Build success and
fake-driver tests are not physical playback evidence.

The new Phase 4 request requires physical Phase 3 evidence before entry. Audit
found no device logs, build photos, or audio measurement rows. Phase 4 has not
started. MP3 remains unimplemented and awaits physical WAV evidence.

Independent Phase 3 preflight repairs move the 4 KiB read buffer off its 4 KiB
task stack, gate task startup until both workers exist, cancel the producer on
audio failure, and wait for producer cleanup before advertising idle. A stop
timeout retains live resources and refuses a new track. Short reads now cancel
with an error. Flash defaults match the documented N8R8 baseline (8 MB).

`docs/audio-validation.md` records the gate and exact evidence handoff. New
underrun and memory-stability CSVs contain headers only. Runtime concurrency,
audio correctness, stack adequacy, and stability are still physically unverified.

Preflight repair commit `ce48169` passed Firmware CI (run `36750836556`) and
Project quality CI (run `36750836808`, including portable host tests). Local
project validation and whitespace checks passed. No physical test was run.

HUMAN GATE for Phase 3 bench evidence:
1. make a genuine human edit in the tracked editor, verify the resulting Hackatime project, and link it to Pixl project 1200;
2. review/approve the Phase 1 architecture and confirm understanding of direct SD, software decoding, buffering, I²S, the separate headphone/speaker paths, and charger/fuel-gauge roles;
3. obtain the exact parts in `hardware/prototype-BOM.csv` or report substitutions before wiring; no purchase has been authorized by the repository work;
4. return clear photos of both sides of every module, the DevKitC revision, microSD and headphone models, power-source/test-equipment details, and unavailable parts;
5. only after wiring review, follow `docs/phase3-firmware-bringup.md` and `docs/prototype-bringup.md` without a lithium battery and return logs/measurements;
6. Physical WAV/MP3 evidence remains required for hardware validation, but does
   not block digital MP3 integration under the funding-readiness override.

## Session-start instruction for ChatGPT Work

At the start of every session:

1. Read `PRD.md`, `Architecture.md`, `rules.md`, `phases.md`, `design.md`, and this file.
2. Inspect the repository and recent commits.
3. Identify the current phase.
4. Continue the current phase without re-planning finalized architecture.
5. Stop only at a genuine HUMAN GATE, a safety issue, or a rule conflict.
6. Update this file when durable state changes.
