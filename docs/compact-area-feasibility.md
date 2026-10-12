# Compact area feasibility — 2026-10-12

Engineering screening only. No component placement, routing or enclosure CAD.
Conclusion: retain 90 x 60 mm as the first builder study envelope. It is plausible,
not proven. A 95 x 60 mm contingency needs an actual courtyard/routing conflict,
not reuse of the 110 x 100 mm reference. Do not order either envelope from this study.

## Area ledger

DATASHEET body dimensions: ESP32-S3-WROOM-1 18 x 25.5 mm, BM83 15 x 32 mm.
Their combined body area is CALCULATED 939 mm2. This excludes pads, antenna
exclusions and nearby components. Sources:
[Espressif module](https://www.espressif.com/en/module/esp32-s3-wroom-1-en),
[Microchip module](https://www.microchip.com/en-us/product/bm83).

All remaining area values below are ESTIMATED planning allowances, not measured
courtyards or an exact packing calculation. Allowances include local passives.

| Function | Low allowance mm2 | High allowance mm2 |
| --- | ---: | ---: |
| Two module bodies (calculated) | 939 | 939 |
| Module support/land-pattern allowance | 200 | 350 |
| USB-C, protection, CC and always-on supply | 300 | 450 |
| Charger, inductors, two regulators, switching/support | 550 | 800 |
| SD socket and support | 220 | 300 |
| DAC, headphone amplifier, speaker amplifier and filters | 350 | 500 |
| Headphone jack | 150 | 220 |
| TCA9535, controls and display connection | 200 | 300 |
| Battery/speaker/service connectors and test access | 200 | 300 |
| Mounting/edge manufacturing allowance | 200 | 300 |
| **Subtotal before routing and RF exclusions** | **3309** | **4459** |

CALCULATED 90 x 60 = 5400 mm2, leaving 941..2091 mm2 (17.4..38.7%) for
unaccounted routing and RF constraints. This is NOT a free-space map: RF
exclusions have shape and edge constraints, and usable ground cannot be counted
as an arbitrary spare rectangle. High-case occupancy is tight. Stacking a TFT
above parts does not remove its metal/RF or vertical-clearance requirements.
An extra 5 mm length adds only 300 mm2; it cannot repair an unsuitable battery stack.

## Edge and separation requirements

- Give both module antennas access to a nonmetal enclosure edge. Do not put
  battery foil, speaker magnet, TFT metal, connector shells or screws in their
  exclusion volumes. Do not invent a universal inter-antenna distance.
- BM83 manufacturer recommends at least 15 mm clearance from external metal;
  preserve its distinct module ground region and antenna no-copper region.
  [Manufacturer placement guidance](https://onlinedocs.microchip.com/oxy/GUID-414904F5-364E-4377-B959-9226AD29D6A9-en-US-8/GUID-495EA464-310A-41EC-A892-5E46727B10FC.html).
- USB-C needs plug-shell, cable, ESD-return and insertion-force access at an edge.
  microSD needs insertion/ejection clearance; headphone access includes the
  actual plug body, not only the jack footprint. No assigned edge coordinates.
- Keep switching hot loops, inductors and class-D output return away from DAC
  and headphone signal routing. Preserve a continuous reference plane rather
  than splitting grounds indiscriminately. Check thermal copper before shrinking.
- Keep battery and speaker connector current paths short and broad, with strain
  relief outside component courtyards. Service pads need physical probe access.

## Device envelope

CALCULATED illustrative internal space for 2 mm walls is 106 x 66 x 21 mm.
A 90 x 60 board leaves 16 mm length and 6 mm width TOTAL for bosses, tolerance,
ports and cables; this is not clearance on each side. An assumed 1.2 mm PCB plus
a 10 mm battery leaves 9.8 mm vertical space for both component faces, display,
insulation, swelling clearance and retention. Every allowance must be checked
against selected vendor drawings before the builder creates the final stack.

Main feasibility risk is the metal battery/display/speaker envelope around both
antennas, not the bare IC area. Exact pack drawing and swelling specification,
speaker depth and TFT connector/cable height remain prerequisites to fit proof.
Do not compress a pouch, omit protection, or reduce RF clearance to meet a target.

## Builder acceptance evidence eventually required

Dimensioned courtyard/keepout occupancy, Z-stack with tolerance and cell swelling,
connector mating access, RF metal-exclusion review, complete hot-loop/thermal
review, assembly accessibility and fabrication DRC. This study does not replace
any of those or authorize the unfinished power circuit.
