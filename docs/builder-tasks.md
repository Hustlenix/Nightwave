# Builder tasks and source-review handoff

2026-10-01 final-addendum override: **do not start final schematic/PCB/CAD tasks
until Bluetooth architecture (BD-14) and display (BD-15) choices are recorded,
the current software checkpoint is pushed/verified, selections propagated, and
the updated power model, BOM and GPIO budget are complete**.
Read bluetooth-architecture.md and display-selection.md. The existing S3 pin
table and OLED envelope are provisional and cannot be treated as final.

BD-14 is selected: S3 + BM83SM1-00TA. BD-15 is still open. Only after all the
entry checks above, start with task 1 (MCU); remaining tasks are assignments,
not a demand to complete the whole device before review. Return one subsystem
at a time. AI keeps independent research/test/documentation work moving, but
never claims your decisions/hours or authors final design files wholesale.

Before a long authoring session, verify genuine Hackatime tracking in your
editor; do not share keys. Record your actual choices/reasons in
[builder-decisions.md](builder-decisions.md). Save source under hardware/kicad
or enclosure, with required custom libraries. Screenshots support review but
are not substitutes for editable sources. No task below is marked completed.

Review loop: builder authors -> assistant traces against the exact datasheet,
identifies pin/net/footprint/value mistakes with explanations -> builder fixes ->
assistant re-reviews. Later add an independent human sanity check. For each
review record source commit, files, issue severity, required correction and
retest evidence. Do not mark ERC/DRC or fit passed unless actually executed.

## Bluetooth qualification checklist — not the first schematic builder task

### Objective

After the architecture choice, qualify an actual A2DP source path before locking its schematic.

### What I need to create

Builder-authored block for the selected module/MCU, control/provisioning access,
power/reset and digital audio interface; or integrate Classic Bluetooth in the
chosen main MCU block. Do not author both alternatives as if both were selected.

### Datasheets I need

Exact chosen module datasheet, source-firmware release notes/UART guide or
Espressif Classic A2DP source API/example; links in bluetooth-architecture.md.

### Engineering decisions I must make

Exact source-capable SKU/firmware, clocks/rates, provision/recovery access,
power-off/backfeed, GPIO allocation and antenna coexistence/keep-outs.

### Constraints

No sink-only/BLE substitution. Support the required sample rates via validated
conversion if needed. No sudden speaker fallback on wireless disconnection.
Never use a module's internal power output to feed arbitrary peripherals.

### Checklist

- [ ] Record BD-14 choice/reasons; verify real firmware/tool availability and source role.
- [ ] Reconcile display/SD/audio/control GPIO and rail/peak-current budgets.
- [ ] Return one-headphone digital/bench evidence under a separate safe human gate.
- [ ] Keep provisioning and firmware licensing reproducible; no unqualified codec claims.

### What files/screenshots to return

Actual builder source block/reference annotations, firmware/SKU/tool versions,
BD-14 entry, unresolved issues and real prototype logs when performed.

## BUILDER TASK — 1. Chosen MCU module

### Objective

After every entry gate above, create the retained ESP32-S3 MCU sheet. Reconcile
the combined BM83/TFT/SD/audio GPIO and updated power budget first.

### What I need to create

A KiCad module block with 3V3/GND, EN reset, GPIO0 boot, service access and named peripheral interfaces.

### Datasheets I need

Module datasheet pin table/reference/land pattern; Espressif schematic checklist (links in schematic-requirements.md §1).

### Engineering decisions I must make

Module suffix, reset timing, USB versus UART recovery and GPIO-to-module-pad mapping.

### Constraints

Use the module reference, not bare-chip crystal/flash circuitry. Do not assign PSRAM/strap/USB pins to other functions.

### Checklist

- [ ] Verify all grounds/center pad, local bulk/decoupling, EN default, boot access, no accidental rail shorts and header-consistent GPIOs.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable .kicad_pro/.kicad_sch plus custom symbol/library files; MCU-sheet PDF or screenshots, ERC report with unresolved items, and BD-01 choices.

## BUILDER TASK — 2. USB-C input and service

### Objective

Define an input that never overdraws its source.

### What I need to create

Connector/protection/CC block and a default/attached/detached current-policy table.

### Datasheets I need

Exact GCT suffix drawing; chosen CC controller if used (§2).

### Engineering decisions I must make

Power-only versus native USB, passive Rd versus current-aware sink, shield/protection and legacy-source policy.

### Constraints

No default highest-current assumption; no 5 V into GPIO; protect every powered-off path.

### Checklist

- [ ] Trace both plug orientations, CC independence/controller Rd, reset/dead-battery limit, source-change fallback and inrush.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable sheet, exact connector drawing, state/current table and BD-02/11 entries.

## BUILDER TASK — 3. Charger and power path

### Objective

