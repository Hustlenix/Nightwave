# Builder design authorship and review

Status: NOT COMPLETE. A review checklist is not proof of original authorship.
See `pixl-funding-readiness.md` for the current program rule.

Provide original editable design sources and commit history as the work evolves.
For each subsystem record the builder's decisions, source/datasheet, authored
changes, reasoning, reviewer and unresolved issues. Do not invent dates or hours.

- [ ] Power tree, input limits and charge-while-play behavior
- [ ] Battery protection, polarity, NTC, charge current and thermal limits
- [ ] MCU boot/reset/programming, decoupling and antenna keepout
- [ ] SD pull-ups, voltage, pin map and detect
- [ ] DAC/headphone gain, jack switching and output safety
- [ ] Speaker supply, BTL output and shutdown
- [ ] Regulator layout, load/peak margins and grounding
- [ ] Exact footprint geometry and pin-1/orientation
- [ ] PCB return paths, placement/routing and manufacturer rules
- [ ] Enclosure port datums, fasteners, battery clearance and interference
- [ ] Firmware GPIOs agree with schematic and PCB
- [ ] Sources and AI disclosure accurately describe who created each artifact

Request clarification from Pixl before submission if the authorship boundary is
uncertain. Do not claim that rubber-stamping generated hardware satisfies it.
