# Independent charge-temperature window: design investigation

2026-10-12. Requirements and corner calculations only; no schematic or PCB
edited. This is not a released safety circuit or a replacement for charger TS.

## Candidate signal path

Use a separate SEMITEC 103AT-2 pack-contact sensor, pulled up by 10 kohm to
the analog supply. Do not share/load the BQ25628E TS node. Two ratiometric
references use 100 kohm upper resistors and 215 kohm (cold) / 61.9 kohm (hot)
lower resistors. All fixed resistors in this calculation have 0.1% tolerance.
The sensor is in range only when its voltage is below the cold reference and
above the hot reference. Open sensor must fail cold; short must fail hot.
Wiring shorts to intermediate voltages are not covered by that fault model.

`tools/independent_thermal_window.py` enumerates independent resistor corners,
an ESTIMATED +/-5% sensor resistance envelope and an ESTIMATED 33 mV comparator
allowance at 3.0 V. It checks blocking at 0/45 C and permission at 25 C. The
45 C resistance is log-interpolated from nominal 40/50 C table values.
Positive margins prove only this sensitivity calculation. They do not establish
guaranteed sensor accuracy, temperature trip points or thermal response time.

## Startup finding and implementation requirement

TI TLV7042DGKR was investigated for its two open-drain outputs. Its POR behavior
is high impedance, so wired-AND outputs with a pull-up cannot alone establish
valid temperature at startup. A released output is not evidence of a healthy
sensor. Do not connect that combined signal directly to an active-low charger
enable pull-down.

The push-pull TLV7032 family instead holds outputs low during POR. Investigate
two separate active-high healthy signals with an AND qualification stage,
not tied push-pull outputs. Either approach still needs supply-valid startup
delay, output validity across brownout, no unpowered backfeed and a default-off
CE stage. A pull-down on firmware intent is mandatory. Temperature permission
alone must never enable charging: source permission, qualified configuration,
valid pack, software intent and fault latch all remain necessary conditions.

## Before schematic release

- Verify the exact comparator variant, common-mode accuracy, hysteresis,
  startup delay, logic thresholds and powered-off behavior at all supply corners.
- Replace the assumed sensor envelope with guaranteed R(T) bounds, aging,
  self-heating and attachment/thermal-lag allowance for the selected pack.
- Select exact resistor, logic, supervisor and CE switch MPNs and document
  input/output states for disconnected supply, reset, open/short sensor and
  stuck software intent. No final component selection is implied here.
- Keep the BQ TS path active as a separate protection layer. Verify its reset
  and watchdog settings independently. Hardware temperature gating does not
  correct excessive default voltage/current or precharge/trickle settings.
- Perform independent schematic review before battery connection; physical
  temperature ramps/fault injection later verify rather than replace analysis.

Sources: [TI comparator datasheet, sections 5 and 6](https://www.ti.com/lit/ds/symlink/tlv7042.pdf),
[SEMITEC AT thermistor table](https://www.semitec-global.com/uploads/2022/01/P12-13-AT-Thermistor.pdf).
