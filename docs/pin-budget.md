# ESP32-S3 Pin Budget

Basis: `ESP32-S3-WROOM-1-N16R8` / `ESP32-S3-DevKitC-1-N8R8`. This is a provisional logical map, not a wiring instruction.

## Reserved and dangerous GPIO

- GPIO0, GPIO3, GPIO45, GPIO46: strapping; avoid for attached circuits that can alter boot levels.
- GPIO19/20: native USB/JTAG D−/D+; reserved for programming/debug.
- GPIO26–32: flash/PSRAM interface; do not use.
- GPIO33–37: octal PSRAM signals on R8 variants; do not use.
- GPIO43/44: UART0 TX/RX; retain for bring-up logs.
- GPIO45/46 also have special input/voltage constraints; left unused.

Source: [ESP32-S3 GPIO documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html) and the module datasheet.

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

## Final Board Provisional Pin Map

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

- confirm all selected GPIO are actually bonded out on the exact module footprint;
- confirm no development-board onboard device conflicts with the prototype map;
- use external SD pull-ups required by the SD specification; internal pulls are not the final design;
- verify each button's idle level and boot-time state;
- verify I2S fanout signal integrity and whether small series resistors at the ESP source are useful;
- confirm charger/fuel-gauge alert polarity and whether pull-ups are required;
- run `gpio_dump_io_configuration()` during bring-up and compare with this table.
