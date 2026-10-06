# memory.md

# Nightwave — Persistent Project Context

## Project summary

Nightwave is a standalone offline physical music player for the Pixl music-player Trial. Current scope is microSD MP3/WAV, speaker, wired stereo and Bluetooth output, synchronized LRC, rich library, readable display, physical controls and rechargeable battery.

The project targets Pixl T4 Nexus depth, but T4 is not guaranteed.

## Current objective

2026-10-06 continuation (supersedes historical checkpoint details below):
reference GPIO profile, TCA9535 button runtime with recovery, ST7789 tiled TFT
renderer/ESP-IDF SPI adapter and separate legacy bench profile implemented.
Typed active-symbol ERC and four explicit supply declarations replace the
all-passive model. Local native ERC/DRC/unconnected/parity counts are zero;
39 Python tests, project/package/pin validators and focused host tests pass.
Source add96254341f786c73a4e9cfb6795bf37f1b4b58 was committed and pushed.
Quality CI 37441295569 passed all 15 Release and all 15 ASAN/leak suites plus
39 Python tests and project/reference validators. Firmware CI 37441295524
passed both reference and legacy bench ESP32-S3 profiles. Native ERC negative
controls caught deliberately conflicting output drivers and missing battery
supply declarations. Older green CI is not proof for later source changes. See
docs/design-completion-status.md for remaining power, procurement, Bluetooth,
mechanical, review and eligibility gates. No fabrication approval or physical
performance is claimed. Starter project/user changes remain preserved.

Published 2026-10-03 verified software source: `cb3db92af2a889a90981650fe004e3e03244d0a7`.
Project quality37127240521 verified successful: all13 Release/all13 ASAN with
leak detection, 28 Python tests, project validator/calculations. New source
includes selected TFT geometry, compiled TCA9535 input adapter, malformed BT PCM
fail-closed fix, current power screening and bounded Trial evidence inventory.
Firmware37127240508 verified completed successfully for that exact source.
The evidence-ledger follow-up changes documentation only; firmware/tests/tools
remain identical to this validated checkpoint. Actual Trial audit
returns BLOCKED, 41 missing findings, measured runtime null and physical_pass=false.
Independent agent/source reviews do not satisfy independent human hardware review.
Follow-up verification fixes funding inventory's stale status allowlist for
BD-15/16 and retained S3: selections are not falsely reported pending, while
missing prices, final total, power review and sources remain real blockers.
New regression raises Python suite count to28 (10funding/12Trial/6power), locally
and in CI passing. Firmware/C++ sources remain identical to a7edc5f.
Actual funding inventory is BLOCKED with34 findings, priced core USD53.16 and
no final total. Selected TCA9535 has no verified price; it is not counted as zero.
The baseline/history below is retained and superseded by this current entry.

2026-10-03 builder handoff: `hardware/kicad/nightwave-starter` now contains an
AI-assisted KiCad 10 project skeleton with a root hierarchy, six empty subsystem
sheets, a blank PCB and explicit per-sheet TODO notes. KiCad CLI loads and plots
all seven pages and ERC reports zero violations because no electrical circuit is
present yet. This starter does not resolve the final GPIO map, charger/pack/BT
rail, exact support parts, footprints, placement or routing; it is not a finished
schematic or funding-ready design. The builder must materially author and review
the actual design, returning one subsystem at a time for review.

2026-10-03 finished-device continuation: preserve the complete frozen systems
scope, not arbitrary feature-count growth. Main was fast-forwarded to the latest
published runtime checkpoint `8bb9232`; live verification found firmware
37021176284 and quality37021176476 successful (11 Release +11 ASAN/leak suites,
nine synthetic funding inventory tests). New malformed-PCM Bluetooth fault-state
coverage and a separate bounded Trial evidence inventory are added next; their
new-source CI must be verified independently. Runtime is unknown and no physical
test/demo/final schematic/CAD exists. The pending evidence record must never be
filled with generated or invented human observations.

