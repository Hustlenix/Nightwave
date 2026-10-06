# Selected-product power screening — 2026-10-03

## Reference-circuit override - 2026-10-06

Latest product-mode authority: [exclusive-output model](output-power-estimates.md)
and [power architecture](final-power-architecture.md). The retained-BT reference
screen below is a hardware-overhead sensitivity, not simultaneous audio output
or final compact battery sizing.

New compact requirements: [builder constraints](compact-builder-constraints.md).
`tools/compact_design_screen.py` calculates capacity-independent eight-hour
power ceilings. Neither 6,600 mAh nor 10,050 mAh is selected for the final device;
the reference model below is retained only as a load/charge sensitivity.

The reference-specific screening now lives in `tools/reference_power_budget.py`
and [its saved report](../hardware/reference-power-screen.json): 3.6 V BT rail,
10.05 Ah typical / 9.5 Ah minimum reference pack, retained BT load, regulator
losses, runtime sensitivities, reset charging and harness-current checks. This
supersedes the generic rail/pack assumptions below for the reference circuit.
It explicitly fails the existing timer, USB permission and peak harness gates.

This remains a load sensitivity model, **not the final reference-circuit power
model**. The AI-authored KiCad candidate now uses BQ25628E switching charging,
dual TPS63802 rails and TPS22965 switching. Its provisional Adafruit 5035 pack
is not a locked/approved battery. BQ25185 linear-charger analysis below is
historical and must not be used to configure or approve that reference board.
Existing 6.6 Ah examples are hypothetical, not the reference pack's runtime.
The reference uses 5.49 kohm ILIM (about 455 mA typical), but USB permission,
component tolerance, startup/suspend, watchdog defaults and full load balance
still need resolution. Neither the old calculations nor zero DRC authorize
connecting a battery. See [completion gates](design-completion-status.md).

Hardware decisions are now S3 + BM83SM1-00TA and Waveshare **24382**, non-touch
240x280 ST7789V2, plus builder-selected TCA9535PWR input expansion (BD-16).
Its IC and pull-up load are included only in the estimated 5 mA support allowance;
exact resistor/pressed-input/current and shared-bus measurements remain pending.
`tools/product_power_budget.py` is the current reproducible
screening model. The old OLED budget and hypothetical display scenarios are
historical, not current authority. **No battery, charger or rail circuit is locked.
No load, runtime or charge time below is measured.**

## Inputs and provenance

| Load | Typical screening input | Higher-load/peak screening | Basis |
| --- | ---: | ---: | --- |
| MCU/flash/PSRAM at 3.3 V | 200 mA playback; 80 mA awake idle | 300 mA practical; 500 mA transient allowance | ESTIMATED, not datasheet audio consumption |
| microSD at 3.3 V | 40 mA playback; 2 mA idle | 100 mA practical; 200 mA burst | ESTIMATED; card-specific measurements required |
| Selected 24382 display/backlight at 3.3 V | 90 mA continuously lit | 90 mA | Maker FAQ maximum; no assumed linear PWM scaling |
| PCM5102A + headphone path at 3.3 V | 25 + 3 mA retained quiescent load | Same allowance plus explicit headphone audio | ESTIMATED; muting does not prove power-off |
| Controls/gauge/pull-ups/support | 5 mA at 3.3 V; 25 mW cell-side allowance | Retained in all profiles | ESTIMATED, covers unresolved interface hardware, not final IC sizing |
| Speaker acoustic/electrical output | 0.25 W electrical RMS planning level | 1 W practical sensitivity; 2 W transient screen | ESTIMATED; volume percentage is not watts |
| Wired headphones electrical output | 0.02 W total planning level | User-adjustable bounded sensitivity | ESTIMATED; actual safe level/load pending |
| BM83 separate provisional 3.7 V rail model, pending qualification | 50 mA source/scanning | 100 mA allowance | ESTIMATED, not AT-source module rating |

