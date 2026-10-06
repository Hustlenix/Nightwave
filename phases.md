# phases.md

## Current execution override - 2026-10-06

The user subsequently authorized implementation of a separate AI-authored PCB
reference; preserve the original builder starter and disclose the distinction.
BD-14/15/16 are selected, and the reference GPIO/firmware map exists. Do not
repeat those decisions as unanswered. Current work and exit evidence are in
[the completion ledger](docs/design-completion-status.md), which supersedes
historical workflow and completion claims below. Native checks do not approve
fabrication, funding eligibility or T4. Power/procurement, integrated Bluetooth,
enclosure and independent human review remain open before design submission.

# Nightwave — End-to-End Execution Plan for ChatGPT Work

## Execution philosophy

2026-10-01 final addendum: finish independent software/research, then stop for
builder Bluetooth architecture and display decisions before final schematic
tasks. See docs/bluetooth-architecture.md, docs/display-selection.md and
docs/engineering-budgets.md. The new scope includes lyrics, metadata/library,
modes/resume/sleep and Bluetooth output; old exclusions are superseded.
The provisional OLED and S3 pin map are not a final hardware lock.

2026-10-03: both builder choices are recorded: BD-14 S3 plus BM83SM1-00TA;
BD-15 Waveshare 24382 non-touch TFT. Before Builder Task 1: propagate both
choices, review the selected BD-16 TCA9535PWR strategy's combined map and current calculated
power/charge model and critical BOM/pins, and push/verify the software checkpoint.
docs/current-power-model.md replaces the old OLED runtime estimate; it does not
lock a battery or charger, or establish achieved runtime.
An old checklist below never authorizes skipping these prerequisites.

2026-10-01 builder-authored continuation: the builder reports direct Pixl
clarification allowing AI tutoring/review with actual human engineering work.
This supersedes any instruction below for Codex to author final schematic,
PCB routing or enclosure wholesale. Builder owns those sources and decisions;
Codex researches, calculates, explains, checks and reviews returned files.
Independent software/research work does not wait for builder authoring.
Follow docs/builder-tasks.md and record genuine choices in
docs/builder-decisions.md. No human hours or review approvals are inferred.

2026-10-01 funding-readiness override: digital Phase 4A/5 software and pre-funding
design preparation may proceed without bench results. Physical exit criteria
remain unverified, not waived or fabricated. Read `docs/pixl-funding-readiness.md`
for current design-stage requirements and original builder-authorship limits.
Complete digital work does not establish final hardware approval or runtime.

Current digital checkpoint: WAV/MP3 three-task streaming and OLED/five-button
product interaction, LRC/metadata/M3U/modes/resume/sleep and checked NVS are
implemented with recorded CI evidence. The 2026-10-02 continuation adds an
idle-built SD catalog, whole-card songs/artists/albums pages and indexed queues,
plus bounded PSRAM group lookups and checked ordinal-hint resume. See
docs/software-validation.md for separate execution evidence. SD/fallback latency
and other limitations remain in docs/library-index.md.
Runtime observation now adds explicit start/stop/status and minute JSON logs,
boot-lifetime accepted-media/fault counters, missed-sample/clock/paused/stalled
handling and permanent human-review status. It prepares Phase 16 evidence;
it does not complete that physical phase. See docs/runtime-test.md and
docs/trial-acceptance.md for the full retained systems and requirement mapping.
Physical Phase 4/5 acceptance is
pending. Final power, complete BOM, schematic/PCB/CAD and manufacturing are
NOT complete; the current builder-authorship gate is documented in
`docs/funding-readiness.md`. Do not equate software completion with full phase
or funding completion.

ChatGPT Work owns the **digital engineering workflow** and continuously moves the repository forward.

The human builder is primarily required at physical-world gates: buying parts, wiring, soldering, measuring, ordering fabrication, assembling, listening/testing, and recording evidence.
Original hardware design authorship and independent sanity checking are also
human requirements at the digital funding stage, not only after fabrication.

This plan is deliberately ordered to reduce the chance of paying for a PCB before the audio architecture is proven.

---

## Phase 0 — Pixl compliance + project bootstrap

