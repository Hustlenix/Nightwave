# Nightwave KiCad starter

This is an **AI-assisted starter structure**, not a complete schematic, PCB, or
reviewed electrical design. It deliberately contains no finished circuits,
component values, footprints, GPIO assignments, board outline, or routing. The
builder must materially author and understand those items and disclose AI use
where the program requires it.

## Open it

1. Close the separate `Nightwave-Practice` window if it is still open.
2. Open `Nightwave.kicad_pro` from this folder.
3. Open the Schematic Editor.
4. On the root sheet, double-click one of the six large blocks to open it.

## Work in this order

1. `01_power_charging.kicad_sch`
2. `02_mcu_storage.kicad_sch`
3. `03_local_audio.kicad_sch`
4. `04_display_controls.kicad_sch`
5. `05_bluetooth.kicad_sch`
6. `06_connectors_test.kicad_sch`
7. Run ERC, resolve or explain every finding, then update the PCB from the
   schematic. Do not route the blank PCB before the schematic and footprints
   have been reviewed.

## Already selected

- MCU: ESP32-S3-WROOM-1-N16R8.
- Bluetooth architecture: ESP32-S3 plus BM83SM1-00TA.
- Display: Waveshare 24382 non-touch, 240x280 ST7789V2.
- Input expansion: TCA9535PWR, used as an input-only expander.

## Still open — do not guess

- Final combined numbered GPIO map, TCA9535 ports/address/IRQ, and BM83 clock
  requirements.
- Charger, protected battery pack, BM83 supply rail, shutdown/wake circuit, and
  their support components.
- Exact audio-device suffixes/footprints, headphone-detect circuit, connectors,
  switches, ESD/protection parts, and mechanical constraints.

Use `docs/schematic-requirements.md`, `docs/pin-budget.md`, `hardware/BOM.csv`,
and the exact manufacturer datasheets while drawing. Return one completed sheet
at a time for review before moving to PCB placement and routing.