[Waveshare FAQ/specifications](https://www.waveshare.com/wiki/1.69inch_LCD_Module)
reports 3.3 V/90 mA = **0.297 W** module input. Rechecked via indexed official
content on 2026-10-03; direct page fetch was refused. This is not our measurement.
The maker's touch variant has a different current and is not interchangeable.

Use 85% main/BT regulator and 80% output amplifier screening efficiency;
worst practical/peak use 80%/75%. These are assumptions, not guaranteed curves.
Battery nominal energy uses 3.7 V, 80% usable fraction, 80% aging fraction and
20% load margin. The separate fractions are intentionally conservative sensitivity,
not an assertion about a particular pack. Low-cell 3.2 V calculations are current
screening only, not a selected shutdown threshold.

## Calculated results from those assumptions

| Profile | Cell-equivalent W | Main/BT regulator loss W | Required Ah for 8 h | Hypothetical 6.6 Ah runtime |
| --- | ---: | ---: | ---: | ---: |
| Awake idle, display lit, BT off assumed | 0.821 | 0.119 | 3.328 | 15.87 h, not playback |
| Typical speaker, BT off assumed | 1.747 | 0.211 | 7.082 | 7.46 h |
| Typical speaker plus BT scanning | 1.964 | 0.244 | 7.964 | 6.63 h |
| Worst practical sensitivity, 1 W speaker/BT scanning | 3.978 | 0.524 | 16.128 | 3.27 h |
| Typical wired, BT off assumed | 1.459 | 0.211 | 5.916 | 8.92 h |
| Typical Bluetooth, local outputs muted but analog quiescent retained | 1.652 | 0.244 | 6.697 | 7.88 h |
| Conditional dimmed speaker + analog power-gating | 1.366 | 0.154 | 5.539 | 9.53 h |

The last row assumes **20 mA average display module current** and zero inactive
analog load. Neither dimming policy nor safe rail isolation exists/has been
qualified, so this is an optimization target, not the acceptance baseline.
BT-off rows likewise require verified shutdown/backfeed isolation; use scanning
sensitivity if that cannot be achieved. Silence/mute is not rail-off.

Transient screen: 823 mA main 3.3 V load; combined estimated input **6.549 W**,
about **2.047 A at 3.2 V** before the 20% margin. This is not a continuous runtime
profile. Pack/protection/connectors, power path, regulator derating, capacitance,
SD bursts and safe speaker headroom must tolerate the actual measured transients.
Speaker power itself is voltage/load limited; a 2 W label does not guarantee
2 W clean output over a discharging 1S rail.

Capacity sweep 2.5/4/5/6.6/8 Ah is printed by the tool. Do not select a 16 Ah
pack merely to sustain the worst sensitivity or present 6.6 Ah as passing.
First define audible speaker level, duty/brightness, inactive-route gating and
profile actual hardware; then choose a documented pocket-feasible pack with
current/protection/charge/NTC/polarity/dimensions reviewed.

## Charging, USB source and rail constraints

[BQ25185](https://www.ti.com/product/BQ25185) is a **linear**, maximum 1 A charger,
with programmable 100/500/1100 mA input limits and nominal **4.5 V SYS** in adapter
mode. It remains a candidate. The BM83 operating rail must not be directly tied
to this 4.5 V node; a separate qualified supply/isolation is required, including
cell-end voltage, powered-off I2S/UART backfeed and disabled internal BM83 charger.
[BM83](https://www.microchip.com/en-us/product/bm83) supply and pin revisions must
be traced to the selected TA module/firmware. No silent regulator substitution.

Ideal constant-current-only times for a hypothetical 6.6 Ah pack are 22 h at
300 mA, 13.2 h at 500 mA and 6.6 h at 1 A. With the explicit 1.3 taper screening
factor: 28.6/17.16/8.58 h. These are conditional arithmetic, not achieved charge
times; cell limits, precharge, temperature, charge-while-play and timer can make
them longer or prevent completion. Review the latest safety timer in
[power assumptions](power-assumptions.md); do not defeat it to fit an oversized pack.

The tool's source-current calculation is an **optimistic switching-energy ceiling**,
not a model of BQ25185's linear current allocation. With 1.747 W playback,
4.2 V × 1 A battery charge and assumed 85% conversion, the energy-only floor is
**1.399 A from 5 V**. A 500 mA source leaves only a 90 mA optimistic charge ceiling;
100 mA leaves none. Even advertised 1.5/3 A cannot bypass the chosen charger's
1100 mA input limit. Real linear path balance/thermal losses must be recalculated
from its reviewed circuit, with source permission and startup/suspend policy.
Linear charging alone dissipates approximately (5−3.7)×1 = **1.3 W** at 1 A;
system path loss adds heat. An enclosed pocket device needs thermal review.

[TPS63802](https://www.ti.com/product/TPS63802) remains the main regulator
candidate; its headline output rating does not prove the specific low-cell/
inductor/capacitor/thermal layout. Actual rail, charger/source strategy, pack and
NTC remain builder decisions. No lithium connection, order or schematic-entry
approval is conveyed by this model.
