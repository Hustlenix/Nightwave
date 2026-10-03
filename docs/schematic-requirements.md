# Schematic mentoring and review requirements

Entry gate: BM83SM1-00TA (BD-14) and Waveshare 24382 non-touch (BD-15) are builder-selected.
Do not start Builder Task 1 until the current software checkpoint is pushed and
verified, selections propagated, selected BD-16 input strategy mapped/reviewed and the new
power/charger/pack/BOM/pin budget reviewed. See current-power-model.md and
pin-budget.md. The legacy OLED footprint and two-output map are not final.

Selected Bluetooth block must reserve the exact 50-pad BM83 land pattern,
qualified 3.2–4.2 V rail, UART, MFB, provisioning/boot, reset and reviewed I2S
clock/rate interface. Disable its internal charger with the external product
charger; evaluate powered-off audio/control backfeed. Preserve AT firmware
provision/recovery access and both module antenna keep-outs. Public framing
support is not qualification of source commands, firmware access or a headset.

BM83 P0_0 is a configuration-dependent UART_TX_IND host-wake output, not the
Test-mode strap. P3_4/SYS_CFG supplies reset-time Test-mode access in public
documentation and can also be RTS. Review the exact TA AT image's functions;
preserve safe service access and do not treat every service signal as free GPIO.
Candidate BQ25185 SYS is 4.5 V in adapter mode and must not directly feed BM83;
PENDING_BT_POWER_RAIL makes the unresolved supply explicit in the BOM.

Checked 2026-10-01. This is a requirements/reference packet, not a complete
netlist or final schematic. The builder chooses parts/values and draws the
actual KiCad sheets. Numbers below are reference values or calculated examples,
not component lock. Use exact package pin tables, never a breakout's labels as
an IC pin map. Review the actual files before marking any connection correct.

## 1. ESP32-S3 module

[Module datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf),
pin definitions, reference schematic and land pattern. N16R8 contains flash,
PSRAM, crystal and RF circuitry: do not duplicate a bare-chip reference circuit.
Connect 3V3 and all module grounds, including the center grounding pad. EN is
reset/enable, GPIO0 selects recovery boot, GPIO19/20 are USB D-/D+. Do not use
octal-PSRAM GPIO35–37. Check module-pad numbers separately from GPIO numbers.

[Espressif checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html)
recommends 3.3 V, at least 500 mA capability for the MCU supply, local bulk and
decoupling, and typically 10 kohm/1 uF EN RC. EN must not float. The RC time
constant is 10 ms, not proof of reset timing for a slow-rising supply. Consider
a supervisor if needed. Keep GPIO0 accessible and avoid a large boot-pin cap.

Review: annotate module variant, EN startup, boot states, reserved pins and
reset/programming access. Unused pins need intentional NCs, not hidden errors.

## 2. USB-C input and service

