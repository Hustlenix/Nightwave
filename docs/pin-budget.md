# ESP32-S3 Pin Budget

Basis: `ESP32-S3-WROOM-1-N16R8` / `ESP32-S3-DevKitC-1-N8R8`. This is a provisional logical map, not a wiring instruction.

BD-14 selects ESP32-S3 + BM83SM1-00TA, BD-15 selects Waveshare 24382 non-touch,
and BD-16 selects TCA9535PWR **input-only** expansion (2026-10-03). Reconcile their combined SPI/UART/wake/reset/backlight and
antenna/power needs before final schematic work. No new GPIO was silently
assigned and the old OLED interface is not a TFT pin allocation.

## Reserved and dangerous GPIO

- GPIO0, GPIO3, GPIO45, GPIO46: strapping; avoid for attached circuits that can alter boot levels.
- GPIO19/20: native USB/JTAG D−/D+; reserved for programming/debug.
- GPIO26–34: not available at this module's external GPIO pads; do not use.
- GPIO35–37: bonded module signals occupied by octal PSRAM on N16R8; do not use.
- GPIO43/44: UART0 TX/RX; retain for bring-up logs.
- GPIO45/46 also have special input/voltage constraints; left unused.

Source: [ESP32-S3 GPIO documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html) and
[module datasheet v1.8, pin table and R8 footnote](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf), checked 2026-10-03.

## Selected-product capacity audit, not a final numbered map

| Allocation | GPIO count | Basis |
| --- | ---: | --- |
| Module external GPIO pads before octal-PSRAM exclusion | 36 | GPIO0-21 and GPIO35-48 |
| Exclude octal-PSRAM GPIO35-37 | -3 | N16R8 module footnote |
| Exclude four boot straps, native USB pair, UART0 pair | -8 | Retain service/recovery and boot safety |
| Available product pool | **25** | 36 minus 3 minus 8; explicit list below |
| Historical local-output map | 21 | Includes five buttons, three slow status, OLED reset |
| Replace one OLED reset with six TFT controls | +5 | 24382 SPI/data/control/backlight |
| BM83 UART pair, MFB, P0_0 host-wake input, RST_N | +5 | Baseline subject to exact AT configuration |
| Direct-only minimum product signals | **31** | Optional MCLK/flow control/mute/CC IRQ not yet included |
| Move eight slow inputs to TCA9535, add one direct INT | -7 | Five buttons + SD_CD/CHARGE_STATUS/FUEL_ALERT |
| Selected-expander minimum direct signals | **24** | One nominal spare, zero if separate MCLK required |

Explicit retained GPIO pool: **1,2,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,21,
38,39,40,41,42,47,48** = **25**. The module datasheet's 36 count includes
occupied GPIO35-37. Direct-only deficit is **six**;
selected expansion leaves **one** nominal spare, **zero** with extra MCLK.
Use the explicit set, not headline counts, for a reviewed numbered map.
Additional DAC mute, UART RTS/CTS, CC-controller IRQ or reset/power control must
fit this set or be explicitly reviewed; do not silently claim free headroom.

## Prototype Pin Map

