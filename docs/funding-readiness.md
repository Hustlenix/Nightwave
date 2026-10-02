# Funding readiness

Checked: 2026-10-01. **NOT READY.** This checklist records evidence, not intent.

- [x] Public GitHub repository and organized firmware/docs
- [x] Firmware source and passing baseline firmware/host CI
- [x] Honest AI disclosure; no claimed physical success
- [x] WAV/MP3 three-task source, OLED/five-button browser, volume persistence;
  firmware and host CI (see software-validation.md for limits)
- [ ] Final BOM with every part linked, priced and totalled
- [ ] Builder Bluetooth architecture choice and verified A2DP SOURCE solution
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
- [ ] Project 1200 current Trial/linkage and funding fields checked in session

Software evidence: [device build at a2aa741](https://github.com/Hustlenix/Nightwave/actions/runs/36865138056),
[six Release + AddressSanitizer suites at 6349cd0](https://github.com/Hustlenix/Nightwave/actions/runs/36865602237).

Latest expanded-scope evidence: [50df90c device build/size report](https://github.com/Hustlenix/Nightwave/actions/runs/36882011549)
and [92c17ea Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36882687260),
7/7 Release and 7/7 AddressSanitizer/leak suites. Remaining library/BT/TFT/power
software is listed in expanded-software-checkpoint.md; full production software
completion is not asserted.

2026-10-02 software continuation: [14b10c0 device build/size](https://github.com/Hustlenix/Nightwave/actions/runs/36962788706)
and [Project quality, 8 Release + 8 ASAN/leak suites](https://github.com/Hustlenix/Nightwave/actions/runs/36962788677)
passed. The SD-backed songs/artists/albums catalog and indexed/filter/folder
resume are implemented with [explicit limits](library-index.md). Fast filtered
navigation, MP3 duration/seek, selected BT/TFT adapters and reviewed fuel/power
HAL/acceptance remain incomplete. Funding status remains NOT READY.

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
First decide [Bluetooth architecture](bluetooth-architecture.md) and
[display](display-selection.md); then start [the MCU builder task](builder-tasks.md), using
[subsystem requirements](schematic-requirements.md). Return the actual sources
for specific review, then revise and re-review. PCB/CAD constraints are in
[layout and mechanical requirements](layout-mechanical-requirements.md).

Final component lock, source-level footprint verification, ERC/DRC, mechanical
interference checks, exports and independent sanity review depend on those
sources. An authenticated Project 1200 session is still needed to check its
funding fields. There is no finished funding package to submit yet.

Current priced BOM is only a preliminary core subtotal, not a funding/order total.
The historical candidate subtotal is $55.97, excluding new BT/display choices.
It is not the updated product cost. Expanded-load calculations in
[engineering budgets](engineering-budgets.md) invalidate a blanket eight-hour
claim for the 6600 mAh candidate. Power calculations identify a larger
pack, but charge-timer and peak-load compatibility are unresolved; final power
design is not marked complete. Builder hardware authorship is a genuine gate,
not the obsolete physical-before-software gate. Review/approval alone does not
automatically turn a fully AI-generated hardware package into eligible work.
Physical validation remains pending and is not the reason digital tasks stop.
Do not submit or label the package ready until mandatory design items are checked.