Bluetooth's combined audio router, actual radio adapter and low-rate conversion
are genuine software gaps, not merely unmeasured hardware. BD-14 remains S3 plus
BM83SM1-00TA; BD-15 was explicitly selected by the builder on 2026-10-03 as
Waveshare 24382 non-touch 240x280 ST7789V2. This dated entry supersedes all
older pending-BD-15 notes below. Host-tested geometry/tile contract is not a
working SPI/glyph driver. Builder explicitly selected BD-16 TCA9535PWR input
expansion on 2026-10-03. 25 retained usable S3 GPIO versus 31 minimum direct
product signals; eight slow inputs + one direct IRQ save seven. Address/port/IRQ
and combined numbered map remain unreviewed. Portable all-input driver and
finite-timeout ESP-IDF shared-bus adapter are now digitally implemented with
fault/readback/partial-read/recovery tests; the old ButtonMonitor still uses
bench GPIO, so final button/gesture integration and physical qualification remain open.
The current reproducible screening is tools/product_power_budget.py and
docs/current-power-model.md: continuously lit typical speaker 1.7468 W,
8 h requirement 7.0816 Ah under stated margins, hypothetical 6.6 Ah 7.4559 h.
These are calculated, not measured. BQ25185's 4.5 V SYS cannot directly feed
the BM83 operating rail. Charger/source/thermal, isolated BT rail and pack review
remain open. Core captured-price subtotal is now USD53.16 including selected
display; battery/support/BT rail/fab/delivery excluded. No battery, final pin map
or schematic-entry lock is inferred. Stale final-SH1106 bench text and funding evidence/decision wording
are corrected; prototype prices and core quantities remain dated snapshots.
The builder's #pixl-help clarification question is drafted, not sent or approved.

2026-10-02 Trial-completion continuation implements sampled runtime diagnostics
and boot-lifetime accepted-media/fault counters. The current power/audio adapters
remain unqualified, so the recorder cannot establish physical battery runtime.
docs/trial-acceptance.md maps all Trial requirements and the full ten-system scope;
uploaded animation remains a proposal. Firmware/tests must be validated for this
new checkpoint before describing it as a successful software build.
Local strict C++17 runtime regression executable and the existing project
validator pass. A simulated eight-hour timeline is explicitly synthetic, not
an elapsed device test. Full firmware/host CI for the published source is required.

2026-10-02 shipping continuation adds tools/check_shipping_readiness.py and
synthetic regression tests. Development validation remains separate from
funding inventory: pending costs/core subtotals and empty CAD/PCB directories
cannot establish readiness. Even a complete inventory requires human review;
physical acceptance and tier remain unestablished. The new animation idea is
not implemented or counted as completed work. Journals remain builder-authored.
Local verification: nine synthetic inventory tests and the existing project
consistency validator pass. The current inventory remains BLOCKED: 18 components,
11 priced rows, USD43.67 captured subtotal, no final total and no design sources.
Concept-site text now reflects selected BM83, pending display/power hardware,
simulated UI values and T4 target rather than an awarded tier.

2026-10-02 authenticated Project 1200 cleanup: repository/status/AI notes were
audited against actual source. Three original journal claims totalled 23.4h,
but the supplied builder discussion attributed hour values to text length;
no reliable replacement personal durations were supplied. Journal prose stays
builder-authored; AI does not add retrospective sessions or infer human time.
See docs/project-1200-audit.md for current form requirements and separate funding
versus final-build gates. Display BD-15 and builder schematic/PCB/CAD remain
pending. The README seek/shuffle contradiction and old login-gate notes are
corrected. No physical success or funding submission is claimed.