### Objective
Start the project in a way that can be honestly reviewed and reproduced.

### ChatGPT Work tasks
- Re-read current Pixl hardware requirements, Trial requirements, tier description, and AI rules.
- Create/inspect GitHub repository.
- Install or verify ESP-IDF project structure.
- Create repository folders from Architecture.md.
- Add license.
- Add initial README skeleton.
- Add `.gitignore`.
- Create `docs/ai-disclosure.md`.
- Create `docs/decisions.md`.
- Verify Hackatime project tracking is configured before substantial coding.
- Record exact Trial text/requirements in `docs/requirements.md`.
- Create first architecture diagram.
- Commit bootstrap.

### Human gate
- Confirm the project is registered in Pixl and linked to the correct Hackatime project.
- Confirm the AI-assistance plan is acceptable for the intended submission.
- Ask `#pixl-help`/current Pixl support if the Trial specifically requires anything not in published docs.

### Deliverables
- public repo;
- tracking enabled;
- initial docs;
- first commit.

### Exit criteria
- no ambiguous Trial requirement remains that could invalidate the build;
- work tracking starts before implementation.

---

## Phase 1 — Engineering research and architecture lock

### Objective
Convert the product idea into a datasheet-backed electrical/firmware architecture.

### ChatGPT Work tasks
Research and document:
- ESP32-S3 module variants;
- PSRAM/flash needs;
- SDMMC pin/peripheral constraints;
- display options;
- MP3 decoder library options and licenses;
- WAV parsing;
- PCM5102A;
- headphone amplifier options;
- MAX98357A/newer alternatives;
- charger/power-path options;
- fuel-gauge options;
- regulator options;
- USB-C sink implementation;
- headphone jack detect options;
- speaker options;
- battery sizes/capacities;
- PCB fab/assembly constraints and part availability.

Create:
- `docs/architecture-notes.md`;
- `docs/decisions.md`;
- preliminary `hardware/BOM.csv`;
- preliminary power tree;
- pin-budget table;
- firmware dataflow.

Make a "why not DFPlayer" note.

### Human gate
Builder reviews the architecture and demonstrates understanding of:
- direct SD;
- software decode;
- buffering;
- I²S;
- headphone path;
- speaker path;
- charging/fuel gauge.

### Deliverables
- architecture lock;
- candidate parts;
- risk register;
- pin map draft.

### Exit criteria
No unresolved fundamental question about how audio moves from microSD to speaker/headphones.

---

## Phase 2 — Prototype BOM + bench wiring

### Objective
Create a safe prototype using development boards/breakouts before the final PCB.

### ChatGPT Work tasks
Create:
- exact prototype BOM;
- vendor alternatives;
- `docs/prototype-wiring.md`;
- pin-by-pin wiring table;
- voltage-domain table;
- expected current ranges;
- initial test-track set specification;
- bring-up checklist.

Prototype should represent:
- ESP32-S3 dev board;
- microSD interface;
- I²S DAC;
- headphone path;
- I²S speaker amp;
- legacy USB-powered bench OLED (not the final readable screen);
- five buttons.

Battery/charger is not required for first audio proof.

### Human gate
Buy/obtain parts and wire prototype.

Human returns:
- clear photos;
- exact module labels;
- power source;
- any measured voltages requested.

### Exit criteria
Work has enough verified physical information to write board-specific firmware without guessing module variants.

---

## Phase 3 — Firmware foundation

### Objective
Bring up peripherals independently with real hardware.

### ChatGPT Work tasks
Build ESP-IDF firmware components for:
- logging;
- SD mount/read;
- display;
- buttons;
- NVS/settings;
- I²S output;
- fuel-gauge stub/abstraction.

Create test modes:
- button test;
- display test;
- SD throughput test;
- I²S tone generator;
- L/R channel identification.

Add CI firmware build.

### Human gate
Flash firmware and run tests.

Human returns:
- console logs;
- photos/video where useful;
- whether test tone/channel output is correct.

### Exit criteria
Each hardware peripheral needed for the audio prototype works independently.

---

## Phase 4 — Audio engine core

### Objective
Implement the system that justifies the T4 target.

### ChatGPT Work tasks
Implement:

