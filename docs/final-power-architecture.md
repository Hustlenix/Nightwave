# Power architecture closure record

2026-10-06. Architecture direction selected by builder; detailed circuit NOT
released. No CAD generated or changed. DATASHEET denotes published limits,
ESTIMATED planning assumptions, CALCULATED arithmetic; MEASURED: none.

## Selected direction and alternatives

Builder approved development of 5 V-only USB-C sink, TUSB320LAI current detection,
protected reverse-blocked input and BQ25628E switching power path with charging
disabled by default at reset. This is NOT approval of the old CE-tied-low circuit.

| Candidate | Engineering comparison | Disposition |
| --- | --- | --- |
| BQ25628ERYKR | Switching NVDC power path; programmable current and status; needs inductor, careful hot-loop layout and fail-safe host policy | Selected direction, detailed implementation open |
| BQ24074RGTR | Standalone linear power path, simpler control; enclosed dissipation rises with charge current | Not preferred for compact charge-while-play |
| BQ25185DLHR | Standalone linear 1 A-class candidate, small package; slower cool charging or more thermal burden | Not preferred |
| MCP73871-2CCI/ML | Linear load sharing with USB low/high current control; larger support/thermal budget | Useful simpler alternative, not selected |

CALCULATED linear charger-only heat at 5 V input, 3.7 V cell and 0.96 A charge:
(5-3.7)*0.96 = 1.248 W, excluding system-path loss. Switching is recommended
for heat reduction, not because the reference already used it. Actual switching
efficiency depends on operating point and layout; model assumes 85%, not a guarantee.

