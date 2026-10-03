# Funding readiness

Repository checked: 2026-10-03. **NOT READY.** This checklist records evidence, not intent. The authenticated Project 1200 form snapshot below is dated 2026-10-02, not a fresh platform check.

- [x] Public GitHub repository and organized firmware/docs
- [x] Firmware source and passing baseline firmware/host CI
- [x] Honest AI disclosure; no claimed physical success
- [x] WAV/MP3 three-task source, OLED/five-button browser, volume persistence;
  firmware and host CI (see software-validation.md for limits)
- [ ] Final BOM with every part linked, priced and totalled
- [x] Builder Bluetooth architecture choice: ESP32-S3 plus BM83SM1-00TA
- [ ] Qualified BM83 A2DP SOURCE transport and rate handling
- [ ] Builder display choice and updated GPIO/memory/power/mechanical budgets
- [ ] Remaining expanded-scope software and declared performance acceptance
- [ ] Builder-authored schematic and PCB sources
- [ ] Footprint verification and reviewed ERC/DRC
- [ ] Gerbers, drills, CPL, assembly BOM, schematic PDF and board STEP
- [ ] Editable enclosure and electronics-inclusive STEP assembly
- [ ] Every component has a mount and accessible interface
- [ ] Assembly/exploded/PCB renders from actual sources
- [ ] Funding-quality README with all artifacts and complete build instructions
- [ ] Builder's original hardware-design provenance and understanding
- [ ] Independent sanity-check feedback addressed
- [x] Project 1200 current Trial/linkage and funding fields checked in session
- [ ] Complete funding-form attachments and shipping-inclusive costs

See [Project 1200 audit](project-1200-audit.md) for the authenticated 2026-10-02
form check, journal-hour issue and the separate design funding/final build gates.
The original journal prose needs the builder's own correction; no new journal
sessions or personal time are inferred from software checkpoints.

Current verified baseline is `8bb9232`: [ESP32-S3 firmware](https://github.com/Hustlenix/Nightwave/actions/runs/37021176284)
and [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/37021176476)
passed, including 11/11 Release, 11/11 ASAN/leak suites and nine synthetic funding
inventory tests. This includes the sampled runtime recorder, not a physical runtime
test. See [software-validation.md](software-validation.md) for revision-specific
evidence. The entries below are historical; their old MP3 duration/seek gap is
superseded by current software. Later source changes need their own CI evidence.

Software evidence: [device build at a2aa741](https://github.com/Hustlenix/Nightwave/actions/runs/36865138056),
[six Release + AddressSanitizer suites at 6349cd0](https://github.com/Hustlenix/Nightwave/actions/runs/36865602237).

Historical expanded-scope evidence: [50df90c device build/size report](https://github.com/Hustlenix/Nightwave/actions/runs/36882011549)
and [92c17ea Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36882687260),
7/7 Release and 7/7 AddressSanitizer/leak suites. Remaining library/BT/TFT/power
software is listed in expanded-software-checkpoint.md; full production software
completion is not asserted.

2026-10-02 software continuation: [14b10c0 device build/size](https://github.com/Hustlenix/Nightwave/actions/runs/36962788706)
and [Project quality, 8 Release + 8 ASAN/leak suites](https://github.com/Hustlenix/Nightwave/actions/runs/36962788677)
passed. The SD-backed songs/artists/albums catalog and indexed/filter/folder
resume are implemented with [explicit limits](library-index.md). Later v2 lookup
source adds accelerated filtered navigation; its evidence is recorded separately
in software-validation.md. Fallback/SD-latency acceptance, MP3 duration/seek,
selected BT/TFT adapters and reviewed physical fuel/power adapters were incomplete
at that checkpoint. Current MP3 duration/seek and portable HAL contracts are tested;
selected hardware integrations and measured performance remain incomplete.
Funding status remains NOT READY.

## Builder-authored continuation

On 2026-10-01 the builder reported direct Pixl clarification permitting AI as
a tutor/second pair of eyes when the builder does the actual schematic/board
engineering and makes the final decisions. This is user-reported guidance,
not independently verified correspondence. It enables mentoring now; it does
not establish that human design work or hours have already occurred.

Research, calculations, candidate-part/footprint checks, firmware, test work,
PCB/CAD constraints and review preparation continue independently. Codex must
not generate the final schematic, routing or enclosure wholesale. The builder
authors those editable sources and records choices in builder-decisions.md.
BD-14 selects S3 plus BM83 and [BD-15](display-selection.md) selects Waveshare
24382 non-touch (2026-10-03). Before [the MCU builder task](builder-tasks.md),
resolve BM83 source provisioning, selected TCA9535 BD-16 combined map and current power/
battery/BOM requirements. Use [subsystem requirements](schematic-requirements.md).
Return the actual sources
for specific review, then revise and re-review. PCB/CAD constraints are in
[layout and mechanical requirements](layout-mechanical-requirements.md).

Final component lock, source-level footprint verification, ERC/DRC, mechanical
interference checks, exports and independent sanity review depend on those
sources. Recheck the authenticated Project 1200 fields at submission; the last
recorded check is 2026-10-02. There is no finished funding package to submit yet.

Current priced BOM is only a preliminary core subtotal, not a funding/order total.
The active priced-core subtotal is **$53.16**, including selected BM83 and 24382,
excluding battery, support parts, isolated BT rail and other unpriced rows.
The old $43.67/$55.97 subtotals and
6600 mAh OLED-era runtime model are historical, not an updated product cost or
battery selection. [Current route/idle/peak/regulator/charging screening](current-power-model.md)
uses the selected hardware, not measured loads; charge-timer, source-current and thermal/peak-load constraints remain
unresolved. See [engineering budgets](engineering-budgets.md). Final power design
is not marked complete. Builder hardware authorship is a genuine gate,
not the obsolete physical-before-software gate. Review/approval alone does not
automatically turn a fully AI-generated hardware package into eligible work.
Physical validation remains pending and is not the reason digital tasks stop.
Do not submit or label the package ready until mandatory design items are checked.

The separate [finished-device evidence gate](trial-acceptance.md#finished-device-evidence-inventory)
also requires actual standalone speaker/control demonstrations, measured battery
runtime, real build photos and a demo video. Funding artifact presence does not
establish any of those results.
