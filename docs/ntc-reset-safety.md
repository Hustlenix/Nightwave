# Battery-temperature reset safety

2026-10-12. No hardware qualification or CAD change.

For BQ25628E reset thresholds, merely replacing the reference resistors with
nominally calculated values is insufficient. `tools/ntc_window_screen.py` checks
a necessary condition for a pull-up and thermistor-to-ground network, with or
without a parallel lower fixed resistor. It does not select a final network.

For normalized TS voltage v, define a=1/v-1. With pull-up Rt and lower shunt Rb,
a=Rt/Rntc+Rt/Rb. A positive shunt cannot increase a_hot/a_cold beyond
Rntc_cold/Rntc_hot. Guaranteed blocking at the two limits therefore requires
R_cold_min/R_hot_max >= (1/Vhot_min-1)/(1/Vcold_max-1).

DATASHEET comparator bounds used are cold maximum .738 and reset hot minimum
.339, from BQ25628E Rev C section 7.5. The required ratio is CALCULATED 5.4923.
[TI source](https://www.ti.com/lit/ds/symlink/bq25628e.pdf).

DATASHEET nominal 103AT resistances: 27.28 kohm at 0 C, 5.827 kohm at 40 C and
4.160 kohm at 50 C. ESTIMATED resistance at 45 C uses log interpolation. The
ESTIMATED +/-2% envelope is a sensitivity, NOT a manufacturer's guaranteed R(T)
bound. Exact R25/B tolerance, aging, resistor tolerance, self-heating and mounting
lag must replace it for release.
[SEMITEC source](https://www.semitec-global.com/uploads/2022/01/P12-13-AT-Thermistor.pdf).

Under that sensitivity the available ratio is CALCULATED 5.3235, which fails.
Even with no lower shunt the cold bound requires Rt <=9491 ohm while the hot
bound requires Rt >=9792 ohm. Nominal-only arithmetic passes; tolerance does not.
Adding a positive parallel shunt cannot fix this necessary-condition failure.

Engineering disposition: do not release a 103AT passive network on nominal
reset-threshold arithmetic. Investigate an independently hardware-enforced
window or a suitable steeper guaranteed thermistor curve; both require exact
component and pack thermal-contact analysis. Programming the hot threshold is
not by itself reset-safe. CE must remain disabled through reset and watchdog
recovery until qualification is restored, and independent thermal protection
must still cover stuck firmware. Keep TS enabled. The pack's protection PCB
does not imply charging-temperature protection unless its specification says so.

This is a documented rejected approach, not a request for the builder to solve
the analog design and not proof that no safe architecture exists.
