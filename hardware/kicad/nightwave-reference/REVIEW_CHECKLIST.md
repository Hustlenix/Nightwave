# Independent review checklist

- [ ] Confirm exact USB4105 suffix, mating/cutout dimensions, shield strategy and CC implementation.
- [ ] Confirm BQ25628E pinout, input-current detection, ILIM, charge current/voltage, safety timer, TS network and thermal limits from the current datasheet.
- [ ] Confirm Adafruit 5035 connector polarity, 3 A limits, mechanical retention, replaceability and external 10 kOhm NTC attachment.
- [ ] Confirm TPS63802 inductors, feedback dividers, effective capacitance, peak currents and thermal copper for both rails.
- [ ] Confirm ESP32-S3 N16R8 GPIO availability, straps, USB, PSRAM exclusions and antenna keepout.
- [ ] Confirm BM83SM1-00TA 50-pad land pattern, supply sequencing, AT source firmware, UART/MFB/service pins, I2S clock direction, sample rates and antenna keepout.
- [ ] Confirm Waveshare 24382 cable connector, pin order, 3.3 V levels, backlight current/PWM and enclosure orientation.
- [ ] Confirm TCA9535 address 0x20, external pulls for every input, interrupt polarity and unused-input treatment.
- [ ] Confirm PCM5102A/TPA6132A2/MAX98357A reference circuits, gain, pop/click behavior, SJ-3523 switch-based headphone detection and BTL speaker isolation.
- [ ] Re-place and manually route the board for switch-mode current loops, controlled USB pair, uninterrupted RF keepouts and continuous return paths.
- [ ] Re-run ERC, DRC, schematic parity, fabrication viewer, assembly drawing and enclosure interference checks.
- [ ] Obtain named independent reviewer sign-off before ordering or connecting a cell.
