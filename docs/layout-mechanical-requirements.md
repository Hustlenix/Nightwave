# PCB and enclosure requirements for builder authoring

2026-10-01. Requirements and candidate constraints only. No outline, placement,
routing, footprint or enclosure source has been authored/verified here. Builder
must select dimensions/stackup, author files and return them for source review.

Final-addendum gate: Bluetooth/display decisions must precede final layout/CAD.
Use their exact module/panel/cable/antenna envelope, not the historical OLED
window. Recalculate pack/charging/thermal space from expanded engineering budgets.

BD-14 now retains S3 and selects BM83SM1-00TA: reserve its 32 x 15 x 2.5 mm
module envelope plus manufacturer land pattern, antenna exclusion and UART/
reset/provisioning access. Both RF modules need their own reviewed antenna
space, kept away from pack/speaker/metal and each other; nominal body area is
not the RF envelope. BD-15 selects Waveshare 24382 non-touch: 31.5x39 mm board,
27.972x32.634 mm active area with maker R5 corner description. Thickness,
mount-hole datum, glass projection, MX1.25 cable bend/mate and lens tolerance
require the exact supplied drawing. BD-16 selects TCA9535PWR TSSOP-24;
reserve its verified land pattern and input pull-ups, not a module-sized box.
The recalculated pack remains unselected. The old
OLED cutout and 6600 mAh pack dimensions below are historical examples only.

## Footprint verification register

| Candidate | Source/package fact | Source-file verification still required |
| --- | --- | --- |
| ESP32-S3-WROOM-1-N16R8 | Module 18x25.5x3.1 mm; manufacturer land pattern | Module-pad/GPIO mapping, center ground pad, antenna exclusion, height/tolerance |
| PCM5102APWR | TI PW TSSOP-20 | Exact pad pitch/body, pin 1, analog/digital grounds, model rotation |
| TPA6132A2RTER | TI RTE WQFN-16, 3x3 mm | Exposed pad, paste/vias, charge-pump pin mapping |
| MAX98360CEFB+T | ADI ordering table: 10-pin FC2QFN, NOT WLP | Corrected October 2023 OUTP/OUTN map; exact outline/land pattern, assembly support |
| BQ25185DLHR | TI DLH WSON-10 2.2x2 mm, unresolved choice | Thermal pad/paste, polarity, charger decision |
| BQ25628E alternative | TI RYK WQFN-18 2.5x3 mm, unselected | Full drawing/pad map, power components and assembly quote |
| TPS63802DLAR | TI DLA 10 numbered pins plus exposed pad | Copper/paste/thermal connection, switch pins, regulator-loop geometry |
| MAX17048G+T10 | ADI TDFN-EP 8, 2x2 mm; outline 21-0168, land pattern 90-0065 | VDD sense versus CELL, EP ground, pad numbering/model |
| USB4105-GF-A | GCT series page: 3.31 mm profile, 7.35 mm body length | Exact suffix/stake length/locating pegs drawing; board edge/mating envelope |
| Molex 104031-0811 | Manufacturer part page located | Actual sales drawing/pin table/detect switch/card insertion envelope |
| SJ-3503-SMT-TR | Indexed maker drawing identifies sleeve/tip/ring and two audio switches; full PDF returns 403 | Full drawing/pad map and physical continuity; no isolated digital detect assumed |
| BM83SM1-00TA | Selected 50-pad module, 32 x 15 x 2.5 mm | Exact land pattern, second antenna keep-out, test/provision access, assembly height |
| Waveshare 24382 non-touch TFT | Selected 31.5x39 mm board; 27.972x32.634 mm active area | Exact drawing/revision/thickness/mount/cable/mate and window tolerance |
| TCA9535PWR input expansion | Selected TSSOP-24 PW | Exact pin-1/pad pitch/land pattern and external pull-ups; no floating inputs |
| Switches, speaker/battery/display connectors | Exact choices not locked | Maker drawings, hole/pad numbering, cable/actuation/clearance envelopes |

Source links and electrical roles: [schematic requirements](schematic-requirements.md).
Body dimensions are not pad dimensions and a 3D model does not prove a footprint.
Package suffixes are engineering inputs, not interchangeable names.

For every actual footprint: compare top/bottom view, pin-1 landmark, pitch,
pad size, thermal pad net, solder-mask/paste, drills/slots, courtyard and height
against its dated manufacturer drawing. Save a dimensioned comparison and
source URL/revision. Inspect actual KiCad library and placed-instance overrides.
Third-party downloads remain unverified until compared. No register row is PASSED.

## Stackup and fabrication

Four layers are a candidate for continuous returns and compact power/audio
routing, not a builder selection. Candidate topology: top components/signals,
adjacent continuous GND, inner power/slow signals, bottom signals/ground. Two
layers require proving returns and fitting exact packages without risky necks.

[JLCPCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities) is a
comparison source, not a quotation or committed manufacturer. Choose a real
service/stackup and capture copper weights, thickness, drill/annular ring,
trace/space, edge clearance, mask/paste, via treatment and assembly capabilities.
Fine-pitch FC2QFN and any WLP option need package-specific assembly confirmation.
Do not equate advertised PCB minimum trace width with supported assembly.
Do not order, buy or export a fictional manufacturing-ready package.

## Placement and electrical constraints

