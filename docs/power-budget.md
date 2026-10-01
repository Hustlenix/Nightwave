# Power budget — calculated, not measured

Checked 2026-10-01. This is a conservative design-input model, not a completed
power circuit, a measured runtime claim, or authorization to connect lithium.

## Estimated normal speaker-playback profile

| 3.3 V consumer | Estimated average mA | Estimated concurrent peak mA |
| --- | ---: | ---: |
| ESP32-S3 decoding, radio disabled | 200 | 500 |
| microSD including reads | 40 | 200 |
| PCM5102A provisioned even in speaker mode | 25 | 30 |
| Headphone amplifier/control leakage allowance | 3 | 15 |
| OLED, normal 30 s timeout policy | 5 | 30 |
| Buttons, pull-ups, miscellaneous | 5 | 10 |
| Total | 278 | 785 |

These are **ESTIMATED allocations**, not datasheet-certified maxima. SD card
type, CPU load, display contents, regulator layout and audio gain can exceed
them. Reserve 1 A continuous 3.3 V design capability and test 0.8 A pulses at
minimum usable cell voltage. Regulator limits must be checked at that voltage,
not inferred from the product's headline current rating.

Speaker assumption: 0.25 W average electrical output into the selected 8-ohm
speaker, 80% amplifier efficiency allocation, and 0.025 W direct-rail overhead.
This is a declared model profile, not a known volume-percent mapping. The
firmware's 8% default volume does not prove 0.25 W output.

## Calculated energy and capacity

```
P_3V3 = 3.3 * 0.278 = 0.9174 W
P_cell = P_3V3 / 0.85 + 0.25 / 0.80 + 0.025 = 1.4168 W
required nominal Wh = P_cell * 8 h * 1.20 / (0.80 * 0.80) = 21.252 Wh
required nominal Ah at 3.7 V = 21.252 / 3.7 = 5.744 Ah
```

85% buck-boost and 80% Class-D efficiencies are **ESTIMATED**, not measured or
guaranteed minimums. The 20% load margin, 80% usable energy and 80% aging
retention are separate model assumptions. Do not divide by regulator
efficiency again after converting to cell-equivalent power/current.

Provisional capacity candidate: Adafruit product **353**, protected 1S 6600 mAh
parallel-cell pack, nominal 3.7 V, 69 x 54 x 18 mm, 155 g, attached 2-pin JST-PH,
listed $24.50 before shipping/tax. The supplier recommends sustained current
below 1.3 A in its description despite a higher technical-table rating; use
the lower figure until clarified. This is a sourced product ID, not an invented
manufacturer MPN. Polarity, tolerance, NTC mounting and regional shipping must
be verified. [Supplier specification](https://www.adafruit.com/product/353).

With the same margins the model yields about **9.19 h** for 6600 mAh and **3.48 h**
for 2500 mAh. Neither is achieved runtime. At 0.5 W speaker average, the required
capacity rises to about 7.01 Ah; 6600 mAh would no longer meet this model.
This sensitivity prevents locking capacity from an unspecified music/volume
profile. The former BOM's 2500 mAh entry is a superseded sizing candidate,
not the final runtime solution.

At 3.2 V cell and simultaneous 785 mA 3.3 V peak plus 1 W speaker output,
estimated cell demand is `(3.3*.785/.85 + 1/.80 + .025)/3.2 = 1.35 A`.
That exceeds the candidate's conservative sustained rating: constrain speaker
power/duty and validate pulse ratings, or select a documented higher-current
pack. Do not silently accept this worst case.

**Final runtime remains PENDING PHYSICAL VALIDATION.**

Reproduce the arithmetic with `python tools/power_budget.py`; the script also
checks sensitivity and rejects nonphysical model parameters.