Latest 2026-10-01 final master/addendum supersedes older two-output scope.
Bluetooth A2DP SOURCE and readable display must be chosen by builder before
final schematic tasks. Do not silently switch MCU or preserve old OLED. Current
software adds bounded lyrics/metadata/M3U, modes, seek/resume, sleep and checked
settings. The 2026-10-02 continuation implements an SD-backed paged catalog and
bounded PSRAM lookup, MP3 duration/sparse seek, engineering reports and portable
BT/power/display interfaces. Real SD-latency acceptance, BM83 source adapter,
BD-15 TFT driver and reviewed physical power hardware remain open.
See docs/engineering-budgets.md for exact limits and current evidence ledger.

2026-10-02 builder reply selected BD-14: keep ESP32-S3 plus BM83SM1-00TA.
This is the builder's actual selection, not proof of AT provisioning/interop.
Personal decision reasons were not supplied and are not invented. BD-15 is
pending. Battery, combined pin map and schematic-entry gate remain unlocked.
The v2 library checkpoint was pushed as df301661256671e3db892fd3cd6450fb65588c4f:
Firmware 36994661327 and quality 36994661341 succeeded, 8/8 Release plus 8/8
ASAN/leak. Link DIRAM 204882/341760, .bss 144960, image 445484 bytes.
These are link figures, not board measurements. Older checkpoint notes below
are dated history, not the current hardware choice or software evidence.

Follow-on source checkpoint: bounded <=12 KiB MP3 frame scanner, <=8500-byte
identity/checksum cache, 128-frame checkpoint spacing and preceding-group
decoder preroll; malformed/stale/over-cap cache uses slow fallback. Real Helix
15-second indexed seek on original 30-second media matches fallback PCM hash/
sample count with fewer decode calls/SD bytes. Local optimized Release seek,
streaming and HAL tests passed; seek/frontend ASAN/leak passed. Full Release,
ASAN/leak and device CI for this new source is recorded only after execution.
HAL includes 8-device BT state/epoch/timeouts/backpressure, 256-byte BM83 framing
(not AT source commands), unknown-safe power and borrowed full-text display
frames. Console numbers/lines and engineering JSON are bounded. Old OLED/353
pack are removed from active product BOM; prototype BOM remains historical.
Selected BM83, pending TFT/pack/GPIO strategy and critical BOM engineering fields
are explicit in hardware/product-config.json and hardware/BOM.csv.

Digital source 8c7015e has full successful CI: device 37002164509, quality
37002164501; 10/10 Release + 10/10 ASAN/leak, validator/arithmetic pass. Link
DIRAM 205394/341760, remainder136366, .bss145456, image453300 bytes. No runtime
heap/physical measurements. Preceding b28228c device failure was a uint32_t
initializer-list portability mismatch, repaired without weakening checks.
Public Microchip Turnkey1.2.0 command-set2.08 includes AT commands/status;
reviewed packet codec added next. Factory TA2.07 compatibility, actual AT image,
tool/licence/provisioning, connect/EIR/ACK and PCM rate conversion remain open.
Do not substitute newer 00TB or count packet gating as AVDTP START/SUSPEND.

Final firmware source ee2f4fc was pushed and validated: firmware37003131904
success, quality37003131911 success (10 Release +10 ASAN/leak suites, project
validator/arithmetic). Includes documented fixed AT codec/status bounds and
1000 cycles, not a live radio backend. Subsequent diagnostics-reader/docs-only
checkpoint preserves this firmware/C++ source; reader accepts checked engineering
reports without treating any record as physical acceptance. BD-15 remains
pending, so current full power/charging/pack/GPIO review and Builder Task1 are
not complete or authorized to start. No final schematic authored.

Final pushed diagnostics-reader checkpoint3316ff5: quality37003778650 success,
10/10 Release +10/10 ASAN/leak, project/engineering/parser checks. Firmware and
C++ tests are identical to validated ee2f4fc (device37003131904). This evidence-
only continuation changes no software. Worktree clean after commit/push must
be checked, not assumed. Next required user input is BD-15; recommend non-touch
Waveshare24382, alternativeAdafruit3787. Neither selected. Only after selection
propagation, full updated power/charging/battery analysis and BOM/GPIO review
may the first precise builder-authored schematic task be handed off.