Resolve the large-pack timer/source/thermal conflict.

### What I need to create

Chosen charger sheet, component calculations and POR/watchdog/fault table.

### Datasheets I need

power-options.md; exact selected charger/cell datasheets (§3).

### Engineering decisions I must make

Charger, current, timer, termination, temperature monitoring and recharge expectation.

### Constraints

Keep safety mechanisms; charge setting cannot create absent source power. No cell connection or ordering.

### Checklist

- [ ] Trace IN/BAT/SYS, CE, NTC open/short, timer, input cap and thermal estimate.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable sheet, value calculations, reset/fault table, source evidence and BD-03/04 entries.

## BUILDER TASK — 4. 3.3 V regulator and shutdown

### Objective

Provide stable logic power and real off behavior.

### What I need to create

Regulator sheet plus master-disconnect/wake schematic.

### Datasheets I need

TPS63802 reference/package or chosen alternative; switch/latch datasheet (§4).

### Engineering decisions I must make

Converter, effective caps, inductor, MODE/EN, charging while off and wake method.

### Constraints

SYS speaker branch must obey off policy; no invented latch just from POWER_HOLD.

### Checklist

- [ ] Check divider tolerance, inductor saturation, min-cell peak load, shutdown leakage paths.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable sheet, calculations, off/wake state diagram and BD-05 entry.

## BUILDER TASK — 5. Fuel gauge

### Objective

Sense the protected cell without back-powering switched logic.

### What I need to create

Gauge block and low-battery/hysteresis requirements.

### Datasheets I need

MAX17048 pin/application/I2C sections (§5).

### Engineering decisions I must make

Pull-up domains, alert versus polling and unknown/low/recovery states.

### Constraints

MAX17048 VDD senses the cell; CELL is not its internal sense input.

### Checklist

- [ ] Check VDD bypass, CTG/EP/GND, QSTRT, bus off state and ALRT polarity.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block and BD-06 entry; no SOC/runtime measurement assumed.

## BUILDER TASK — 6. microSD

### Objective

Connect one-bit storage with all required biasing.

### What I need to create

Socket, decoupling, detect and protection block.

### Datasheets I need

Exact Molex sales drawing and Espressif SD pull-up requirements (§6).

### Engineering decisions I must make

Socket, detect polarity, inrush and protective/series components.

### Constraints

External CMD/DAT0–3 pull-ups even for one-bit; 3.3 V contacts.

### Checklist

- [ ] Compare contact numbers to drawing, card insertion envelope and current GPIO map.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, actual socket drawing and BD-07 entry.

## BUILDER TASK — 7. Builder-selected display

### Objective

Implement the BD-15 chosen display interface after its combined budget review.

### What I need to create

Exact module/connector/reset block and dimension reference.

### Datasheets I need

Actual chosen module supply/pin/dimension/controller drawing; display-selection.md.

### Engineering decisions I must make

Exact MPN/controller, SPI versus I2C, reset/backlight, voltage and mounting.

### Constraints

Do not reuse the old OLED pin/window assumptions for TFT. Budget DMA/PSRAM,
backlight current and SPI GPIO together with the BD-14 Bluetooth transport.

### Checklist

- [ ] Check controller, logic rail, pull-ups, sleep/off bus and mount/window dimensions.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, vendor drawing/URL, exact MPN and BD-15 entry.

## BUILDER TASK — 8. Five buttons

### Objective

Provide reliable electrical controls and mechanically supported actuators.

### What I need to create

Five switch blocks with named controls and exact switch drawing.

### Datasheets I need

Chosen switch maker datasheet; firmware interface contract (§8).

### Engineering decisions I must make

Switch package/force/travel, pull-up/RC, exposed-interface ESD.

### Constraints

Active-low mapping; do not accidentally ground unused common pads or a strap.

### Checklist

- [ ] Confirm commoning, rest/pressed state, debounce compatibility and actuator height.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, switch drawing and BD-08 entry.

## BUILDER TASK — 9. PCM5102A DAC

### Objective

Create the line-level stereo source.

### What I need to create

DAC block, pump/decoupling/filter circuit and mute requirement.

### Datasheets I need

TI pin table and reference figure 33 (§9).

### Engineering decisions I must make

3-wire mode, FMT/FLT/DEMP, filtering and mute sequencing.

### Constraints

No headphones directly on line output; GPIO18 enable relationship is not finalized.

### Checklist

- [ ] Trace each pump/supply pin, I2S, output routing and shutdown timing.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, pin trace and BD-09 entry.

## BUILDER TASK — 10. Headphone amplifier

### Objective

Provide a deliberately bounded, low-pop loaded output.

### What I need to create

Amp block, gain/attenuation budget and chosen input circuit.

### Datasheets I need

TPA6132A2 single-ended reference/gain table (§10).

### Engineering decisions I must make

