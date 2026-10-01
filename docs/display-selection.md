# Display selection — builder decision required

Checked 2026-10-01. The new request requires readable lyrics, not automatic
retention of the old 128×64 OLED. No TFT is selected; current OLED firmware is a
working provisional adapter, not proof of final-screen readiness.

| Candidate | Resolution / interface | Published envelope | Price / availability snapshot | Readability and integration |
| --- | --- | --- | --- | --- |
| Existing provisional 1.3-inch OLED / Adafruit 938 comparison | 128×64, I2C/SPI | Adafruit board 35.6×33×6.2 mm; maker's module, not old SP12864 drawing | $19.95; recheck stock before lock | Very limited multiline lyrics; existing ASCII driver, smallest framebuffer |
| Adafruit 3787, 1.54-inch ST7789 IPS breakout | 240×240, SPI | Board 43.7×41.8×5.5 mm; screen 32×31 mm; assembled 9.8 g | $17.50, 7 listed in stock | Larger glyphs/color; well-documented maker libraries; large breakout footprint |
| Waveshare non-touch 1.69-inch ST7789V2 IPS module | 240×280, SPI | Board 31.5×39 mm; active 27.972×32.634 mm; thickness/connector drawing still open | Indexed $9.31–$9.99 range; direct page blocked, stock NOT VERIFIED | Extra vertical lyric line; compact candidate, exact offsets/connector and driver must be tested |

Sources: [Adafruit 938](https://www.adafruit.com/product/938),
[Adafruit 3787](https://www.adafruit.com/product/3787),
[Waveshare module](https://www.waveshare.com/product/1.69inch-lcd-module.htm),
[maker indexed price listing](https://www.waveshare.com/product/displays/lcd-oled.htm?dir=asc&order=price).
Prices exclude delivery/tax/cable and are not a procurement quote. Touch is out
of scope; do not accidentally select the similarly named touch product.

## CURRENT OPTION / ALTERNATIVE

CURRENT OPTION: unverified provisional OLED. ALTERNATIVE: 1.54 or 1.69-inch IPS
ST7789-family display, with the builder choosing exact module or bare panel.

- BENEFITS: more readable lyric focus/current-next context, metadata and library
  rows; enough pixels for original angular Nightwave UI, without copyrighted art.
- DRAWBACKS: backlight power, more GPIO, larger active/enclosure area and SPI DMA.
- COST: comparison above; exact panel/carrier/cable/mounting must enter final BOM.
- POWER: no measured current yet. Screen at 20/60/100 mA 3.3 V equivalents plus
  dim/sleep duty-cycle as sensitivity cases. Do not call these datasheet ratings.
- COMPLEXITY: add SPI display HAL, validated controller offsets/rotation,
  partial updates, backlight PWM and bounded glyph rendering. Existing OLED
  text renderer is ASCII only; retaining UTF-8 text is not full Unicode display.
- T4 VALUE: real embedded UI/readability work; visual polish alone proves no tier.
- RISK: RAM/DMA allocation, bus contention, unknown thickness and actual brightness.
- RECOMMENDATION: qualify the **non-touch 1.69-inch candidate** for lyric space
  and compact board; choose Adafruit 3787 if documented availability/support is
  more important. Neither is silently locked. Do not retain OLED without the
  builder explicitly accepting its readability tradeoff.

## Budgets and acceptance

240×240 RGB565 full framebuffer = 115,200 bytes (112.5 KiB); 240×280 = 134,400
bytes (131.25 KiB). Keep framebuffer in explicit PSRAM; two 240×16-line DMA
tiles require 15,360 internal bytes total. At 20 MHz SPI, ideal full frame
transfers are 46.08/53.76 ms before overhead; at 40 MHz, half. These are wire
arithmetic, not measured refresh. Use partial lyric/metadata regions at 5 Hz;
audio has higher priority. No per-frame allocations or SD album-art fetching.

Reserve MOSI/SCLK/CS/DC/reset/backlight controls, and draw a combined GPIO budget
with Bluetooth. There are not enough unrestricted spare GPIO for every new
signal without reuse, hardware-tied reset or an expander. Shared I2C remains
needed by the fuel gauge. Never silently reuse USB/strap/PSRAM pins.

Builder returns chosen SKU, physical envelope, intended viewing distance,
minimum text size/line count, brightness policy, actual decision reasons and
budget impact (BD-15). Then write the real driver/layout and qualify long titles,
Unicode policy, lyric wrapping, cursor/scroll and 320/240-pixel screenshots.
Do not create final screen cutouts, PCB mount holes or CAD before this choice.
