# Component research follow-up

Checked 2026-10-01. Proposals only, not builder selections, final procurement
or source-file footprint passes. Existing BOM labels/prices are historical
candidate inputs. The $55.97 subtotal stays unchanged; the examples below are
not silently added to a funding total. Final quantity/cost depends on the real
builder schematic and chosen fab/assembly/mechanical process.

## Display alternative with documented interface and dimensions

[Adafruit product 938](https://www.adafruit.com/product/938) is a 1.3-inch
128x64 SSD1306 module, I2C default with optional reset, address selectable
0x3c/0x3d, compatible in principle with the firmware's SSD1306 option (not
physically tested). Listed $19.95 and out of stock at check time. This is a retail
product ID, not an invented bare-panel manufacturer MPN. Board dimensions are
35.6x33x6.2 mm, mounting pitch 30.5x28 mm, holes 2.5 mm; confirm received revision.
Maker quotes approximately 25–40 mA depending on usage. It requires reviewing
the current model's 5 mA OLED average assumption, duty cycle and transient budget.
Documented source beats an unverified pinout, but out-of-stock availability and
module bulk/cost are tradeoffs. Do not replace the builder's intended module
without their decision. Never use an old revision's mounting pattern blindly.

## Passive reference candidates, not a complete passive BOM

[TPS63802 reference table 10-6](https://www.ti.com/lit/ds/symlink/tps63802.pdf)
lists Coilcraft XFL4015-471ME (0.47 uH), Murata GRM188R60J106ME84 (10 uF) and
GRM188R60J226MEA0 (22 uF) as application-characterization components. These are
exact research candidates, not automatically approved substitutions. Obtain
current maker drawings/derating curves, lifecycle, stock and assembly quotes.
Nominal capacitance and example inductor current do not establish adequacy at
the chosen bias/temperature/transients. Choose resistor tolerance/package for
divider and current-limit error budget. Passives count comes from actual sheets.

## Connector and assembly issues

- [GCT USB4105](https://gct.co/connector/usb4105) includes distinct shell stake
  lengths. Capture the exact purchasable suffix/drawing and quoted quantity.
  Do not use a 060 model to prove fit for a different stake length.
- [Same Sky jack drawing](https://www.sameskydevices.com/product/resource/sj-3503-smt-tr.pdf)
  indexed pin table confirms tip/ring audio switches, not isolated digital detect.
  Full fetch is 403. Resolve detector topology or choose a documented isolated
  detect jack. No actual continuity/footprint check has passed.
- [Same Sky speaker datasheet](https://www.sameskydevices.com/product/resource/cms-28528n-l152.pdf)
  indexed text distinguishes base wire-lead part from A/B connector variants.
  Full fetch is 403; reserve cable strain relief and obtain the mechanical drawing.
- MAX98360CEFB+T is the 10-pin FC2QFN. CENL+T is the 9-ball WLP. The
  [manufacturer ordering/pin table](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98360A-MAX98360D.pdf)
  and corrected output locations control selection; review actual library files.

## Before a priced BOM can be final

For every builder-chosen component capture reference designators, exact MPN,
quantity, package, datasheet revision, validated footprint/model reference,
supplier URL, unit/extended price, currency/date/quantity, stock and substitutions.
Include NTC, source controller, master disconnect, all passives/ESD, connectors
and mates, switches, fasteners/retention, PCB/assembly, print/material, shipping
and tax. Mark unknown prices unavailable, never zero. Distinguish raw prototype
cost, final per-unit cost and minimum-order cash outlay. Requote before approval.
No purchase, funded budget, or human selection is asserted in this research.
