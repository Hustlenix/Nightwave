# Battery candidate screening - 2026-10-06

No selected pack; no measured runtime. Candidate capacity range to investigate:
5000-6000 mAh protected1S flat pouch, conditional on lower average power and fit.
This range does NOT yet meet eight-hour speaker playback in the present model.

| Candidate | Published capacity and dimensions | Sourcing / qualification |
| --- | --- | --- |
| Soldered333287,606090 JST | 4000 mAh; current product90x60x6 mm; documentation81.1x50x7.7 mm | EUR11.95, sold out, lead estimate6 weeks; conflicting dimensions require revision-specific drawing |
| Soldered333288,105575 JST | 5000 mAh; current product75x55x10 mm; documentation67.4x54.6x10.1 mm | EUR14.95, sold out, lead estimate6 weeks; no verified India delivery |
| Soldered333289 | 6000 mAh; documentation90.2x60x9 mm | Product link404 during check; current model, cost and stock unverified |

Sources: [4000 product](https://soldered.com/products/li-po-battery-4000mah-3-7v-606090-jst),
[5000 product](https://soldered.com/products/li-po-battery-5000mah-3-7v-105575-jst),
[manufacturer family table](https://docs.soldered.com/li-ion-battery/hardware/).
Family documentation specifies protection and1C discharge, but this does not
qualify the connector/harness at4-6 A. All use JST-PH2 mm; polarity warning is
explicit. Two-wire interface provides no NTC lead. Exact mass, pack-level max
continuous current and NTC mounting approval remain unknown. Do not invent them.
Manufacturer says no air shipping for these products; landing in India unresolved.

CALCULATED typical speaker/wired/BT hours respectively from output_power_model.py:
4000 mAh4.52/5.41/4.80;5000 mAh5.65/6.76/5.99;6000 mAh6.78/8.11/7.19.
These use ESTIMATED loads and conservative capacity/aging/margin factors.
Speaker and BT are exclusive. Remaining current/thermal/fit gaps mean none is
purchase-approved or a guarantee of overnight operation.

25 mm outer thickness with illustrative2 mm walls and1.2 mm PCB leaves19.8 mm
before parts/pack/display/insulation/tolerances. A10 mm pack consumes about half.
Place no screw, sharp solder joint or compression load against pouch; supplier
swelling allowance must be obtained. A90x60 pack nearly consumes the PCB footprint
and conflicts with antenna clearances unless its actual envelope is accounted for.
No enclosure/placement model has been generated. Next sourcing step is an exact
available India-deliverable protected pack drawing and ratings, not choosing one
of these unresolved listings merely to fill the BOM.