Sources: [BQ25628E](https://www.ti.com/lit/ds/symlink/bq25628e.pdf),
[BQ24074](https://www.ti.com/lit/ds/symlink/bq24074.pdf),
[BQ25185](https://www.ti.com/lit/ds/symlink/bq25185.pdf),
[MCP73871](https://www.microchip.com/content/dam/mchp/documents/APID/ProductDocuments/DataSheets/MCP73871-Data-Sheet-DS20002090F.pdf).

## Required power tree

USB-C VBUS -> connector protection/inrush/reverse blocking -> BQ25628E VBUS.
Protected 1S pack <-> BAT; SYS -> controlled system switch -> 3.3 V logic/audio
regulation and separately controlled 3.6 V Bluetooth regulation. Speaker uses
reviewed SYS-derived supply, never an unqualified 5 V boost assumption.
MAX17048 senses the cell independently of the switched application rails.

## USB and startup requirements

2026-10-07 builder decision: charging and dead-battery startup require a Type-C
source advertising at least 1.5 A. Default-current ports are for programming
while running from a charged battery, NOT charging. No USB enumeration event
may override this restriction. A 3 A advertisement still uses the 1.5 A design
ceiling. This supersedes the former default-USB charging proposal below.

Hardware truth table (requirements, not a released circuit):

| TUSB320LAI GPIO OUT1 / OUT2 | Main VBUS path | CE |
| --- | --- | --- |
| H/H unattached | Off | High (disabled) |
| H/L default attached | Off | High |
| L/H 1.5 A, protection valid | Eligible for controlled ramp | High until configuration and NTC valid |
| L/L 3 A, protection valid | Same 1.5 A ceiling | Same |
| Controller unpowered, fault, reset or indeterminate | Off | High |

Use GPIO-mode detection (ADDR floating), not an I2C-only software permission.
OUT1 low identifies either elevated advertisement; qualify it with power-good,
sink-only configuration and non-accessory state. Pull states and inversion must
make unpowered/high-impedance inputs disable the main path. CC logic and its
small supply are upstream of the gate; their startup consumption/inrush must
remain within default-source rules. Do not feed their supply from gated SYS:
that creates a dead-battery startup dependency loop. OUT3 accessory rejection,
controller reset behavior and propagation time need schematic-level review.

Software charge policy is implemented and host-tested in `charge_policy.h`:
default unqualified/off, <=100 ms freshness, 1400 mA programmed planning limit,
960 mA maximum charge request rounded down in 40 mA steps, pack/current/NTC/
readback qualification, and latched timer/watchdog/thermal faults. Rearm requires
fresh detached fault-free state. It is deliberately not connected to charging
writes on the unsafe historical reference board. The upstream hardware gate
must react without waiting for this policy; CE alone is not input isolation.

Correction: BQ25628E Rev C table 8-9 specifies 20 mA per IINDPM code and 3200 mA
POR. The previous telemetry decoder used 10 mA/code and reported half the actual
limit. Code, full-range regression and reference power screen are corrected.
This did not change hardware current limits; telemetry is still read-only.

TUSB320LAIRWBR is the proposed exact CC controller. Configure sink-only; its
internal Rd replaces, rather than parallels, discrete CC pull-downs. This is not
the older TUSB320: live current-advertisement behavior differs. Provide dead-cell
CC operation and avoid back-driving its unpowered pins. No PD voltages requested.
[TI TUSB320LAI datasheet](https://www.ti.com/lit/ds/symlink/tusb320lai.pdf).

All limits need tolerance/headroom for CC/protection current outside the charger
measurement path. Nominal ILIM arithmetic is not a worst-case current guarantee.

Remaining implementation: exact always-on supply/gate/protection network and
tolerance/short-circuit analysis. The approved policy removes default-current
charging, but does not establish component selection or electrical qualification.

Protection candidate TPS25947 family offers reverse-blocking/inrush/OVP, but
exact suffix, clamp/threshold tolerance, low-current limiting range and coordinated
VBUS TVS remain unselected. A high-current eFuse is not automatically a compliant
100 mA startup limiter. Candidate TPD2E2U06DCKR for exposed USB data needs clamp,
capacitance and placement verification; CC needs separate ESD/short-to-VBUS review.
No final protection BOM claim is made.
[eFuse](https://www.ti.com/lit/ds/symlink/tps25947.pdf),
[ESD](https://www.ti.com/lit/ds/symlink/tpd2e2u06.pdf).

## Charger, thermal and fault policy

Proposed normal charge target 960 mA (ESTIMATED policy), limited further by the
eventual pack rating, source permission, available system power and temperature.
Keep safety timers enabled. DATASHEET table default: 14.5 h nominal, 10.5 h minimum,
15.5 h maximum; conflicting 12 h prose is recorded, not silently used. Dynamic
timer/termination behavior needs full stage review. Do not reset timer repeatedly
to force completion on a weak source.

CE requires a hardware default-off path surviving reset, watchdog, unpowered MCU
and I2C faults. Thermal safety cannot depend solely on firmware. NTC must be an
exact curve/tolerance, attached to the selected pack with open/short response
verified. The reference 103AT network's 0..60 C mapping is not compatible by
default with a 0..45 C pack. Thresholds, resistor tolerances and reset register
values remain unresolved; never disable TS protection to pass startup.
See `ntc-reset-safety.md`: the simple 103AT pull-up/parallel-shunt solution fails
a documented tolerance sensitivity at reset thresholds. Nominal-only resistor
selection is rejected; independent hardware window/alternate curve work remains.

Charge-while-playing uses available input power first; charge reduces on overload,
then battery supplementation may occur. A plugged-in device can still discharge.
On source drop/detach, reduce allowed input without depending on slow UI tasks.
Dead-cell/absent-cell and poor-source boot must not oscillate indefinitely.

## Rails, shutdown and gauge

TPS63802DLAR remains a candidate for 3.3 V and 3.6 V regulation. DATASHEET 2 A
rating at 3.3 V for VIN>=2.3 V does not qualify the 3.6 V rail or every layout.
Verify inductance/saturation, effective DC-biased capacitors, current peaks and
thermal derating. Use 80/85% efficiency only as ESTIMATED screening inputs.
[TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf).

Low voltage: mute/stop playback, persist state once, stop SD access, then release
power hold; hardware supervisor/protection must cover brownout or wedged firmware.
Exact threshold/hysteresis and supervisor/latch parts remain pending rail/pack
review. Gauge SOC is telemetry, not the protection cutoff. Existing read-only
MAX17048/BQ25628E backend and shared-I2C owner are tested digitally; no charging
register writes or actual controlled-shutdown backend are yet qualified.

## Mode model and closure status

`tools/output_power_model.py` is the current exclusive-output model. Speaker,
wired and Bluetooth never stream simultaneously. BT-off assumptions require
verified shutdown/isolation; otherwise retained-radio overhead is a separate
sensitivity, not simultaneous Bluetooth audio. See output-power-estimates.md.

Architecture choice COMPLETE. Reset input gating, protection coordination,
NTC/reset safety, final support parts and closed-loop control remain IN PROGRESS.
These are agent engineering tasks, not falsely labelled blocked on the builder.
