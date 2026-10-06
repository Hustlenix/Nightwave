# Exclusive-output estimates - 2026-10-06

Source: `python tools/output_power_model.py`. Every numeric result below is
CALCULATED from ESTIMATED loads/efficiencies, not MEASURED. No pack selected.
Runtime uses 3.7 V nominal, 80% usable, 80% aging and 20% load margin. Currents
below are cell-side at 3.7 V before load margin; the tool also gives 3.2/4.2 V.

| Mode / estimate | System W | Regulator loss W (included) | Cell mA | Hours 3000/4000/5000/6000 mAh |
| --- | ---: | ---: | ---: | --- |
| Speaker typical | 1.747 | .211 | 472 | 3.39 / 4.52 / 5.65 / 6.78 |
| Speaker conservative | 3.516 | .431 | 950 | 1.68 / 2.25 / 2.81 / 3.37 |
| Wired typical | 1.459 | .211 | 394 | 4.06 / 5.41 / 6.76 / 8.11 |
| Wired conservative | 2.316 | .431 | 626 | 2.56 / 3.41 / 4.26 / 5.11 |
| Bluetooth typical | 1.646 | .243 | 445 | 3.60 / 4.80 / 5.99 / 7.19 |
| Bluetooth conservative | 2.632 | .521 | 711 | 2.25 / 3.00 / 3.75 / 4.50 |
| Paused typical, dimming conditional | .549 | .079 | 148 | 10.78 / 14.37 / 17.97 / 21.56 |
| Paused conservative, lit | 1.522 | .299 | 411 | 3.89 / 5.18 / 6.48 / 7.78 |
| Scan/UI typical, playback paused | 1.784 | .264 | 482 | 3.32 / 4.43 / 5.53 / 6.64 |
| Scan/UI conservative | 3.420 | .679 | 924 | 1.73 / 2.31 / 2.89 / 3.46 |

Major contributors: playback MCU 200/300 mA, SD40/100 mA, display90 mA,
retained analog28 mA, control5 mA at 3.3 V. Speaker .25/1 W electrical audio;
headphones .02/.1 W; BT50/100 mA at 3.6 V only in Bluetooth mode. Regulator
efficiency85/80%, amplifier80/75%, cell-side overhead25 mW. All are ESTIMATED
screening inputs; display90 mA is a conservative allowance grounded in the maker
maximum, not a measured playback average. No inactive analog power-off assumed.

F: charging while playing is calculated separately for each of the three routes,
at both loads and 100/500/1500 mA conditional 5 V sources. Tool reports source
energy floor, available charging, deficit and a 5000 mAh/960 mA charge sensitivity.
100 mA cannot sustain any playback mode in this model; it provides no charging
headroom. At sufficient source power, 5000/960*1.3 = 6.77 h estimated charge
screen, not a measured or guaranteed charge time. Runtime while powered is not
reported as battery-only runtime. Source conversion heat at full 1.5 A and85%
efficiency is1.125 W; this is an assumed loss screen, not IC junction temperature.

Speaker heat/peak current, headphone charge-pump/load, BT regulator/RF, paused
retained rails and SD scan bursts all require thermal/current verification.
Do not pick 6000 mAh and claim eight-hour speaker playback: it fails these
assumptions. Lower average draw through measured duty/brightness/clock policy
is needed before a compact capacity recommendation can be finalized.