[GCT USB4105](https://gct.co/connector/usb4105): import the exact suffix drawing
before footprint approval. Decide power-only versus native USB; existing docs
disagree and GPIO19/20 are reserved, not a proven wired interface. In a passive
sink each CC has its own Rd; do not join CC1 and CC2. Account for both connector
orientations, shield stakes, VBUS transient protection and input inrush. D+/D-
need low-capacitance protection and impedance-controlled routing if used.

[TUSB320](https://www.ti.com/lit/ds/symlink/tusb320.pdf), sections 5, 7.3/7.4:
candidate CC current detector with internal Rd in sink mode; do not parallel
extra Rd without checking the implementation. PORT defines sink role, CC1/2
sense attach/current, VBUS_DET senses input, ADDR selects I2C or GPIO reporting,
EN_N controls operation. Its initial GPIO current result is not continuously
updated; I2C refresh needs the documented procedure. Check dead-battery and
powered-off backdrive conditions. This is not a USB-PD controller.

Review: default input cap before MCU boot, authorized current after attach,
detach/advertisement-change fallback, host eligibility and no reverse feeding.
Source-current permission is independent of a charger's programmed limit.

## 3. Charger and power path

See [power options](power-options.md). BQ25185 remains an unresolved candidate,
not a locked answer for the larger pack. IN receives protected VBUS, BAT the
protected pack, SYS the system load; these are different nets. ILIM/VSET selects
input/voltage and ISET selects charge current. TS/MR senses temperature/reset,
CE controls charging, STAT outputs need appropriate pull-ups.
[BQ25185](https://www.ti.com/lit/ds/symlink/bq25185.pdf), pin table and sections
6.3/7.2. Check capacitor derating, cell voltage, NTC and timer explicitly.

For a switching alternative, inspect BQ25628E sections 6, 8.3, 8.6, 9 and 11.
Builder must independently derive its bootstrap, inductor, bias, ILIM/TS and
reset-state circuit from that reference. Do not copy an EVM without explaining
why each populated component is needed. Charging faults must remain visible.

Review: charging with MCU absent/reset, unplug behavior, no-battery behavior,
timer/termination, real cell temperature and source current under playback.

## 4. 3.3 V regulator and master shutdown

[TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf), pin table, section 10
and package drawing. VIN is system input, VOUT regulated output, L1/L2 switching
inductor nodes, FB senses the divider, EN enables, MODE sets operating policy,
PG reports power-good. Tie/decouple every ground/supply as specified. Reference
3.3 V divider is 511 kohm/91 kohm with 0.5 V feedback; calculated nominal is
3.3077 V. The example 0.47 uH inductor and 10/22 uF caps require saturation,
DC-bias and layout verification. Ten numbered pins plus exposed pad is not an
arbitrary eleven-pin symbol. Verify at minimum input and peak load.

Review: SYS-fed speaker and regulator both turn off when requested; charging
can remain available. A GPIO called POWER_HOLD does not create a latch. Define
startup/wake, loss of firmware control and low-battery hardware behavior.

## 5. Fuel gauge

[MAX17048](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
pin descriptions and application examples: VDD goes to protected battery positive
and is the MAX17048 voltage-sense input, bypassed by 0.1 uF. CELL is not internally
connected on MAX17048 (unlike MAX17049); follow its application wiring, not a
guessed sense circuit. CTG, GND and exposed pad go to ground. QSTRT goes low if
unused. SDA/SCL and active-low ALRT use the correct logic-domain pull-ups.

Review: gauge remains battery-powered when the player is off, switched 3.3 V
bus does not back-power the MCU, SOC remains an estimate, alert acknowledgment
and low-battery thresholds have a documented hysteresis/recovery policy.

## 6. microSD

[Molex 104031-0811](https://www.molex.com/en-us/products/part-detail/1040310811)
requires the real contact-number drawing, insertion envelope and switch diagram.
VDD is 3.3 V; CLK/CMD/DAT0 implement the current one-bit firmware. DAT1/2/3 still
need intentional bias. Card detect is a separate switch, not a data contact.
[Espressif SD pull-up requirements](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/sd_pullup_requirements.html):
external 10 kohm pull-ups on CMD and DAT0–3 are required even for one-bit use.

Review: local decoupling/inrush, detect polarity, ESD, no 5 V contacts, removable
card accessibility. Firmware does not currently provide automatic hotplug remount.

## 7. Selected display: Waveshare 24382 non-touch

BD-15 is recorded: 240x280 ST7789V2 SPI, 3.3 V supply and logic, six independent
GPIO signals MOSI/SCLK/CS/DC/reset/PWM backlight. Review the exact MX1.25 8-pin
cable pinout, mating connector MPN/footprint, backlight input limits, reset and
powered-off behavior against the supplied revision. A module drawing is not
a bare-controller footprint. Budget 90 mA maximum maker allowance and bounded
RGB565 tiles; no verified ESP-IDF driver or final combined GPIO map exists.
See display-selection.md, pin-budget.md and current-power-model.md.

### Historical OLED bench reference, not the final product

SH1106-compatible is not an exact purchasable module definition. Obtain the
module maker's pinout, drawing, supply limits, address straps, controller and
mounting pattern. Current driver defaults to SH1106 128x64/0x3c and a two-column
glass offset, with SSD1306 option. Bare panel VCC/charge-pump circuitry is not
equivalent to a four-pin breakout supply. Do not approve the old unverified
SP12864-13 label without supplier evidence.

Review: 3.3 V logic, pull-up rail (not 5 V), optional reset GPIO40 if present,
module height/window/mounts and switched-off bus behavior. No new exact module
or price is asserted here.

## 8. Five buttons

Legacy bench firmware uses active-low switches to ground, GPIO1/2/4/10/15.
BD-16 moves product buttons and slow SD/charge/gauge inputs to TCA9535PWR;
exact port allocation, IRQ GPIO/address straps and debounced frontend integration
remain pending. All 16 ports stay inputs with external defined levels; do not
move mute, power hold or timing signals into this input-only contract. Select
exact momentary switch MPN, actuation force, lifetime, travel and actuator height.
External pull-up choices must account for leakage/off state. Avoid a large RC
that invalidates debounce or creates slow noisy edges. Firmware debounce is
25 ms, hold threshold 700 ms; a held control must not accidentally act as boot.

Review: map previous/play/next/volume-down/up consistently, inspect multi-pad
switch commoning, protect exposed long wires, give every button a real mount.

## 9. PCM5102A DAC

[PCM5102A](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf), pin table and figure
33: BCK/LRCK/DIN receive I2S; SCK is handled per three-wire PLL mode, not floated
by accident. FMT low selects I2S, FLT/DEMP need deliberate straps. XSMT is active-
low mute. AVDD/DVDD/CPVDD, LDOO, VNEG and CAPP/CAPM need the specified supply,
decoupling and charge-pump connections. OUTL/R are line-level, not headphone
drivers. Reference full scale is 2.1 Vrms. Use the reference output filtering.

Review: part-specific pin numbering, pump loop, analog returns and mute timing.
At 22.05 kHz the 150-sample-plus-0.2-ms mute interval is about 7.0 ms. Current
GPIO18 was a prototype DAC-mute control; its relationship to final amp EN must
be explicitly designed, not assumed equivalent.

## 10. Headphone amplifier

[TPA6132A2](https://www.ti.com/lit/ds/symlink/tpa6132a2.pdf), pin table, gain table,
single-ended input and charge-pump examples. EN enables; G0/G1 select -6/0/3/6 dB
(both low is -6 dB). INL/R pairs need the documented single-ended configuration,
not arbitrary grounding of an input. CPP/CPN, HPVSS/HPVDD and supply caps support
the internal pump. OUTL/R connect to headphone tip/ring; sleeve is signal ground.

Review: DAC input amplitude versus amp clipping and chosen loaded output ceiling,
input capacitor high-pass corner, gain straps, enable default low and hotplug
noise. At 2.1 Vrms DAC full scale, -6 dB alone predicts 1.052 Vrms before clipping.
See calculations; no fixed voltage is claimed safe for every headphone or ear.

## 11. Speaker amplifier

[MAX98360 family](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98360A-MAX98360D.pdf),
DAI configurations and exact package drawing. BOM suffix CEFB+T is 10-pin
FC2QFN, not the CENL+T 9-ball WLP. Supply range is 2.5–5.5 V. EN enables,
GAIN_SLOT selects fixed gain/slot, DAI0/1/2 wiring selects input mode/channel.
Do not copy MAX98357A breakout labels to this device. For 6 dB I2S reference,
GAIN_SLOT is tied to VDD; verify DAI configuration for the intended mono stream.
Local reference supply caps are 10 uF and 0.1 uF. OUTP/OUTN are differential BTL.

Review: exact-package assembler capability, corrected OUTP/OUTN pin map
(October 2023 revision), gain/format, disabled boot,
peak supply/thermal limits, short output loop. Never ground either speaker output
or use a grounded scope probe there. Differential instrumentation is required.

## 12. Headphone jack and detect

The indexed [SJ-3503-SMT-TR maker drawing](https://www.sameskydevices.com/product/resource/sj-3503-smt-tr.pdf)
identifies 1=sleeve, 2=tip, 3=ring, 4=tip switch, 5=ring switch. These are audio
switches, not an isolated insertion detector. Full PDF access returned HTTP 403;
the actual source footprint and physical continuity are still NOT verified.
Do not put a 3.3 V pull-up on a contact that connects to headphone audio. An
isolated insertion contact or a properly designed detector is required for GPIO16.

Review: plug absent/present truth table with tip/ring/sleeve continuity, digital
thresholds, ESD, TRS/TRRS behavior, default speaker/amp mute and enclosure access.
Return an exact drawing and later unpowered continuity results; no result invented.

## 13. Speaker connector

Select keyed two-contact connector rated for actual BTL current, mating cable
and retention. Nets are SPK_P/SPK_N, not VCC/GND. At 2 W into 8 ohm, sinusoidal
load current is 0.5 Arms/0.707 A peak; switching/short-circuit stresses require
additional margin. Provide strain relief and keep pair routing together.

Review: connector numbering/mate orientation, insulation, assembly cable length,
speaker terminals never connected to chassis/ground and accessible service plug.

## 14. Battery connector and sensor

Protected 1S pack is still provisional. Verify mating connector, cable current,
actual polarity and protection specs from supplier; a JST-style name does not
guarantee common polarity. Two-wire pack needs a retained insulated external
temperature sensor, not a fabricated third wire or fixed-temperature bypass.

Review: battery removal without piercing/soldering cells, short protection,
NTC open/short faults, rated source/load current, reverse-connection mitigation.
Only a human connects a pack after current-limited non-battery checks.

## 15. Test and programming

Provide named GND, VBUS, BAT, SYS, 3V3, EN, GPIO0 and appropriate service pins.
UART0 GPIO43/44 can provide recovery independent of native USB. No raw 5 V UART
into MCU GPIO. Add current measurement access without interrupting protection.

Review: debugger/probe access after assembly, clear pad labels/polarity, no
inaccessible reset/boot, no grounded speaker test point. Capture actual ERC and
net cross-checks; a checklist is not an executed electrical test.

## Legacy bench GPIO contract — not the selected-product map

GPIO numbers below are not module pads or final product assignments. BD-16
relocates five buttons, SD detect, charge status and fuel alert to TCA9535PWR;
their ports/address/IRQ and combined BM83/TFT map require source review. Do not
draw the final schematic from this historical bench table.

| Interface | GPIO | Direction / default intent |
| --- | --- | --- |
| SD CLK/CMD/DAT0 / card detect | 12/11/13 / 14 | Clock/output, bidirectional data, detect input |
| I2S BCLK/WS/DOUT | 5/6/7 | Outputs to both digital audio branches |
| I2C SDA/SCL | 8/9 | Shared open-drain bus, 3.3 V pull-ups |
| Previous/play/next/down/up | 1/2/4/10/15 | Active-low input |
| Headphone detect | 16 | Active-low input expected; circuit pending |
| Speaker/headphone enable | 17/18 | Default off; final DAC mute relationship pending |
| Charge status / power hold | 21 / 38 | Input / output reserved, final policy pending |
| Fuel alert / display reset | 39 / 40 | Input / output reserved |
| Native USB D-/D+ | 19/20 | Reserved; service decision pending |
| UART0 TX/RX | 43/44 | Recovery service candidate |

Header authority: firmware/components/app_state/include/nightwave/hardware_config.h.
Changing an assignment requires coordinated schematic, wiring docs and firmware
tests. New charger/CC interrupt pins need a budget review, not an invented spare.
Calculate total I2C pull-up resistance including all modules: multiple parallel
resistors may exceed device sink limits. For a 100 pF assumed bus and 300 ns
rise-time budget, Rmax = tr/(0.8473*C) = 3.54 kohm; verify measured capacitance
and each device's VOL/IOL before choosing. This is an example, not a fitted value.
Calculation reference: [TI I2C pull-up application note](https://www.ti.com/lit/an/slva689/slva689.pdf).
