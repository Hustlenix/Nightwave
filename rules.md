# rules.md

# Nightwave — Implementation Rules

2026-10-01 final addendum: Bluetooth A2DP SOURCE and a larger/readable display
are required research/decision gates before final schematic work. Wireless
output is permitted; cloud/phone dependency is not. Builder materially authors
the final schematic, PCB placement/routing and editable enclosure; AI researches,
implements software, explains and reviews. Record real choices only after the
builder makes them. Never infer hours, measurements, hardware success or approval.

These rules govern ChatGPT Work, coding agents, and human implementation.

## 1. Product truth

- The device MUST be a standalone offline music player.
- The device MUST play user-supplied local audio files.
- The device MUST NOT depend on a phone, app, Wi-Fi, Bluetooth, streaming service, or cloud account for playback.
- The final submitted hardware MUST use the custom PCB and custom enclosure.
- A T4 rating MUST be treated as an objective, never as a guaranteed outcome.

## 2. AI / Pixl authorship

- ChatGPT Work MAY research, propose, scaffold, calculate, inspect, test software, review, and diagnose.
- ChatGPT Work MUST NOT fabricate physical measurements, build photos, runtime numbers, reviewer feedback, or human work.
- The human builder MUST understand and approve the electrical/mechanical architecture.
- Submitted PCB/CAD MUST NOT be blindly accepted as fully AI-generated design files.
- The builder MUST materially author/review the design and disclose AI use accurately.
- Before Pixl design submission, a second human MUST sanity-check the design.
- If Pixl rules conflict with this package, current Pixl rules win.

## 3. Source-of-truth order

When requirements conflict, authority is:

1. current Pixl/Trial rules;
2. `PRD.md`;
3. `Architecture.md`;
4. `rules.md`;
5. `phases.md`;
6. `design.md`;
7. `memory.md`;
8. implementation details.

A change to a higher-authority document MUST propagate to affected lower-authority docs.

## 4. Datasheet rule

- Electrical values MUST come from the current authoritative manufacturer datasheet or official documentation.
- Blog/forum values MAY be used as debugging anecdotes only.
- Every IC/module locked into the final design MUST have its exact manufacturer part number recorded.
- Every supply voltage, logic level, pin function, charger setting, and absolute maximum relevant to the design MUST be checked.
- Work MUST NOT infer pinouts from visually similar modules.

## 5. Part-selection rule

Before a part is locked:
- check manufacturer status;
- check exact package;
- check current availability;
- check assembly capability;
- check price;
- check footprint/model;
- check operating voltage;
- check thermal/current suitability.

Do not freeze a part solely because an AI remembers it.

## 6. Prototype-first rule

- The core audio architecture MUST be prototyped before final PCB ordering.
- At minimum, direct SD read + software decode + I²S output MUST work on prototype hardware first.
- Power architecture may be prototyped separately.
- Final PCB generation may proceed in parallel, but paid fabrication MUST NOT be ordered until critical architecture gates pass or the risk is explicitly accepted by the builder.

## 7. Firmware architecture

- Firmware MUST use ESP-IDF unless the architecture docs are explicitly revised.
- Playback MUST NOT use DFPlayer or another module that performs the complete file/decode/playback pipeline.
- The ESP32 MUST directly access the filesystem.
- MP3 decode MUST happen in firmware.
- I²S output MUST use DMA.
- Audio data MUST be buffered.
- Audio output MUST have an underrun counter.
- UI and SD reads MUST NOT block the audio output task.
- Persistent settings MUST be versioned/validated.
- Errors MUST be explicit, not silently swallowed.

## 8. Code structure

- No monolithic `main.c/main.cpp` containing the entire product.
- Each major subsystem MUST have an explicit module/component.
- Hardware-specific constants MUST be centralized.
- Magic numbers MUST NOT be scattered through playback code.
- Public APIs SHOULD be narrow.
- Global mutable state SHOULD be minimized.
- ISR handlers MUST do minimal work and defer processing.
- Logging SHOULD include subsystem tags.

## 9. Real-time rules

- AudioOutputTask MUST have enough priority to keep DMA fed.
- Buffer sizing MUST be based on measured latency/throughput.
- Task priority/core affinity MUST be profiled before being treated as final.
- Long filesystem/UI operations MUST NOT run in time-critical audio paths.
- Allocation inside hard playback loops SHOULD be avoided after steady-state start.
- An underrun is a measurable fault and MUST be logged.

## 10. Audio rules

- Stereo-to-mono mixing MUST avoid overflow/clipping.
- Volume transitions SHOULD ramp to avoid clicks.
- Sample-rate changes MUST be explicitly handled.
- Headphone DAC output MUST feed an appropriate headphone driver; do not drive low-impedance headphones directly unless the datasheet explicitly supports it.
- Speaker outputs from a bridge/Class-D amp MUST NOT be connected to ground as though they were single-ended.
- Speaker and headphone paths MUST have explicit mute/enable behavior.
- Analog and Class-D layout guidance from datasheets MUST be followed.

## 11. Power rules

