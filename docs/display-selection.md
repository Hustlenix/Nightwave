# BD-15 Readable display — Waveshare 24382 selected

Builder selected **A**, non-touch Waveshare SKU **24382**, in chat on 2026-10-03.
Personal reasons were not supplied. The comparison below preserves the dated
2026-10-02 research, not an order or live stock guarantee. Direct maker pages
returned 403 during the 2026-10-03 refresh; indexed official specifications and
$9.49 listing matched the recorded snapshot. Numeric stock/India delivery remain unknown.
The old 128x64 OLED remains a bench adapter, not a realistic final default.

| Item | A — Waveshare 1.69inch LCD Module | B — Adafruit 1.54-inch IPS |
| --- | --- | --- |
| Exact SKU | **24382**, non-touch; not touch SKU 27057 | **3787**, current EYESPI breakout |
| Active area | **27.972x32.634 mm**, rounded R5 corners | Maker-linked QT154H2201 drawing: **27.72x27.72 mm**; confirm that drawing against current IPS revision |
| Physical envelope | Board **31.5x39 mm**; thickness/connector projection must be verified from current drawing | Board **43.7x41.8x5.5 mm**, 9.8 g; product describes screen envelope 32x31 mm (not active area) |
| Resolution/controller | **240x280**, **ST7789V2** | **240x240**, **ST7789** |
| Interface/GPIO | 4-wire SPI, MOSI/SCLK/CS/DC + RST/BL = **6 GPIO**; no MISO/touch | SPI, same six with software reset/brightness; auto-reset can reduce to 5, fixed backlight to 4 after review |
| Supply/current | 3.3/5 V carrier; logic must match supply. Maker FAQ reports **3.3 V, 90 mA maximum** = **0.297 W** module input; not our measurement | 3.3/5 V carrier with regulator/level shift; whole-breakout current not published here. Linked panel document conflicts (20 mA maximum vs 45 mA typical backlight); do not use it as an authoritative whole-board rating |
| Driver support | Maker Arduino/STM32/RPi examples; IDF ST7789 controller driver available, panel offset/rotation still to test | Adafruit ST7789 Arduino/CircuitPython support; IDF controller driver available, module revision/offset still to test |
| Lyrics | Same pixel width, 40 extra vertical pixels, about two extra 20-pixel lines before margins; no text in rounded corners | Good large glyphs/current-next lines, less vertical room; same width |
| Enclosure | Smaller board, cable bend/connector and lens allowance pending actual drawing | Larger footprint plus EYESPI cable; overlay/retention required to prevent backlight peeling |
| Price/availability | **$9.49 USD**, maker indexed listing has Add to Cart; numeric stock/India delivery not verified | **$17.50 USD**, live maker page listed **2 in stock** |

Seller prices exclude delivery/tax/mounting and can change. Product/Wiki listing
may be search-cached when direct Waveshare pages refuse fetching; not a stock
reservation. [Waveshare product](https://www.waveshare.com/product/1.69inch-lcd-module.htm),
[Waveshare specifications/power FAQ](https://www.waveshare.com/wiki/1.69inch_LCD_Module),
[Adafruit product](https://www.adafruit.com/product/3787),
[linked panel drawing/electrical document](https://cdn-shop.adafruit.com/product-files/3787/3787_tft_QT154H2201__________20190228182902.pdf).

## Software and acceptance boundary

`DisplaySink` now accepts full bounded title/artist/album and current/next lyric
strings, timing and playback flags; injected adapters are host-tested. The
legacy OLED consumes a truncated fallback frame only. A TFT must render/wrap
the full fields, not silently inherit OLED's 22-column text. Unicode/glyph
coverage must be declared; preserving UTF-8 text is not complete Unicode support.

RGB565 framebuffer: 115200 / 134400 bytes for B/A, explicit PSRAM. Two 240x16
DMA tiles: 15360 internal bytes. Ideal full-frame wire times at 20 MHz:
46.08 / 53.76 ms; at 40 MHz half. These are arithmetic, not refresh results.
Use bounded partial updates at 5 Hz, audio priority and no frame-time SD art
loads/allocations. Current panel drivers are **not** implemented or qualified.

`SelectedDisplayProfile` now tests portrait 240x280 bounds, inclusive GRAM windows
with a 20-row offset and <=16-row RGB565 tiles, including bottom row 299 and
failed-call output preservation. Maker RPi example records MADCTL 0x00, COLMOD
0x05, 100 ms reset phases and 120 ms sleep-out wait. These constants come from
[Waveshare's downloadable example](https://files.waveshare.com/upload/8/8d/LCD_Module_RPI_code.zip),
not an actual initialized panel. Exact module revision/gamma/colour/orientation
and hardware SPI init/render/sleep tests remain required. Do not silently replace
0x05 with a generic ST7789 value or treat this geometry contract as a working UI.

Now that the builder has chosen, combine GPIO with selected BM83 UART/wake/reset and
the selected TCA9535PWR input expansion,
I2S, verify connector/outline/revision, implement exact panel initialization,
offsets/PWM/sleep and test long/Unicode policy/lyric wrapping/readability. Measure
brightness-dependent current. No final cutout, battery or schematic task before
selection propagation and updated power/BOM/pins. The actual chat selection is
recorded in builder-decisions.md; it does not qualify the physical panel or approve
the remaining GPIO, battery, power circuit or mechanical details.