```text
StorageTask
  -> compressed ring buffer
DecoderTask
  -> PCM ring buffer
AudioOutputTask
  -> I²S DMA
```

Add:
- MP3 decoder;
- WAV PCM support;
- track open/close;
- EOF handling;
- pause/resume;
- next/previous;
- volume ramp;
- stereo-to-mono mix;
- format/sample-rate handling;
- buffer telemetry;
- underrun counter;
- decoder errors.

Create debug commands/metrics:
- SD throughput;
- decoder realtime factor;
- PCM buffer watermark;
- task stack high-water marks;
- heap/PSRAM use.

### Human gate
Run multiple files:
- MP3;
- WAV;
- different bitrates;
- at least two sample rates if supported.

Human listens for:
- glitches;
- channel reversal;
- clipping;
- track transitions.

### Deliverables
- functional direct-SD software audio player;
- profiling logs;
- no DFPlayer.

### Exit criteria
At least 2 hours of prototype playback with zero unexplained crashes and an acceptable underrun count.

---

## Phase 5 — Product interaction layer

### Objective
Turn the audio engine into an actual music player.

### ChatGPT Work tasks
Implement:
- now-playing screen;
- file/folder browser;
- five-button mappings;
- volume;
- battery placeholder UI;
- track timer where valid;
- headphone/speaker mode indicator;
- no-SD screen;
- corrupt-file behavior;
- settings persistence;
- display sleep;
- optional sleep timer.

Do not add decorative features that destabilize audio.

### Human gate
Usability test:
- navigate without serial console;
- start/pause/skip/volume;
- recover from errors.

### Exit criteria
A new user can operate prototype from physical controls/display alone.

---

## Phase 6 — Prototype power characterization

### Objective
Get real current data before battery sizing/final power design.

### ChatGPT Work tasks
Prepare measurement worksheet.

Request current measurements for:
- off/leakage if measurable;
- idle;
- display on;
- headphone playback at fixed volume;
- speaker playback at 25/50/75% volume;
- SD scanning;
- peak transitions.

Calculate:
- average power;
- battery-capacity target for 8 h;
- margin for 10 h target;
- expected regulator losses;
- charging-current/thermal requirements.

Produce:
- `docs/power-budget.md`;
- `measurements/current-draw.csv`.

### Human gate
Use multimeter/USB power meter/bench tools to supply requested measurements.

### Exit criteria
Battery capacity and regulator/charger selection are measurement-backed.

---

## Phase 7 — Final component/BOM lock

### Objective
Freeze the exact manufacturable electronics.

### ChatGPT Work tasks
For every final part:
- exact MPN;
- package;
- datasheet;
- price;
- source;
- footprint;
- 3D model;
- assembly capability;
- lifecycle/availability;
- substitutes if needed.

Finalize:
- MCU module;
- DAC;
- headphone amp;
- speaker amp;
- charger;
- regulator;
- fuel gauge;
- USB-C;
- microSD;
- display connector/module;
- buttons;
- jack;
- speaker connector;
- battery connector;
- passives;
- ESD/protection.

Update `hardware/BOM.csv` with total cost.

### Human gate
Approve budget and confirm what parts/fab services are realistically purchasable.

### Exit criteria
No DNP/TBD part remains in the core system.

---

## Phase 8 — Schematic creation and review

### Objective
Create the final custom schematic.

### ChatGPT Work tasks
Create/review KiCad hierarchy:
- POWER;
- MCU;
- STORAGE;
- DISPLAY_INPUT;
- AUDIO_DIGITAL;
- AUDIO_ANALOG;
- CONNECTORS_TEST.

Perform:
- pin mapping;
- decoupling;
- boot/programming circuit;
- USB-C power;
- charger/power path;
- fuel gauge;
- 3.3 V regulation;
- SD;
- I²S fanout;
- DAC/headphone amp;
- speaker amp;
- headphone detect;
- buttons/display;
- test points.

Run ERC.

Create `docs/schematic-review.md` containing:
- rail table;
- signal table;
- power-up behavior;
- unresolved warnings with justification.

### Human gate
Builder manually traces each subsystem using datasheets.
Then request a second human sanity check.