| Signal | Peripheral | GPIO | Direction | Boot Risk | Shared | Reason | Status |
|---|---|---:|---|---|---|---|---|
| SD_CLK | SDMMC 1-bit | 12 | Out | No | No | safe matrix GPIO | PROVISIONAL |
| SD_CMD | SDMMC 1-bit | 11 | I/O | No | No | safe matrix GPIO | PROVISIONAL |
| SD_D0 | SDMMC 1-bit | 13 | I/O | No | No | safe matrix GPIO | PROVISIONAL |
| SD_CD | card detect | 14 | In | No | No | socket/removal event | PROVISIONAL |
| I2S_BCLK | I2S TX | 5 | Out | No | DAC + speaker amp | shared synchronous clock | PROVISIONAL |
| I2S_LRCLK | I2S TX | 6 | Out | No | DAC + speaker amp | shared frame clock | PROVISIONAL |
| I2S_DOUT | I2S TX | 7 | Out | No | DAC + speaker amp | one digital stream | PROVISIONAL |
| I2C_SDA | I2C | 8 | I/O | No | OLED + fuel gauge | low-speed control bus | PROVISIONAL |
| I2C_SCL | I2C | 9 | Out | No | OLED + fuel gauge | low-speed control bus | PROVISIONAL |
| BTN_PREV | GPIO | 1 | In | No | No | dedicated control | PROVISIONAL |
| BTN_PLAY | GPIO | 2 | In | No | No | dedicated control | PROVISIONAL |
| BTN_NEXT | GPIO | 4 | In | No | No | dedicated control | PROVISIONAL |
| BTN_VOL_DOWN | GPIO | 10 | In | No | No | dedicated control | PROVISIONAL |
| BTN_VOL_UP | GPIO | 15 | In | No | No | dedicated control | PROVISIONAL |
| HP_DETECT | GPIO | 16 | In | No | No | switched-jack contact | PROVISIONAL |
| SPK_ENABLE | GPIO | 17 | Out | No | No | amplifier enable/mute | PROVISIONAL |
| HP_ENABLE | GPIO | 18 | Out | No | No | headphone path enable | PROVISIONAL |
| DISPLAY_RESET | GPIO | 40 | Out | No | No | deterministic OLED reset if required | PROVISIONAL |

Prototype breakouts may force different pins; any variant must live in one board configuration and preserve the reserved list. The exact Phase 2 wiring package keeps this map except that SparkFun BOB-00544 has no mechanical card-detect switch, so GPIO14 is deliberately unwired. ESP32-S3-DevKitC-1 v1.1 also uses GPIO38 for its onboard RGB LED, so the final-board `POWER_HOLD` assignment is not used on the development board.

## Historical OLED/two-local-output board map — not the current product allocation

| Signal | Peripheral | GPIO | Direction | Boot Risk | Shared | Reason | Status |
|---|---|---:|---|---|---|---|---|
| SD_CLK | SDMMC 1-bit | 12 | Out | No | No | short dedicated route | PROVISIONAL |
| SD_CMD | SDMMC 1-bit | 11 | I/O | No | No | native protocol | PROVISIONAL |
| SD_D0 | SDMMC 1-bit | 13 | I/O | No | No | 1-bit data | PROVISIONAL |
| SD_CD | card detect | 14 | In | No | No | removal/recovery | PROVISIONAL |
| I2S_BCLK | I2S TX | 5 | Out | No | two loads | fanout at source; routing review | PROVISIONAL |
| I2S_LRCLK | I2S TX | 6 | Out | No | two loads | common rate/channel framing | PROVISIONAL |
| I2S_DOUT | I2S TX | 7 | Out | No | two loads | common PCM data | PROVISIONAL |
| I2C_SDA | I2C | 8 | I/O | No | OLED + MAX17048 | shared control bus | PROVISIONAL |
| I2C_SCL | I2C | 9 | Out | No | OLED + MAX17048 | shared control bus | PROVISIONAL |
| BTN_PREV | GPIO | 1 | In | No | No | dedicated physical input | PROVISIONAL |
| BTN_PLAY | GPIO | 2 | In | No | No | dedicated physical input | PROVISIONAL |
| BTN_NEXT | GPIO | 4 | In | No | No | dedicated physical input | PROVISIONAL |
| BTN_VOL_DOWN | GPIO | 10 | In | No | No | dedicated physical input | PROVISIONAL |
| BTN_VOL_UP | GPIO | 15 | In | No | No | dedicated physical input | PROVISIONAL |
| HP_DETECT | GPIO | 16 | In | No | No | mute/route event | PROVISIONAL |
| SPK_ENABLE | GPIO | 17 | Out | No | No | shutdown/power policy | PROVISIONAL |
| HP_ENABLE | GPIO | 18 | Out | No | No | shutdown/power policy | PROVISIONAL |
| CHARGE_STATUS | GPIO | 21 | In | No | No | charger status | PROVISIONAL |
| POWER_HOLD | GPIO | 38 | Out | No | No | controlled shutdown option | PROVISIONAL |
| FUEL_ALERT | GPIO | 39 | In | No | No | MAX17048 alert | PROVISIONAL |
| DISPLAY_RESET | GPIO | 40 | Out | No | No | display reset | PROVISIONAL |
| SPARE_1 | GPIO | 41 | I/O | No | No | debug/future | RESERVED |
| SPARE_2 | GPIO | 42 | I/O | No | No | debug/future | RESERVED |
| UART0_TX | UART0 | 43 | Out | default log | No | ROM/bring-up console | RESERVED |
| UART0_RX | UART0 | 44 | In | default log | No | ROM/bring-up console | RESERVED |
| USB_D− | USB/JTAG | 19 | I/O | native USB | No | programming/debug | RESERVED |
| USB_D+ | USB/JTAG | 20 | I/O | native USB | No | programming/debug | RESERVED |
| SPARE_3 | GPIO | 47 | I/O | No | No | test/future | RESERVED |
| SPARE_4 | GPIO | 48 | I/O | No | No | test/future | RESERVED |