[Espressif layout guidance](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html):
place module antenna toward/outside board edge where practical, maintain its
specified all-layer exclusion and account for enclosure/battery/speaker metal.
Recommended housing clearance around antenna is at least 15 mm. Native USB
uses continuous reference, 90 ohm differential ±10%, minimal transitions with
return vias. SDIO recommendations include 50 ohm ±10%, short routes and matched
CMD/DAT versus CLK. Use actual stackup impedance calculation, not guessed widths.

Keep converter/charger hot loops compact using each selected IC's layout
reference. Put input/return decoupling directly at pins; keep SW copper away
from DAC/headphone inputs, I2C and RF. Keep FB sense separate from noisy nodes.
Use a continuous ground return rather than blindly splitting analog/digital
grounds. Do not send switching/speaker supply return through headphone ground.
Keep DAC/amp analog traces short and symmetric where appropriate. Route I2S
over ground, review fanout capacitance and source damping after edge-rate data.

Route SPK_P/N together with short, adequately wide traces; neither is ground.
Account for DC/RMS and switched-current losses, connector contact resistance,
via/bottleneck resistance and speaker cable EMI. Keep headphone traces away
from that pair. Protection parts belong near exposed connectors with a short
return. Route input and battery power for worst-case validated current.

Builder width calculation example: copper resistivity assumed 1.72e-8 ohm-m,
50 mm long, 1 mm wide, 35 um thick trace gives 24.6 mohm. At 1.5 A this is 36.9 mV
drop and 55.3 mW per trace, excluding vias/connectors and temperature rise. A
similar return doubles path loss. This is NOT IPC thermal/current qualification.
Set allowed drop/temperature first, then size copper and validate physically.

Required checks: schematic/netlist consistency, no unconnected critical pins,
manufacturer DRC, plane continuity, power-loop review, RF keepout, test access,
mounting holes free of unintended copper and silkscreen polarity/revision.
Record every intentional waiver with reason; a zero DRC count is not a design
review. Final exports must be tied to the reviewed source commit.

## Mechanical dimensions and tolerances

Create a shared datum: PCB origin, top surface Z=0, port mating planes, display
active-area datum, button axes and mounting-hole coordinates. Record mm and
orientation. Builder chooses board outline from actual component/service
envelopes, not a generic pocket-size rectangle.

The historical, unselected 6600 mAh pack example is about 69x54x18 mm and 155 g; see
[supplier page](https://www.adafruit.com/product/353). Allow cable, connector,
retention and inspection space. Do not compress/pierce cells, use sharp bosses
against wraps or place a hot charger against the pack. Do not assume that an
estimated runtime justifies this size/weight or fixes the supplier-current issue.
The indexed [speaker maker datasheet](https://www.sameskydevices.com/product/resource/cms-28528n-l152.pdf)
lists diameter 28 mm, height 5.2 mm, weight 7.8 g and 750 Hz nominal resonance.
Full PDF access returned 403; wire/retention/tolerance drawing remains pending.
The base suffix has wire leads without connector; A and B specify different
mating housings. Do not budget a pre-attached connector for the base suffix.
BD-15's SKU is selected, but glass/cable/mate and mount-hole source dimensions
still need verification; never infer coordinates or thickness from a product photo.

Proposed fit starting points, NOT manufacturing guarantees: 0.3–0.5 mm clearance
per side for rigid printed sliding fits, 0.5 mm electrical part-to-wall reserve,
and 2 mm wall thickness. Builder must choose printer/material, run a tolerance
coupon and adjust; different processes require different values. Battery safety,
thermal, RF and moving-control clearances override these examples.

Use worst-case tolerances: minimum cavity minus maximum component size minus
retention/assembly reserve must stay positive. Include board thickness, solder
height, connector stake projection, print shrink/warp and fastener protrusion.
Do not call an overlap-free ideal STEP assembly a tolerance analysis.

## Retention, acoustics, controls and service

- PCB: actual screw/boss coordinates with insulating standoff height; no loose
  board. Choose screw/insert drawing, engagement and boss wall thickness before
  holes. Keep screws away from battery, traces and antenna keepout.
- Pack: removable cradle/strap or rated retention method with strain relief.
  Account for the selected pack's mass/inertial loads, no conductive abrasion and NTC contact.
- Speaker: secured flange/holder, gasket if appropriate, clear cone travel,
  isolated front/back paths. Choose acoustic cavity/grille based on actual
  speaker data and listening; no fabricated tuning or sound-pressure result.
- Display: secure mounts, glass not carrying enclosure screw force, aligned
  visible area with cable bend/connector removal access.
- Buttons: guide axes, rest preload, full switch travel, overtravel stop, return
  force and tolerance stack. Do not claim five plungers work from a render.
- Ports: plug-body/cable clearance, insertion/removal grip, USB/card/jack datum
  alignment, no side force on solder joints. Battery must remain serviceable.
- Assembly: documented order with screws/tool path, no trapped connectors,
  NTC/speaker wiring separated from switching nodes, no wire crushed by lid.
- Thermal: preserve charger/regulator heat paths and evaluate safe touch/cell
  temperatures later. Venting assumptions are not measured cooling performance.

Builder returns editable CAD plus electronics-inclusive STEP, dimensioned port
and mounting drawings, tolerance table, section views, interference results and
exploded assembly. Review starts with source dimensions and actual components.
STL alone is not an editable design. No final enclosure is authored here.