### Exit criteria
- all ERC items resolved/reviewed;
- no unexplained floating critical pins;
- second-person review complete.

---

## Phase 9 — PCB planning, placement, routing

### Objective
Create a manufacturable mixed-signal custom board.

### ChatGPT Work tasks
Before placement:
- select manufacturer rules;
- import validated footprints/3D models;
- define board outline jointly with CAD envelope;
- define connector edge positions;
- define mounting holes;
- define ESP32 antenna keepout.

Place by subsystem.

Route with attention to:
- return paths;
- audio analog;
- Class-D outputs;
- USB/power;
- SD;
- I²S;
- decoupling;
- high-current paths.

Add:
- ground planes;
- test points;
- silkscreen;
- revision marking;
- polarity/orientation labels.

Run DRC.

Create `docs/pcb-review.md` with screenshots/checklists.

### Human gate
Builder visually checks:
- orientation;
- jack/USB/SD placement;
- button positions;
- mounting holes;
- battery connector polarity;
- speaker connector polarity.

### Exit criteria
DRC reviewed and all mechanical interfaces have known positions.

---

## Phase 10 — Enclosure co-design

### Objective
Create a complete editable enclosure around the real PCB.

### ChatGPT Work tasks
Build/import board assembly.

Design:
- base;
- lid;
- PCB mounts;
- battery cradle;
- speaker mount;
- acoustic grille;
- display opening;
- button plungers;
- USB opening;
- headphone opening;
- SD access if intended;
- screw system;
- serviceability.

Run interference/clearance review.

Export:
- `.FCStd`;
- `.STEP`;
- `.STL`.

Create annotated assembly diagram.

### Human gate
Optional cheap prototype print or dimensional check before electronics order if geometry risk is high.

### Exit criteria
Complete CAD assembly proves all parts have a physical place and mounting method.

---

## Phase 11 — Pre-fabrication verification

### Objective
Prevent expensive first-revision mistakes.

### ChatGPT Work tasks
Create `docs/fabrication-checklist.md`.

Cross-check:
- BOM ↔ schematic ↔ PCB;
- pin 1/orientation;
- footprint dimensions;
- USB jack;
- SD socket;
- headphone jack;
- battery connector;
- button height;
- display geometry;
- screw/mounting holes;
- antenna keepout;
- PCB stackup;
- manufacturer DRC;
- assembly side/orientation;
- component availability.

Generate:
- Gerbers;
- drill files;
- position files;
- assembly BOM;
- schematic PDF;
- STEP PCB;
- manufacturing archive.

### Human gate — PAID ACTION
Builder reviews quote/order and explicitly approves fabrication.

### Exit criteria
Fabrication package is frozen under a tagged commit/release.

---

## Phase 12 — Final firmware on production pinout

### Objective
Prepare firmware for the real board before it arrives.

### ChatGPT Work tasks
- move hardware abstraction to final pin map;
- add production board config;
- keep prototype board config if useful;
- add charger/fuel-gauge driver;
- add headphone detect;
- add low-battery shutdown;
- add speaker/headphone power gating;
- create manufacturing self-test;
- clean diagnostics;
- build release image;
- document flashing/recovery.

### Exit criteria
Firmware builds for final PCB and has a bring-up/self-test path.

---

## Phase 13 — PCB assembly and safe bring-up

### Objective
Power the real board safely and isolate faults subsystem-by-subsystem.

### ChatGPT Work tasks
Generate exact bring-up sequence and interpret returned measurements.

### Human gate — PHYSICAL
Builder:
- inspects assembly;
- measures resistance to ground;
- powers with current limit where possible;
- measures rails;
- flashes MCU;
- tests buses;
- tests audio paths;
- only then connects battery.

Human returns measured values and photos/logs.

### Exit criteria
Final PCB performs playback through both output paths.

---

## Phase 14 — Mechanical assembly

### Objective
Build the complete product.

### ChatGPT Work tasks
Provide:
- assembly order;
- screw/fastener list;
- wire lengths if any;
- speaker/battery orientation;
- cable routing;
- fit troubleshooting.

### Human gate
Print enclosure, install hardware, close device, operate controls.