- Lithium cells are safety-critical.
- A battery MUST NOT be connected before polarity and rail checks.
- Use protected cells or a documented protection system.
- Use a legitimate single-cell charger.
- Charging current MUST match battery capability.
- Thermal behavior MUST be considered for the charger.
- USB-C power input MUST include correct CC configuration.
- Power-up SHOULD use current limiting during first bring-up where possible.
- Never instruct bypassing battery protection, intentionally shorting a cell, puncturing/heating/crushing it, or unsafe charging.

## 12. PCB rules

- Final PCB MUST be custom.
- DRC MUST use the intended manufacturer's capabilities.
- DRC errors MUST be investigated, not globally disabled.
- ERC warnings MUST be reviewed individually.
- Every custom footprint MUST be checked against a mechanical drawing.
- Connector orientation MUST be cross-checked against enclosure CAD.
- ESP32 antenna keepout MUST be respected.
- High-current switching/Class-D loops MUST be kept away from sensitive analog audio.
- Decoupling components MUST be placed close to the pins they support.
- Major rails/buses MUST have test points.
- Silkscreen MUST show important polarity/orientation.
- No internet-downloaded footprint is trusted without dimension verification.

## 13. CAD rules

- Enclosure MUST have editable source and STEP export.
- Electronics MUST be mechanically secured.
- Battery MUST have a safe cradle/pocket and clearance.
- Speaker MUST have a defined mount and grille/opening.
- PCB standoffs/bosses MUST align with PCB mounting holes.
- Port cutouts MUST be derived from PCB/connector geometry.
- Tape/glue MUST NOT be the primary structural retention method.
- CAD MUST include the electronics assembly or references accurate enough to prove fit.
- Final design MUST be printable/manufacturable with declared tolerances.

## 14. UI rules

- The player must be operable without reading documentation.
- Dedicated physical play/pause, previous, next, volume-, volume+ controls are the baseline.
- Playback controls MUST work even when a menu is open unless there is a strong documented reason otherwise.
- Error screens MUST state what is wrong and what the user can do.
- Battery percentage MUST come from the fuel gauge/validated estimate, not a fake animation.
- Do not display time/progress data the decoder cannot actually know.

## 15. Runtime/measurement rules

- Predicted runtime MUST be labeled "estimated."
- Final runtime MUST come from a real continuous test.
- Every measurement artifact MUST state test conditions.
- Do not round a failed 7 h 40 min result into "8 hours."
- If runtime misses the requirement, redesign or state the failure; do not hide it.
- Current-draw measurements SHOULD be stored in CSV.
- Runtime SOC measurements SHOULD be stored in CSV and graphed.

## 16. Repository rules

Required top-level outputs:
- README;
- firmware source;
- KiCad source;
- BOM CSV with total;
- manufacturing exports;
- editable CAD;
- STEP;
- build photos;
- final photos;
- demo video link;
- validation data.

- Repository MUST remain organized.
- No giant dump of unrelated exported files.
- Large media SHOULD be stored sensibly and linked if repo limits make direct storage inappropriate.
- Generated files MUST be reproducible from source when practical.

## 17. Git rules

- Initialize Git before substantial implementation.
- Commit after meaningful milestones.
- Commit messages MUST describe the change.
- Do not squash all genuine development history into one final commit before review.
- Do not generate fake history.
- Do not commit secrets.
- API keys/tokens MUST never be committed.
- Preserve the user's existing Hackatime/WakaTime credentials; never expose or rotate them as part of this project.

## 18. Tracking/journal rules

- Ensure the project is registered/tracked before substantial work.
- Journal after real work sessions.
- Journals MUST describe actual changes, failures, tests, and decisions.
- Never invent hours.
- Autonomous AI work MUST NOT be represented as human engineering time.
- Physical build sessions MUST be documented with progress photos when relevant.

## 19. Testing rules

Before release:
- firmware builds from clean checkout;
- supported test tracks play;
- speaker works;
- L/R headphones work;
- button controls work;
- SD fault behavior is tested;
- headphone insertion behavior is tested;
- charging behavior is tested;
- low battery is tested;
- long playback is tested;
- full-night runtime is tested;
- enclosure fit is tested;
- final PCB source matches any bodges/revisions made during assembly.

## 20. ChatGPT Work behavior

At the beginning of every Work session:
1. read all six execution docs;
2. inspect repository status;
3. read `memory.md`;
4. identify current phase;
5. execute only work that advances the current phase;
6. run relevant checks;
7. commit meaningful completed work if repository access permits;
8. update `memory.md` with durable new state.

Work SHOULD continue autonomously through digital tasks.

Work MUST STOP at HUMAN GATES requiring:
- purchase;
- soldering;
- physical connection;
- multimeter/oscilloscope measurement;
- battery handling;
- fit check;
- fabrication order;
- physical runtime test;
- human Pixl sanity check.

At a HUMAN GATE, Work must provide:
- exact action;
- exact connection/measurement point;
- expected result/range;
- safety note;
- what evidence/result the human should return.

## 21. Definition of done

A task is done only when:
- source is created/updated;
- relevant build/check passes;
- result is documented;
- no known blocker is hidden;
- `memory.md` reflects durable status.

A phase is done only when its exit criteria in `phases.md` are met.

The project is done only when the physical device passes the PRD launch definition.
