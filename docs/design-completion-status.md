# Design completion ledger - 2026-10-06

Target: complete, reproducible Nightwave digital design. T4 is a reviewer decision,
not a claim. This document is engineering status, not a personal work journal.

## Completed in this checkpoint

- Separate reference/legacy firmware profiles and schematic-to-firmware pin checks.
- Reference TCA9535 button sampling/recovery, initial TFT transport/text/lyrics,
  and corrected headphone-enable versus DAC-mute GPIO handling.
- Explicit IC pin types in generated schematics, including BM83 MCLK direction;
  external supply/ground declarations instead of an all-passive ERC model.
- Schematic-only generation preserves existing project rules and footprint files.
- Packaging checks typed ERC as well as DRC/parity; it must not archive a failing
  native report or silently claim that digital checks certify electrical safety.

## Still open; do not label the design complete

| Work | Exit evidence |
| --- | --- |
| Pixl authorship eligibility | Organizer clarification covering the actual AI-authored CAD, plus honest disclosure |
| Physical envelope/budget | Builder's maximum size and parts/PCB budget; current PCB alone is 110x100 mm |
| USB source-current policy/protection | Reviewed CC/USB enumeration policy, startup current, ESD and connector implementation |
| Charger/battery safety | Verified limits, NTC curve/attachment, default/reset behavior, charge timing and pack approval |
| Power/current budget | Mode-specific calculations including transient load, connector/wire/copper limits and estimated runtime |
| Purchaseable BOM | Exact passives/NTC/harness/connector MPNs, ratings, availability and complete priced total |
| Mixed-signal layout | Reviewed switching loops, return paths, current necks, RF clearances, USB impedance and assembly process |
| Product firmware | BM83 live transport/provisioning, rate compatibility, real power backend, status/SD handling and improved TFT UI |
| Enclosure | Editable complete assembly, realistic component bodies, retention and interference/tolerance checks |
| External review | Named independent human review and resolution of material findings |
| Final build | Real assembly, listening/thermal/fit tests, overnight runtime, photos and video after funding |

No purchase, fabrication order, Pixl submission, organizer message or human
review has been performed by this checkpoint. No approval is inferred from CI.
