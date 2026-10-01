# Power design constraints and unresolved decisions

Checked 2026-10-01. Model inputs: [power-budget.md](power-budget.md).
This research is AI-assisted; it is not a builder-authored final circuit.

Mentor continuation: see [power-options.md](power-options.md) for the researched
BQ25628E switching alternative, minimum safety-time screening and source/load
balance. It is not selected. Builder must resolve reset/watchdog/default-source
behavior, cell sensing/NTC and shutdown before locking the power circuit.

## Datasheet facts checked

- [BQ25185 Rev. B](https://www.ti.com/lit/ds/symlink/bq25185.pdf): standard
  4.2 V/500 mA input setting uses 18 kohm ILIM/VSET; 1 kohm ISET gives 300 mA.
  TS is designed for 10 kohm, beta(25/85)=3435 K NTC. August 2026 revision
  changes nominal fast-charge safety time from 720 to 360 minutes. It supplies
  regulated SYS in adapter mode and supports load priority/supplement mode.
- [TPS63802 Rev. D](https://www.ti.com/lit/ds/symlink/tps63802.pdf): 3.3 V
  reference circuit uses 511 kohm/91 kohm feedback, 0.47 uH inductor, 10 uF input
  and 22 uF nominal output. Effective capacitance, saturation current and layout
  require separate verification. Its package has ten numbered pins plus an
  exposed pad; the old BOM's unqualified "11-pin" description needs correction.
- [MAX98360 family](https://www.analog.com/en/products/max98360a.html): supports
  supply 2.5–5.5 V and mono/averaged-stereo modes. Typical 92% efficiency is
  condition-specific, not an across-range guaranteed minimum.

## Charging conflict — must resolve before component lock

Even an ideal 6600 mAh / 300 mA constant-current fill takes 22 hours, before
taper and playback. BQ25185's six-hour nominal safety timer makes the preliminary
low-current/large-pack combination unsuitable for a promised uninterrupted
full recharge. Dynamic timer behavior must not be assumed to fix idle charging.
Do not disable/restart the timer to conceal this conflict.

[BQ24074 Rev. N](https://www.ti.com/lit/ds/symlink/bq24074.pdf) is a comparison
candidate with programmable timers, but its supported TMR resistance range is
18–72 kohm and timer factor is 36–60 s/kohm. At 72 kohm the fast-charge window
can be only 7.2 h minimum. A guessed 150 kohm "30-hour timer" is outside the
documented range and is not a solution. A different charger with a legitimately
supported longer safety window or a proven higher-current, thermally suitable
charging design is needed. No charger substitution is locked yet.

## Required electrical decisions

1. USB-C: independent CC Rd, VBUS protection, connector shield strategy and
   source-current detection/default policy. Merely fitting Rd does not authorize
   1.1 A from every source. Native USB data/500 mA host eligibility is not yet
   implemented. Define allowed power adapters or implement current negotiation.
2. Preserve charger power path, battery protection and temperature sensing.
   The two-wire pack needs a mechanically retained, electrically insulated
   external NTC touching the cell assembly; do not bypass it with a fixed resistor
   in the final design. Exact sensor/threshold tolerance remains a design task.
3. SYS feeds speaker branch; TPS63802 provides 3.3 V. A rated master load
   disconnect must isolate both branches while leaving charging possible.
   ESP32 enable pins alone cannot shut down the SYS-fed speaker rail.
4. Fuel gauge attaches to protected battery rail; follow its cell-sense and
   3.3 V bus-voltage limits. Firmware currently displays `BAT ?` rather than
   pretending to have a reading. Low-battery shutdown needs final hardware data.
5. Select exact ESD devices, switch, inductor, capacitor effective values, NTC,
   connectors and passives in the builder schematic. Verify pin maps and
   thermal-pad drawings before any PCB or assembly cost is treated as final.

## Physical validation later

Use current-limited USB/bench power first. Check shorts/polarity and disabled
audio enables before a human connects the pack. Measure 3.3 V startup, SD/CPU
pulses, both audio paths, charge-while-play noise, charger/cell temperature,
source limits, thermistor faults, shutdown leakage and full-night runtime.
All are **PENDING PHYSICAL VALIDATION**, not digital completion evidence.
