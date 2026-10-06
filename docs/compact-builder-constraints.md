# Compact Nightwave requirements - 2026-10-06

User-approved constraints, not a finished layout or enclosure design.
Supersedes the former unanswered size/budget decision and prior permission to
produce the final PCB routing or final enclosure CAD automatically.

## Envelope and authorship

- PCB target <=90 x 60 mm; finished device target about <=110 x 70 x 25 mm.
  A few extra millimetres need an engineering reason and builder acceptance.
- The 110 x 100 mm AI reference is historical engineering input, not the final
  pocket-player board. Do not preserve its size merely to reuse routing.
- Builder authors final KiCad layout/routing and editable enclosure CAD.
  AI continues research, calculations, software/tests, sourcing, requirements
  and review. Do not shrink, reroute, generate or overwrite final CAD files.
- Retain selected S3 + BM83SM1-00TA, Waveshare 24382 and TCA9535; no silent
  substitutions or deletion of speaker, headphones, SD, controls or lyrics.

## Budget

Electronics plus PCB fabrication/assembly target maximum: INR15,000, excluding
reusable tools. Conservatively include delivery, taxes, import charges, setup,
stencil, minimum order quantities and spares within this ceiling. Enclosure
production cost is reported separately until inclusion is clarified, not free.

Planning allocations, NOT quotes: electronics INR8,000, PCB/assembly INR4,000,
freight/tax/contingency INR3,000. Rebalance against dated quotes, never by cutting
safety or assembly capability. There are 48 unresolved reference BOM entries
and no complete landed total. Partial prices do not establish affordability.
Quote the actual final board/stackup/quantity, not the large reference exports.

## Battery-independent power screening

Run `python tools/compact_design_screen.py`. Assumptions: 3.7 V nominal,
80% usable energy, 80% aging factor, 20% load margin, eight hours. These are
estimates, not measured performance. No candidate capacity is a pack selection.

| Hypothetical capacity | Maximum average cell-side power for 8 h |
| --- | ---: |
| 4,000 mAh | 0.987 W |
| 5,000 mAh | 1.233 W |
| 6,000 mAh | 1.480 W |
| 8,000 mAh | 1.973 W |

Existing reference speaker + retained BT estimate is 1.959 W, requiring about
7,940 mAh; wired requires about 6,775 mAh and Bluetooth about 6,674 mAh under
these assumptions. These are not final specifications. Worst-practical 1 W
speaker sensitivity requires about 16,077 mAh; do not automatically pick a huge
pack to satisfy that sensitivity. Define listening level and display duty,
implement safe inactive-route isolation/dimming, then recalculate. Muting is
not power-off. Peak current remains a separate protection/path constraint.

Neither historical 6,600 mAh nor reference 10,050 mAh is selected for the compact
device. Final sizing follows finalized BT/TFT/speaker/power topology, losses,
load profiles, minimum guaranteed capacity, protection, temperature and connector
limits. Actual runtime requires later physical measurement.

## Mechanical screening, not CAD

Illustrative 2 mm walls leave a 106 x 66 x 21 mm internal bounding box. A 1.2 mm
PCB leaves 19.8 mm total vertical space for components, display, pack, insulation,
retention and tolerance. Bosses, ports and cables reduce usable space further.

The [reference Adafruit 5035 pack](https://www.adafruit.com/product/5035) is
66.6 x 55.3 x 18.7 mm (manufacturer checked 2026-10-06). Stacking it over that
PCB leaves only 1.1 mm for everything else under these assumptions. This flags
a stack conflict, not proof all arrangements fail. Do not compress a cell to
fit. Use supplier-specific dimensional/swelling allowances for the eventual pack.

Builder requirements:

- Dimensioned XY/Z stack for exact display/module revisions, connectors/mates,
  cable bends, SD insertion, plug-body access and assembly sequence.
- Separate manufacturer RF exclusion volumes for both radios, accounting for
  every copper layer, battery, speaker metal, fasteners and enclosure material.
- Speaker retention/cone clearance/front-back acoustic separation; no invented
  acoustic tuning. Buttons need actuation travel and tolerance analysis.
- Battery service access, insulated retention, strain relief and NTC contact;
  no screws/edges pressing on cells; thermal review around charger/regulators.
- Actual fabrication stackup/impedance, power loops/returns, courtyards and
  tolerance/interference checks before routing release.

## Remaining non-builder closure before final handoff

1. USB source-current permission and reset-safe charger control; compatible NTC,
   charge limits, timing and fault behavior. Read-only telemetry is not safety control.
2. Final mode-specific energy/current/charge model with the compact constraints.
3. Exact sourced BOM and dated landed INR quote within target.
4. BM83 transport/provisioning/audio-rate integration and digital tests.
5. Verified manufacturer drawing register for mechanical and footprint requirements.

I2C race fix 161cbc9 is already pushed: CI37445159892 passed 18 Release,
18 ASAN/leak suites and 46 Python tests; firmware37445159915 passed both profiles.
New compact-screen tests are separate from that checkpoint. No builder CAD is
changed here. Independent review and physical qualification remain necessary.
