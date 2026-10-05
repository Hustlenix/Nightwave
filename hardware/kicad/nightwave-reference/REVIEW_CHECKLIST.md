# Independent review checklist

- [ ] Confirm exact USB4105 suffix, mating/cutout dimensions, shield strategy and CC implementation.
- [ ] Review fixed 5 V USB-C sink source-current policy: two Rd resistors, no PD or CC current detector; 5.49k ILIM targets about 455 mA typical. Do not assume a USB host permits this before enumeration. Review inrush, ESD and charger defaults/configuration before attaching USB/cell.
- [ ] Confirm BQ25628E RYK-18 pinout/22 split lands, charge current/voltage, safety timer, TS network and thermal limits from the current datasheet.
- [ ] Confirm Adafruit 5035 connector polarity, 3 A limits, mechanical retention, replaceability and external 10 kOhm NTC attachment.
- [ ] Confirm TPS63802 inductors, feedback dividers, effective capacitance, peak currents and thermal copper for both rails.
- [ ] Confirm ESP32-S3 N16R8 GPIO availability, straps, USB, PSRAM exclusions and antenna keepout.
- [ ] Confirm BM83SM1-00TA 50-pad land pattern, supply sequencing, AT source firmware, UART/MFB/service pins, I2S clock direction, sample rates and antenna keepout.
- [ ] Confirm Waveshare 24382 cable connector, pin order, 3.3 V levels, backlight current/PWM and enclosure orientation.
- [ ] Confirm TCA9535 address 0x20, external pulls for every input, interrupt polarity and unused-input treatment.
- [ ] Confirm PCM5102A/TPA6132A2/MAX98357A reference circuits, gain, pop/click behavior, SJ3-350153AG isolated headphone detection and BTL speaker isolation. Review the 47k/10k averaged-stereo SD_MODE divider including internal 100k and GPIO VOH.
- [ ] Review the native four-layer layout for switch-mode current loops, USB 90-ohm differential routing/ESD, RF keepouts, return paths, rail neck-down current density and via/thermal capacity. Autorouter output is not signal-integrity sign-off.
- [ ] Verify 1.2 mm board thickness required by the selected jack, 110x100 mm board outline, real mounting/port access, display cable and pack fit. The board STEP alone is not an enclosure STEP.
- [ ] Finish exact passive/connector/NTC MPNs, DC-bias/voltage ratings, sourcing, assembly capabilities and priced BOM; generic-value rows are pending selections, not purchase-ready MPNs.
- [ ] Reconcile the reference GPIO configuration with firmware. Do not flash the historical OLED/direct-button configuration onto this board and claim TFT/BT qualification.
- [ ] Re-run ERC, DRC, schematic parity, fabrication viewer, assembly drawing and enclosure interference checks.
- [ ] Obtain named independent reviewer sign-off before ordering or connecting a cell.
