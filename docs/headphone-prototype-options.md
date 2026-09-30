# Headphone prototype options

Checked: 2026-09-30. Availability and price can change; recheck before any purchase. This document is not purchase authorization.

## Decision

**PROVISIONAL SELECT: Adafruit product 6309, TLV320DAC3100 I2S DAC with headphone and speaker output, for the first practical headphone proof.**

This is a bench-only alternative path. It does not replace the provisional final PCM5102A + TPA6132A2 architecture and it does not validate that final analog chain. The module was listed in stock at USD 6.95 on the check date. It accepts I2S, requires I2C register configuration, can derive its clock from BCLK, and its headphone output is documented for 16 ohm and 32 ohm loads.

Do not buy or wire it from this note alone. At the human gate, confirm the exact received product number and revision with clear front/back photos. Then create a revision-specific wiring table. The initial test must use the headphone output at a conservative digital level; its Class-D speaker terminals remain disconnected.

## Comparison

| Option | What it proves | Availability / effort | Main risks | Disposition |
|---|---|---|---|---|
| TI TPA6132A2EVM2 after PCM5102 breakout | Closest prototype of the intended final split DAC/amplifier path; selectable gain and single-ended inputs | TI currently reports the EVM unavailable for direct order; distributor sourcing and price are unresolved | Old/limited module supply; an ambiguous marketplace substitute could have a different circuit or pinout | PREFERRED ARCHITECTURAL MATCH, BLOCKED ON SOURCING |
| Adafruit 6309 TLV320DAC3100 breakout | Direct ESP32 I2S-to-stereo-headphone proof using a documented current board | Listed in stock at USD 6.95; board, schematic, guide, and reference drivers are published | Replaces both PCM5102 and TPA6132 during this test; requires a new ESP-IDF I2C control driver and does not validate the final analog path | **PROVISIONAL SELECT** |
| Adam Keher TPA6132A2RTER breakout | Retains the selected headphone amplifier IC and accepts line input | Open MIT Eagle source and Gerbers are available, but fabrication and WQFN assembly are required | Third-party design has very limited adoption; schematic, BOM, jack detection, grounding, assembly, and output safety all need human review before fabrication | RESERVE FABRICATION OPTION; NOT READY TO ORDER |

## Electrical fit of the provisional selection

- Logic is 3.3 V; the existing ESP32-S3 I2S signals can be reused.
- BCLK, WSEL, and DIN are already allocated on GPIO5, GPIO6, and GPIO7.
- The existing I2C bus on GPIO8/GPIO9 can configure the codec, but bus sharing with the OLED must be tested.
- The breakout requires reset and I2C configuration. A reset allocation must be chosen only after the exact module and display wiring are confirmed.
- Use 3.3 V for a headphone-only first test. The board documentation permits 3 V to 5 V board power but states that logic is 3.3 V only.
- The codec headphone outputs are AC-coupled on the breakout. Do not use or probe its Class-D speaker output as a ground-referenced signal.

## Integration sequence

1. Complete DevKitC, button, SD, and muted I2S diagnostics first.
2. Prove the MAX98357A speaker path at very low level if that exact documented module is available.
3. When the exact Adafruit 6309 is present, capture both sides and confirm its revision.
4. Add a dedicated TLV320DAC3100 ESP-IDF I2C component using the manufacturer register map and the published Adafruit initialization sequence as a cross-reference.
5. Start muted, configure clocks from BCLK, select 16-bit I2S, set a conservative DAC/headphone gain, prefill zero PCM, then enable headphones.
6. Run silence, left/right identification, and the dual-tone fixture. Return logs and observations; do not claim audio quality from firmware-only testing.
7. Separately revisit a TPA6132A2 board before the final schematic is frozen.

## Sources

- [TI TPA6132A2EVM2 product page](https://www.ti.com/tool/TPA6132A2EVM2)
- [TI TPA6132A2 product and datasheet page](https://www.ti.com/product/TPA6132A2)
- [Adafruit product 6309](https://www.adafruit.com/product/6309)
- [Adafruit TLV320DAC3100 guide](https://learn.adafruit.com/adafruit-tlv320dac3100-i2s-dac?view=all)
- [TI TLV320DAC3100 datasheet](https://www.ti.com/lit/ds/symlink/tlv320dac3100.pdf)
- [Adafruit open PCB source](https://github.com/adafruit/Adafruit-TLV320DAC3100-I2S-DAC-PCB)
- [Third-party TPA6132A2 breakout source](https://github.com/AdamKeher/TPA6132A2RTER-Breakout)

