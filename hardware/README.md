# Hardware inventories and authority

`reference-procurement.csv` is the current procurement working inventory:
it is generated from the actual reference circuit BOM plus manufacturer-checked
resistor MPN selections in `reference-resistor-selections.json`. Run
`tools/build_reference_procurement.py` after changing either input. It resolves
49 resistor positions without changing their values/footprints. Zero-ohm current
ratings, capacitor effective capacitance, other missing MPNs and complete quotes
remain open. Empty prices are unknown, not zero; retained older prices are marked
historical. This file is not a completed purchasing list or an approved order.

Current product choices: ESP32-S3, BM83SM1-00TA Bluetooth output, Waveshare 24382
TFT and TCA9535PWR controls. Speaker, wired headphones, microSD, synchronized
lyrics and rechargeable power remain in scope.

Use `kicad/nightwave-reference/reference-bom.csv` for the **actual AI-authored
reference schematic inventory** and `design-manifest.json` for its pin/net
assignments. Missing prices and `PENDING_SELECTION` parts are unresolved, not
free or purchase-ready. The reference includes BQ25628E, dual TPS63802,
TPS22965 and MAX17048; battery approval and safety review remain open.

`BOM.csv` is the older research/candidate cost inventory, not a purchasing list
for that PCB. Its BQ25185 and pending BT rail entries do not describe the
reference circuit. `prototype-BOM.csv` and `prototype-wiring.csv` describe the
legacy USB-powered OLED/direct-button bench, not the selected product.

Do not combine these inventories to produce a purported complete price or
substitute their wiring maps. See `../docs/design-completion-status.md` for
the remaining design, procurement, enclosure and independent-review gates.