### Exit criteria
No loose electronics; controls/ports align; device is serviceable.

---

## Phase 15 — Hardening and T4 evidence

### Objective
Turn a working prototype into an engineered system with evidence.

### ChatGPT Work tasks
Design and analyze tests for:
- SD removal;
- corrupt file;
- unsupported file;
- repeated skip stress;
- long playback;
- headphone hot-plug;
- charge while playing;
- low-battery shutdown;
- reset recovery;
- stack/heap stability;
- underruns;
- thermal observations;
- SD throughput;
- decoder CPU load;
- current profiles.

Fix discovered firmware/source issues.

Update PCB/CAD source if physical bodges or fit changes were required.

### Human gate
Perform real tests and return logs/measurements.

### Exit criteria
Known failure modes are handled/documented and source matches final hardware.

---

## Phase 16 — Full-night battery validation

### Objective
Prove the Trial's battery requirement.

### ChatGPT Work tasks
Create exact test protocol and logging table.

Baseline acceptance test:
- 100% charge;
- built-in speaker;
- fixed declared volume;
- normal display timeout;
- known playlist;
- continuous playback;
- periodic SOC/voltage log;
- run until controlled shutdown.

Analyze results and generate runtime plot.

If runtime <8 h:
- diagnose dominant load;
- modify firmware/power policy/battery capacity;
- repeat test.

### Human gate
Run full-duration test.

### Exit criteria
Measured runtime >=8 h.

---

## Phase 17 — Documentation and reproducibility pass

### Objective
Make the project genuinely shippable.

### ChatGPT Work tasks
Finish README with:
- what/why;
- features;
- architecture;
- block diagram;
- photos/renders;
- BOM;
- build steps;
- flashing;
- loading music;
- controls;
- measured runtime;
- validation;
- limitations;
- AI disclosure;
- credits/licenses.

Verify repo contains:
- BOM CSV total;
- KiCad files;
- Gerbers;
- CAD source;
- STEP;
- firmware source;
- final photos;
- build photos;
- measurements.

### Human gate
Builder reads README and confirms it matches reality.

### Exit criteria
A stranger could reasonably reproduce the device from the repository.

---

## Phase 18 — Demo and Pixl ship package

### Objective
Present the physical project clearly.

### ChatGPT Work tasks
Prepare demo shot list:

1. show finished device;
2. show SD card/user music;
3. power on;
4. browse/play;
5. pause;
6. previous/next;
7. volume;
8. built-in speaker;
9. plug in headphones and show speaker mute;
10. show battery/runtime result;
11. brief PCB/enclosure shot.

Prepare:
- thumbnail plan;
- concise project description;
- technical highlights;
- honest AI disclosure;
- changelog if needed;
- submission checklist.

### Human gate
Record/upload final demo and submit.

### Exit criteria
Submission is complete with no missing source/evidence.

---

# ChatGPT Work's ongoing operating loop

Every Work session follows:

```text
READ DOCS
   ↓
INSPECT REPO
   ↓
READ memory.md
   ↓
IDENTIFY CURRENT PHASE
   ↓
EXECUTE DIGITAL WORK
   ↓
BUILD / CHECK / REVIEW
   ↓
COMMIT MEANINGFUL RESULT
   ↓
UPDATE memory.md
   ↓
CONTINUE
        │
        └── if physical action required → HUMAN GATE
```

Work must not stop merely because one task finished if more digital tasks in the same phase can be completed safely.

---

# What creates the T4 case

The strongest T4 evidence is not feature count. It is the combination of:

| Area | Evidence |
|---|---|
| Embedded systems | multi-task real-time audio pipeline |
| Storage | direct SD filesystem + failure handling |
| Codec work | software MP3/WAV decode |
| Real-time | ring buffers + DMA + underrun profiling |
| Audio electronics | DAC + headphone amp + Class-D speaker path |
| Power electronics | custom charger/power path + regulation + fuel gauge |
| PCB | custom mixed-signal board |
| Mechanical | complete custom CAD assembly |
| Reliability | fault/stress testing |
| Measurement | throughput, memory, current, thermal, runtime |
| Reproducibility | complete buildable repository |

Do not add random features until these are real.