Complete the digital software/design preparation for Pixl funding under the
2026-10-01 request. The old physical-before-digital gate is superseded; physical
validation remains pending. Do not order hardware, connect lithium, invent
measurements or human authorship, or call an incomplete package funding-ready.
Current Pixl rules require an original builder-owned hardware design, not fully
AI-generated hardware files, and independent human sanity checking.

## Funding-stage checkpoint — 2026-10-01

- 2026-10-02 continuation: idle-sliced LibraryCatalog with 10,000-track cap,
  songs/artists/albums pages, full indexed/filter playback, backup cache fallback,
  root/checksum validation and folder/indexed-filter resume. Source validation
  evidence is recorded only after execution in docs/software-validation.md.
  Validated source 14b10c0: Firmware run 36962788706 success; quality run
  36962788677 success, 8/8 Release + 8/8 ASAN/leak. Link DIRAM 204882/341760,
  .bss 144960, image 437340 bytes (not runtime measurements). Failed Release
  path warning and noncanonical fixture assertion were repaired; evidence kept.
  V2 lookup continuation: 227488-byte explicit PSRAM generation, active/staging
  peak 454976; 1024 artist/album keys each, 10000 tracks. Group pages need no
  catalog record reads, filtered page ≤16, selection/correct resume hint one;
  path/tag checked and stale hint searches. Legacy v1 is preserved/readable;
  footer/allocation/group-cap failure falls back to slow scans without dropping
  tracks. Host 10000-file tests are metadata-only, not physical audio proof.
  Source df30166 now has its own successful CI evidence above.
  BD-14 is selected as recorded above; BD-15 remains pending. No final builder files created.

- FINAL-MASTER CHECKPOINT: 50df90c firmware/size reports passed (run 36882011549);
  92c17ea quality passed (run 36882687260), 7/7 Release + 7/7 ASAN/leak suites.
  Local WSL frontend/media ASAN suites passed too. Firmware unchanged after 50df90c.
- Added bounded LRC/metadata/M3U, sample clock/WAV/MP3 seek, modes/resume/sleep,
  checked NVS state, output preference, JSON diagnostics/parser. Lyrics/queue
  use explicit S3 PSRAM; no physical alignment/playback success claimed.
- Historical 2026-10-01 gate: BD-14 and BD-15 were both pending then. BD-14 is
  now selected; BD-15 remains the current choice gate. Old MCU-first advice is
  superseded. Full-card index has explicit lookup/fallback limits. New duration,
  sparse seek and HAL work requires separately recorded checkpoint evidence.
- Expanded model: assumed TFT 60mA/speaker 0.25W needs 6.609Ah for 8h; old 6.6Ah
  candidate models only 7.99h. No pack/charger/runtime is locked or measured.

- RESUMED IN MENTOR/REVIEWER MODE: user reports direct Pixl clarification that
  AI tutoring/review is permitted with real builder engineering decisions and
  authoring. This is user-reported, not independently verified correspondence.
  Research/software/checklists continue while editable human sources are pending.
- Do not generate the final schematic, PCB routing or enclosure wholesale.
  Use docs/builder-tasks.md and docs/builder-decisions.md; leave personal choices
  and reasons pending until supplied. Do not inspect Hackatime secrets.
- Mentor packet now covers 15 electrical subsystems and PCB/CAD constraints;
  docs/builder-tasks.md has 17 staged tasks, starting with the MCU block.
