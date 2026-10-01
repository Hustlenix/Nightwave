# Power options for the builder's decision

2026-10-01. No substitute is selected or ordered. Source/model research is
independent of builder authoring. Final values and thermal validation remain
pending. Run `rtk python tools/engineering_calculations.py` for reproducible
examples. Existing runtime model is tools/power_budget.py.

## Current conflict

6600 mAh at 300 mA needs 22 h ideal full-capacity CC-equivalent time, without
taper/load reduction. The [BQ25185 Rev. B](https://www.ti.com/lit/ds/symlink/bq25185.pdf)
nominal six-hour fast-charge timer is not compatible with promising that recharge.
Even 1 A gives 6.6 h before taper. This is a screening calculation, not a simulated
charge curve: fast-charge timer counts its stage, not every minute of a cycle.
Precharge capacity, initial SOC and dynamic timer behavior must be checked, but
must not be used to conceal a clearly unsuitable recharge expectation. Linear
charging at 5 V, 3.2 V cell and 1 A dissipates roughly 1.8 W plus path loss.
No PCB thermal resistance is established, so no junction temperature is claimed.

## Investigated alternative: BQ25628E

[TI Rev. C datasheet](https://www.ti.com/lit/ds/symlink/bq25628e.pdf): I2C switching
power-path charger, RYK 18-pin 2.5x3 mm WQFN. Fast timer is nominal 14.5/28 h,
minimum 10.5/21 h. POR charge current is 320 mA; watchdog expiration changes
charge current. CE must be defined. ILIM hardware and IINDPM register both
constrain input; retain an independently safe startup limit. Scale factor is
2250–2750 A-ohm at its specified condition. An ILIM RC filter is required below
400 mA (1.2 kohm/330 nF). TS_BIAS/TS monitor a thermistor, QON controls supported
wake/reset, BAT/SYS are distinct power nodes, SW/BTST/REGN require the reference
power components. There is no D+/D- detection on the E variant. Safety timers
stay enabled. Inspect sections 6/8.3/8.6/9/11 before circuit authoring.

### Calculated screening scenarios

Assume 6.6 Ah, a 25% allowance on CC-equivalent duration (not a proved CV bound)
and charge-current minimum 95% of setting. At 1 A: 8.68 h versus 10.5 h minimum
timer; at 0.5 A: 17.37 h, exceeding the short timer but below the long minimum.
At default 0.32 A: 27.14 h, exceeding both minimum timers. Passing this screening
does not prove termination or validate the pack. Playback and input limits can
reduce charge current further. A host configuration/readback/watchdog-failure
policy and weak-source user behavior are required, not merely a longer timer.

At 5 V/0.5 A input, assumed 85% input-to-system/battery efficiency and calculated
1.4168 W system load leave about 0.708 W, or 169 mA at a 4.2 V cell. A 1 A charge
setting cannot create unavailable source power. At a 1.35 A actual input cap
(90% of a nominal 1.5 A setting), the same arithmetic leaves about 1.029 A charge
headroom. Applying 20% load margin reduces it to 0.961 A: tight, not validated.
These are simple power balances, not guaranteed converter performance. Turn down
charging for source/load conditions rather than assuming full-rate charge while
playing. Calculate at real input sag and actual charger efficiency later.

### USB current detection is a separate decision

[TUSB320 Rev. F](https://www.ti.com/lit/ds/symlink/tusb320.pdf) can report Type-C
advertised current but is only a comparison candidate. Its GPIO outputs retain
the initial advertisement; the I2C refresh procedure is important if source
capability changes. Configure sink-only, define dead-battery behavior and avoid
backfeeding when VDD is absent. Do not add passive Rd in parallel with its
internal Rd without reviewing the chosen implementation. It is not USB-PD.
Native USB host enumeration/legacy charging eligibility is still separate.

Decision options: conservative USB draw with deliberately limited charging;
current-aware Type-C input with safe reset/default behavior; another documented
charger/source architecture. Unknown sources never inherit the highest limit.
Safe startup must work without firmware. Include controller/regulator startup
power and connector/TVS inrush in that budget.

## Battery, rail and enclosure coupling

[Adafruit 353](https://www.adafruit.com/product/353) is a protected 1S 6600 mAh
candidate, about 69x54x18 mm and 155 g. Supplier's recommended continuous current
below 1.3 A conflicts with a higher technical rating. Use the lower recommendation
until supplier clarifies. No supplier clarification is claimed. Existing combined
low-cell peak model is approximately 1.35 A, so pack/profile selection is open.
The pack is two-wire: charger temperature sensing needs a retained insulated
sensor against the pack. Do not modify cell wraps or solder onto cells.

[TPS63802 reference](https://www.ti.com/lit/ds/symlink/tps63802.pdf) supports the
existing buck-boost comparison. Keep 3.3 V transient validation, output effective
capacitance and master disconnect open. A new charger must not leave speaker SYS
powered during supposed shutdown. Decide charging-while-off and battery wake
behavior before designing the latch or relying on charger ship mode.

## Decision and validation checklist

- Builder records pack, charger, authorized sources and expected recharge time.
- Exact MPN/package/drawing, purchasability and assembly quote are captured.
- Derive ILIM maximum with resistor tolerance AND scale-factor maximum. A nominal
  formula alone is insufficient. Confirm that the datasheet tolerance applies at
  the intended current; do not extrapolate the 1.6 A condition to 100 mA blindly.
- Check POR, watchdog, CE, firmware absent, NTC open/short and timer fault.
- Calculate current/thermal budget with minimum source and maximum system load.
- Physically verify current, temperature, timing, taper/termination, audio noise,
  charging while off and shutdown leakage later under human-controlled safe tests.
- No circuit/BOM lock or charger firmware driver until the builder chooses the
  interface and fail-safe hardware. Independent software remains testable now.