## Checks before schematic lock

Current additions are not assigned by the historical table. The TFT needs SPI
clock/data, CS, DC, reset and PWM backlight (six GPIO with independent reset).
BM83 needs UART TX/RX plus reviewed MFB wake, P0_0 UART_TX_IND host-wake and RST_N
control/service access. Its I2S input may share the existing three audio signals
only after clock-direction, rate, powered-off backfeed and three-load fanout
review; confirm whether the selected AT configuration requires an extra MCLK.
Do not treat sharing as a tested interface.

The four historical free pins (41/42/47/48) cannot cover these additions even
when GPIO40 is reused for TFT reset. BD-16 now selects the TCA9535 input strategy;
the combined numbered map, address, eight port positions and IRQ remain unreviewed.
I2C GPIO8/9 remain bench bus references for the gauge/expander, not the final TFT.
USB, UART bring-up, straps and octal-PSRAM reservations are not spare pins.
`TCA9535PWR` in the BOM records the choice; docs/input-expansion.md tracks integration.
Final firmware pin assignments must follow the builder-reviewed map; none are
silently selected here.

P0_0 is not the Test-mode selection pin. Public BM83 pin descriptions identify
P3_4/SYS_CFG low during reset as Test-mode entry and also assign RTS in applicable
configurations. Preserve service/recovery access and verify the selected TA AT
firmware's actual multiplexing before allocating it. See
[Microchip pin descriptions](https://onlinedocs.microchip.com/oxy/GUID-414904F5-364E-4377-B959-9226AD29D6A9-en-US-8/GUID-5FC141CE-AE19-4658-9333-3E22FD2F4681.html).

Mentor review: GPIO16 needs a real digital insertion detector, not a pull-up on
SJ-3503-SMT-TR's audio switch contacts. GPIO18's prototype DAC mute role versus
final headphone-amp enable requires an explicit circuit/sequencing decision.
Candidate I2C charger/CC controller additions must include address, pull-up,
powered-off and interrupt pin-budget checks before altering this map.

- confirm all selected GPIO are actually bonded out on the exact module footprint;
- confirm no development-board onboard device conflicts with the prototype map;
- use external SD pull-ups required by the SD specification; internal pulls are not the final design;
- verify each button's idle level and boot-time state;
- verify I2S fanout signal integrity and whether small series resistors at the ESP source are useful;
- confirm charger/fuel-gauge alert polarity and whether pull-ups are required;
- run `gpio_dump_io_configuration()` during bring-up and compare with this table.