- Mentor packet CI at ea393ac / run 36874323047 succeeded: project checks,
  engineering-calculation tests, 6/6 Release and 6/6 AddressSanitizer/leak suites.
  https://github.com/Hustlenix/Nightwave/actions/runs/36874323047
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
- Waveshare 24382 non-touch readable TFT (BD-15 selected); old OLED is bench-only.
- TCA9535PWR input expansion (BD-16 selected); combined port/IRQ/pin map unreviewed.
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
- high-contrast readable lyric display;
- dedicated tactile controls;
- solid screwed enclosure;
- useful custom silkscreen;
- no copied PocketPlayer branding/art;
- no gimmicks that compromise engineering.

## Non-goals

- Phone/cloud dependency (Bluetooth A2DP source output is current scope).
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

Historical gate, superseded by the later digital-continuation request: the
earlier Phase 4 request required physical Phase 3 evidence before entry. Audit
found no device logs, build photos, or audio measurement rows. Phase 4 has not
started then. MP3 was not implemented at that historical checkpoint; it is
implemented and digitally tested now. Physical WAV/MP3 proof remains pending.

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
# 2026-10-04 — AI-authored KiCad reference checkpoint

- Generated `hardware/kicad/nightwave-reference/` separately from the live
  beginner starter, preserving the user's KiCad-created settings/history.
- Reference package covers the selected ESP32-S3, BM83SM1-00TA, Waveshare
  24382 and TCA9535PWR architecture plus USB-C, BQ25628E power path, dual
  TPS63802 rails, PCM5102A/TPA6132A2/MAX98357A audio, microSD, buttons,
  connectors and test points.
- KiCad CLI parses and exports the seven-page schematic, four-layer placed
  board, PDF, BOM, manifest, renders, STEP, Gerbers, drill and position files.
- This is not fabrication-ready: the board is unrouted and the captured ERC,
  DRC and schematic-parity reports remain failing. Independent lithium, power,
  RF, footprint and layout review is mandatory before ordering or connecting a
  battery. No physical measurements, runtime or demo evidence exists yet.

## 2026-10-05 - Routed AI reference checkpoint

This supersedes the unrouted state recorded immediately above, not the
unfinished electrical/product qualification gates.

- Completed routing closure of the 132-component reference and added three
  board-only M2 non-plated mounting holes (135 total footprints), copper
  exclusions, ground/logic planes and functional silkscreen.
- Corrected charger/regulator land patterns and logical pin maps, BM83
  BAT_IN/ADAP_IN/VDD_IO treatment and ground lands 56/57, amplifier EP,
  SD pulls, isolated headphone detect and averaged-stereo mode divider.
- Preserved RF antenna keepouts. BM83 body keepout has narrowly scoped windows
  at recommended ground lands, not relaxed antenna restrictions. Via-in-pad
  solder/paste treatment remains an assembly review item.
- Widened 209 power trace sections; remaining necks and switching loops still
  require current/thermal/layout review. Geometry closure is not a safety claim.
- Separated J2/J3/J6/J7 PCB header MPNs from the off-board pack/NTC/speaker/TFT;
  PH header current rating is 2 A with specified wiring, not the pack's 3 A.
  Generic passives, several connectors/harness parts and complete pricing remain
  pending: no complete procurement BOM or updated qualified power model exists.
- Project validator, reference semantic checks and all 34 Python tests passed.
  See reference reports for final native ERC/DRC/parity and source hashes.
  Pushed engineering commit `90974a381d566704c60d4a95520b1424880c3434`
  passed Project quality CI `37344278247` (Release host tests and ASAN/leak
  checks included) and ESP32-S3 Firmware CI `37344278327`. These builds do not
  qualify the reference-board drivers or prove physical operation.
- Rebuilt editable KiCad, PDF, Gerber/drill/placement and PCB STEP review
  package. The STEP is not enclosure CAD and lacks some component 3D bodies.
- Not fabrication-ready or grant-approved: USB source current/protection and
  impedance, power loops/current capacity, complete sourcing, final enclosure
  fit and firmware reconciliation are unresolved. No physical evidence or
  human authorship is invented. Starter user edits/history are preserved.

