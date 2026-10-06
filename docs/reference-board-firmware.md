# Reference-board firmware checkpoint - 2026-10-06

This is AI-authored implementation and digital testing, not physical qualification
or permission to connect the unreviewed board to a lithium pack.

## Explicit board profiles

ESP-IDF menuconfig -> Nightwave hardware has two mutually exclusive profiles.
The default is the AI reference PCB. CI builds both in separate clean jobs:

```
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.reference" build
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.bench" build
```

Use separate build/config directories when changing profiles; defaults do not
override an existing sdkconfig. Check the boot log's board-profile message.
Reference defaults use 16 MB flash; the legacy USB bench profile uses 8 MB.
Neither profile authorizes use on unreviewed hardware.

`reference_board.h` is checked against U1 and U11 in the schematic manifest by
`tools/validate_reference_firmware.py`. The old direct-button profile must never
be used on this PCB: those GPIOs now serve BM83 signals. GPIO18 is DAC XSMT,
while GPIO39 enables the headphone amplifier. GPIO21 receives/reserves BM83
MCLK1; it must not be driven as an ESP32 master-clock output. GPIO10 receives
BM83 UART_TX_IND in host mode; its old name BT_WAKE is not an output direction.

## Implemented reference adapters

- TCA9535 at 0x20, P0.0..4, input-only configuration with readback and bounded
  I2C transfers. InputTask polls both ports. An I2C failure cancels gestures;
  recovery requires release before a new press. Retries are rate-limited to 1 Hz.
  IRQ-driven scheduling and SD/status propagation are still pending.
- SPI2 ST7789 portrait 240x280, row offset 20, panel-specific initialization,
  a 7680-byte tile, RGB565 byte order, display sleep, backlight on/off, and a
  bounded ASCII renderer for the existing browser and wrapped lyric view.
  It uses a dedicated synchronous SPI device owned by UiTask, never AudioTask.
  This is an initial readable text implementation, not finished typography:
  Unicode, proportional fonts, richer browsing and brightness PWM remain open.
- Separate speaker enable, DAC XSMT, and headphone enable controls, including
  a muted transition between output routes. Physical click/pop behavior and
  analog settling still require measurement and possible sequencing changes.

The TFT uses 20 MHz SPI as an unqualified bring-up setting, not a measured
maximum. Full-screen refresh transfers 134400 bytes; bus-only time is about
53.76 ms at that setting, excluding rendering and command overhead. It does not
block the separate audio task, but end-to-end UI/audio scheduling needs profiling.
Backlight is only enabled after a complete first frame; transport failures turn
it off and release the bus. Display failure does not imply playback failure.

InputTask currently owns I2C0 in the reference profile. Future charger/gauge
drivers must share this bus handle through a single board-bus owner; they must
not create a second I2C0 driver. Legacy OLED owns I2C0 only in the bench profile.

## Test boundaries

Host tests cover initialization register ordering/window endpoints, frame size,
sleep/wake and injected failures at every initialization transfer, gesture
recovery, and real audio-adapter GPIO calls under both profiles. Native ESP-IDF
builds are separate from host tests. None measures LCD appearance, sound,
Bluetooth interoperability, charger safety or battery runtime.

Bluetooth and power monitoring in app_main still use unavailable backends.
This checkpoint does not claim a completed integrated player. The physical
qualification flags deliberately remain false.

## Primary implementation sources

- Waveshare 1.69-inch LCD Module vendor example `LCD_1in69.c`, V1.0,
  2023-03-09, in `LCD_Module_RPI_code.zip`:
  https://www.waveshare.com/wiki/1.69inch_LCD_Module
  Panel register data and window geometry were checked against the cached
  official example; this is a separately written transport/renderer.
- ESP-IDF SPI master API, dedicated device ownership and DMA requirements:
  https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/spi_master.html
- TCA9535 register interface: https://www.ti.com/lit/ds/symlink/tca9535.pdf
- BM83 pin directions: manufacturer DS70005402D, table 2-2.
