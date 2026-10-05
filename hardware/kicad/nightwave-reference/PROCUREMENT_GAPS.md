# Procurement status - not a completed priced BOM

`reference-bom.csv` identifies PCB components separately from the off-board
battery, speaker, TFT, NTC and harness. Older `hardware/BOM.csv` candidates are
not a purchase list for this board.

## Corrected connector identities

- J2/J3/J6: JST S2B-PH-K-S PCB headers, not the attached battery/NTC/speaker.
- J7: JST SM08B-GHS-TB(LF)(SN) PCB header, not Waveshare 24382 itself. Mating
  housing GHR-08V-S and SSHL-002T-P0.2 contacts require a correctly wired cable.
- TP1-TP16: copper features, not purchased components.

The [JST PH specification](https://www.jst-mfg.com/product/pdf/eng/ePH.pdf)
rates the family at 2 A with AWG24. The battery's advertised current capability
does not raise the connector rating. Wire, crimp, connector, copper, fuse or
protection, and battery limits must all be considered. The
[JST GH specification](https://www.jst-mfg.com/product/pdf/eng/eGH.pdf)
defines the J7 mating system. Confirm exact ordering suffix and plating with
the distributor, and check harness polarity against the numbered schematic.

## Still required before procurement

- Exact resistor, capacitor, switch, NTC and service-header ordering codes.
- Ceramic capacitor effective capacitance at operating DC bias; voltage,
  dielectric, tolerance and temperature rating, not only nominal capacitance.
- NTC B curve/tolerance matched to the charger's TS network and safe insulated
  attachment to the protected pack; battery polarity and protection data.
- Harness housings, contacts, crimp tooling and wire sizes; display module
  cable pin order and board/header orientation.
- Current distributor prices, stock, minimum quantities, taxes and shipping;
  existing prices are incomplete reference snapshots, not a fresh quote.
- PCB stackup and assembly quote including 1.2 mm board, 0.4 mm-pitch charger,
  plated slots, and BM83 ground-land via-in-pad treatment.

No complete total or purchasing approval is claimed. Blank prices are unknown,
not zero. AI authorship and design-review approval must be reported honestly;
a successfully generated CAD archive is not grant acceptance.