Gain straps, input attenuation, high-pass corner and loaded output target.

### Constraints

A chosen voltage is not universal hearing safety; no listening at uncontrolled full scale.

### Checklist

- [ ] Check both input pairs, pump caps, EN low on reset, load clipping and hotplug policy.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, calculations and BD-09 entry.

## BUILDER TASK — 11. Speaker amplifier

### Objective

Implement the exact part's I2S/BTL path.

### What I need to create

Amp supply/DAI/gain/enable/output block.

### Datasheets I need

MAX98360 exact FC2QFN or WLP pin/DAI/package sections (§11).

### Engineering decisions I must make

Exact suffix/package, mono selection, fixed gain and supply limits.

### Constraints

CEFB+T is FC2QFN, not WLP; OUTP/N never ground.

### Checklist

- [ ] Verify corrected output pin map, DAI configuration, disabled boot and supply current.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, exact footprint/reference drawing and BD-10 entry.

## BUILDER TASK — 12. Headphone jack and detect

### Objective

Produce digital insertion indication without biasing audio.

### What I need to create

Exact jack plus isolated-contact/detector circuit and truth table.

### Datasheets I need

Actual selected jack switch drawing; detector datasheet if used (§12).

### Engineering decisions I must make

Jack/contact topology, logic polarity, ESD and default mode.

### Constraints

Do not pull an audio switch contact to 3.3 V without a justified detector.

### Checklist

- [ ] Trace every contact absent/present and annotate GPIO16 low/high behavior.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, source drawing, circuit truth table and BD-08/11 entries; bench continuity only when actually obtained.

## BUILDER TASK — 13. Speaker connector

### Objective

Retain and service the BTL speaker wiring.

### What I need to create

Rated keyed connector and mating cable definition.

### Datasheets I need

Exact connector/mate current/geometry drawing (§13).

### Engineering decisions I must make

Connector, cable/current/retention and service direction.

### Constraints

SPK_P/N are not power/ground; no chassis/ground contact.

### Checklist

- [ ] Check pin/mate orientation, current and strain relief.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, connector drawing and BD-11 entry.

## BUILDER TASK — 14. Battery connector and NTC

### Objective

Provide safe pack polarity, retention and real temperature sensing.

### What I need to create

Pack connector/sensor block with actual polarity and retention requirements.

### Datasheets I need

Pack, connector/mate and NTC datasheets (§14).

### Engineering decisions I must make

Exact pack/current capability, connector, reverse protection and sensor mounting.

### Constraints

No cell-wrap modification/soldering; no assumed JST polarity; no fixed TS bypass.

### Checklist

- [ ] Check polarity, wire rating, protection and NTC fault behavior.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable block, supplier evidence, sensor plan and BD-04/11 entries.

## BUILDER TASK — 15. Test and programming

### Objective

Make recovery and safe diagnostics accessible.

### What I need to create

Named test points, boot/reset/service pads and measurement access.

### Datasheets I need

MCU recovery reference and selected connector/probe drawings (§15).

### Engineering decisions I must make

UART/USB access, pad geometry and current measurement link.

### Constraints

No grounded probe on BTL output; no 5 V UART GPIO.

### Checklist

- [ ] Check access after assembly and readable net/polarity labels.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable sheet and BD-01/11 entry.

## BUILDER TASK — 16. PCB outline, placement and routing

### Objective

Author the reviewed circuit as a manufacturable board.

### What I need to create

Actual outline/mounts, validated footprints, placements, routes and planes.

### Datasheets I need

layout-mechanical-requirements.md; exact fab stackup/rules and package drawings.

### Engineering decisions I must make

Board geometry, stackup, port datums, placement and routing choices.

### Constraints

Do not start final routing before schematic/footprint review; builder performs placements/routes.

### Checklist

- [ ] Run DRC; inspect returns, power loops, antenna exclusion, source/CAD alignment and assembly capability.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable .kicad_pcb/.kicad_pro, custom libraries, stackup/rules, DRC report, layer views, STEP and BD-12 entry.

## BUILDER TASK — 17. Editable enclosure assembly

### Objective

Author a serviceable fitted enclosure around real components.

### What I need to create

Base/lid, mounts/retention, control guides, port openings and complete assembly.

### Datasheets I need

layout-mechanical-requirements.md; actual board/component/fastener drawings.

### Engineering decisions I must make

Datums, material/process tolerances, cavity/grille, screws, cable/tool access.

### Constraints

No final enclosure around guessed OLED/jack/speaker dimensions; no compressed battery.

### Checklist

- [ ] Check worst-case clearances, interference, assembly order, thermal/RF envelope and removability.
- [ ] Record real decisions/reasons, source revision and unresolved questions.

### What files/screenshots to return

Editable native CAD, electronics-inclusive STEP, sections/dimension/tolerance table, interference report and BD-13 entry.

