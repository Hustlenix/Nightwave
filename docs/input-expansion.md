# BD-16 input expansion — TCA9535PWR

Builder selected TI **TCA9535PWR**, input-only, on 2026-10-03. Its 24-pin PW
TSSOP package and [official part page](https://www.ti.com/product/TCA9535/part-details/TCA9535PWR)
are explicit in the BOM; current unit price/numeric stock were not exposed.
No order or address/port/IRQ assignment is implied. Exact pulls/support/mate
parts must be split from pending support rows before schematic release.

## Electrical and map review

[TI datasheet SCPS201F, September 2026](https://www.ti.com/lit/ds/symlink/tca9535.pdf),
checked 2026-10-03: two eight-bit ports, 0x20-0x27 selected by A2/A1/A0,
up to 400 kHz I2C, no internal input pull-ups, open-drain active-low INT.
Use 3.3 V product logic. Verify PW pin numbering/land pattern separately from
the QFN alternatives. Address straps must be fixed, not floating, and must
not collide with the fuel gauge or other selected bus devices.

Move five buttons plus slow SD detect, charge status and fuel alert to eight
reviewed input positions. Keep headphone insertion, audio enables/mute, power
hold, TFT and BT timing/control directly on S3. Eight unused expander inputs
also need externally defined levels. Determine active polarity and pull-up
rails individually, especially open-drain charger/gauge outputs and powered-off
behavior; not every candidate status output may be treated as a push-pull GPIO.

The selected strategy saves seven direct GPIO after adding INT. The exact
[pin audit](pin-budget.md) leaves only one baseline spare or none with MCLK.
Do not infer sufficient pins for RTS/CTS, independent DAC mute or CC IRQ without
review. Expander selection is not a completed combined board map.

## Implemented and tested digitally

`Tca9535Input` initializes both configuration registers to all-input, disables
polarity inversion, verifies both readbacks and reads both input ports before
declaring ready. Each poll performs one two-byte read, reporting raw levels
and a change mask. A failed/partial read does not change the caller's sample;
the driver invalidates readiness and requires explicit reinitialization.
Recovery starts a fresh baseline, not a synthesized press/change event.
No allocation, output-register write or automatic retry loop exists.

`Tca9535I2c` is a compiled ESP-IDF adapter borrowing an existing shared I2C bus;
it accepts only legal addresses and input/configuration/polarity register pairs,
uses repeated-start reads and **10 ms finite transaction timeouts**, and releases
its device handle. The caller owns bus lifetime and serialized access. The
actual chip/clock stretching can change elapsed timing; this is not a measured
10 ms UI response guarantee. It is not wired into the old GPIO ButtonMonitor.

Host regression covers every input bit, both ports, all invalid byte addresses/
registers, initialization readback mismatch, each failed initialization operation,
partial failed reads, bounded poll fault/recovery, detach failure/retry, adapter
packet lengths and finite timeouts. ESP32 firmware compilation and host tests
are separate from electrical qualification.

## Remaining integration and physical acceptance

- One shared-bus owner must supply the chosen board address/port map; replace
  direct button reads through an explicitly selected product configuration,
  retaining debouncing/long-press and testing stale/bus-failure gesture suppression.
- INT only schedules work; never perform I2C in its ISR. Poll both ports on a
  bounded periodic fallback even without INT. TI warns that transitions near
  the read ACK can be missed, and reading one port does not clear the other.
  This is slow human/status input capture, not a lossless pulse counter.
- Schedule outside audio workers and avoid busy retry starvation on a stuck bus.
  Reinitialize after bus/device power recovery; current samples alone cannot
  prove configuration survived an unnoticed brownout.
- Review total bus pull-ups/capacitance/sink current and every undriven input;
  measure worst button response, SD/charge/gauge changes, shared-bus faults,
  boot/POR/off-state levels and pressed-input current.
- Verify the exact footprint/source and human electrical review before assembly.

Physical qualification is false. A host fake does not establish working buttons
on the finished device or satisfy Trial evidence.
