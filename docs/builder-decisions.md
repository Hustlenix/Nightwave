# Builder decisions

2026-10-01. Candidate comparisons below are AI-prepared research prompts, not
builder selections. The existing BOM's SELECTED labels are historical candidate
status, not proof of human authorship or final approval. Fill choices and reasons
only with the builder's actual account. Preserve alternatives and rejected ideas.
Link the real commit/files and genuine journal entry when available. Do not
invent hours. Verify genuine Hackatime tracking before a long authoring session;
no secret/key inspection is required.

## BD-01 MCU module and service interface

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: ESP32-S3-WROOM-1-N16R8 versus smaller flash; native USB versus UART service pads (research candidates; builder may add/reject).
- Datasheet/reference: Espressif module datasheet and schematic checklist.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Module availability; reserved PSRAM pins; recovery when firmware is broken.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-02 USB source-current policy

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: Fixed conservative sink; advertised Type-C current detector; legacy-host enumeration policy (research candidates; builder may add/reject).
- Datasheet/reference: TUSB320 datasheet; USB-C requirements in schematic-requirements.md.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Default/dead-battery current; changing advertisement; powered-off backfeed.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-03 Charger and recharge target

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: BQ25185 linear versus BQ25628E switching power path; battery/profile changes (research candidates; builder may add/reject).
- Datasheet/reference: power-options.md and linked TI datasheets.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Safety timer; reset defaults; enclosure thermal behavior; weak source.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-04 Pack and temperature sensing

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: 6600 mAh protected pack versus documented higher-current pack; attached NTC versus pack-integrated NTC (research candidates; builder may add/reject).
- Datasheet/reference: Adafruit product 353; selected charger temperature circuit.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Supplier current-rating conflict; mass/size; NTC retention; charge limits.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-05 3.3 V converter and shutdown

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: TPS63802 with master disconnect versus another proven converter/power architecture (research candidates; builder may add/reject).
- Datasheet/reference: TPS63802 datasheet; power-options.md.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Low-cell transients; effective capacitance; off-state leakage; charging while off.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-06 Fuel-gauge and low-battery policy

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: MAX17048; validated voltage/SOC policy; interrupt or polling (research candidates; builder may add/reject).
- Datasheet/reference: MAX17048/MAX17049 datasheet.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: VDD sense wiring; model accuracy; bus backfeed; graceful shutdown.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-07 Storage and display

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: Molex 104031-0811; exact SH1106 module versus documented SSD1306 module (research candidates; builder may add/reject).
- Datasheet/reference: Molex drawing; actual display vendor drawing required.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Connector pin numbering; DAT pull-ups; module pin order and mounts.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-08 Controls and headphone detection

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: Five momentary buttons; isolated insertion contact versus separate detector (research candidates; builder may add/reject).
- Datasheet/reference: schematic-requirements.md; actual jack drawing required.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Audio-contact bias; detect polarity; ESD; button travel.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-09 Headphone signal chain

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: PCM5102A plus TPA6132A2; gain/attenuation options and output ceiling (research candidates; builder may add/reject).
- Datasheet/reference: TI DAC/headphone-amp datasheets; engineering calculations.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Clipping; hearing exposure; input impedance; pop sequencing; output load.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-10 Speaker path and assembly

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: MAX98360CEFB+T 10-pin FC2QFN versus CENL+T 9-ball WLP or compatible alternative; fixed gain and I2S mode (research candidates; builder may add/reject).
- Datasheet/reference: MAX98360 family datasheet.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Exact package assembly availability; DAI mode wiring; corrected output pin map; BTL output isolation; peak power.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-11 Connectors, protection and test access

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: USB4105; keyed battery/speaker connectors; selected ESD devices and test points (research candidates; builder may add/reject).
- Datasheet/reference: Each exact manufacturer drawing required.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Battery polarity; current rating; clamp voltage; service access.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-12 PCB manufacturing and placement

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: Manufacturer/stackup, trace rules, assembly side, outline and mounting (research candidates; builder may add/reject).
- Datasheet/reference: layout-mechanical-requirements.md; chosen fab rules.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: RF keepout; switching noise; trace drop; DRC; component sourcing.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.

## BD-13 Enclosure and acoustic design

- Decision: PENDING BUILDER INPUT.
- Alternatives considered: Battery/speaker placement, mounting/retention, screws, port datums, cavity (research candidates; builder may add/reject).
- Datasheet/reference: layout-mechanical-requirements.md; actual component dimensions.
- Reason I selected it: PENDING BUILDER INPUT; do not AI-fill as a personal account.
- Risks/tradeoffs: Fit tolerances; sound leakage; thermal clearance; serviceability.
- Validation still required: datasheet trace, actual source review, relevant ERC/DRC/fit and physical tests.
- Builder-authored files/commit and date: PENDING.


