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
- Native ERC negative controls pass: deliberately conflicting outputs and a
  removed battery power declaration are both rejected in isolated design copies.
- Core scope documents explicitly distinguish the current reference architecture
  from historical charger, OLED/direct-button, power and authorship workflows.

## Executed checkpoint evidence

Latest software source `161cbc9711793a3932db5e07af251155a77d53fb` is pushed to
`main`. [Quality run 37445159892](https://github.com/Hustlenix/Nightwave/actions/runs/37445159892)
passed all 18 Release suites, all 18 ASAN/leak suites, 46 Python tests and the
project/reference validators. Both reference and bench ESP32-S3 builds passed
[firmware run 37445159915](https://github.com/Hustlenix/Nightwave/actions/runs/37445159915).
This adds serialized shared-I2C device registration and host concurrency,
ownership, timeout and partial-read failure tests against the actual adapters.
Charger timer screening now uses the Rev C table values (10.5 h minimum,
14.5 h nominal, 15.5 h maximum), with the conflicting 12 h prose recorded.
These changes do not alter or qualify the PCB, battery, USB interface or enclosure.

Earlier native CAD checkpoint (CAD unchanged by the latest software source):

Source `add96254341f786c73a4e9cfb6795bf37f1b4b58` is pushed to `main`.
[Project quality run 37441295569](https://github.com/Hustlenix/Nightwave/actions/runs/37441295569)
passed all 15 Release suites, all 15 AddressSanitizer/leak suites, 39 Python
tests and project/reference validators. Hardware tests remain unperformed.
Local native KiCad reports show zero ERC, DRC, unconnected and parity findings.
Both reference and legacy bench ESP32-S3 jobs passed in
[firmware run 37441295524](https://github.com/Hustlenix/Nightwave/actions/runs/37441295524).

## Still open; do not label the design complete

Continuation after add9625: shared I2C ownership and read-only MAX17048/BQ25628E
power telemetry are implemented; host conversion/failure tests pass locally.
The new reference-specific power calculation exposes concrete blockers:
320 mA default charging versus 10.05 Ah/10.5-hour minimum timer, >2 A transient harness
screen, USB current permission and pack-compatible temperature policy.
These are not resolved by reading registers. Source
b7566f34de4ed5d4800aa6802087daeb4587eb0e is pushed. Quality run
[37443602845](https://github.com/Hustlenix/Nightwave/actions/runs/37443602845)
passed 16 Release/16 ASAN-leak suites and 42 Python tests; both firmware
profiles passed [37443602596](https://github.com/Hustlenix/Nightwave/actions/runs/37443602596).
Procurement follow-up resolves 49 resistor MPNs against primary manufacturer
specifications; 48 other entries and complete current pricing remain open.

| Work | Exit evidence |
| --- | --- |
| Pixl authorship eligibility | Organizer clarification covering the actual AI-authored CAD, plus honest disclosure |
| Physical envelope/budget | Decided: PCB <=90x60 mm target, device about <=110x70x25 mm, INR15,000 electronics + PCB/assembly excluding tools. Final PCB/CAD builder-authored; feasibility and quotes remain open. See compact-builder-constraints.md. |
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
