# Phase 2 prototype wiring

Checked: 2026-09-30. This is a staged bench-wiring plan for the exact parts in [`hardware/prototype-BOM.csv`](../hardware/prototype-BOM.csv), not permission to connect a battery or a claim that the wiring has been built.

The machine-readable pin-by-pin table is [`hardware/prototype-wiring.csv`](../hardware/prototype-wiring.csv). `tools/validate_project.py` checks its ESP GPIO assignments against the firmware hardware configuration.

## Prototype scope

The prototype proves the architecture in separable stages:

1. ESP32-S3 boot/logging;
2. five buttons and OLED;
3. raw 3.3 V microSD in 1-bit SDMMC mode;
4. PCM5102 stereo line output;
5. TPA6132A2 headphone output;
6. MAX98357A mono speaker output.

No lithium cell or charger is used. The MAX98357A breakout is an available functional proxy for the final MAX98360C speaker path. The Adafruit SSD1306 breakout is a documented UI proxy while the final SH1106 module remains unresolved.

## GPIO map and development-board exceptions

| Function | GPIO | Physical destination |
|---|---:|---|
| I2S BCLK / LRCLK / data | 5 / 6 / 7 | PCM5102 and MAX98357A inputs in parallel |
| I2C SDA / SCL | 8 / 9 | OLED |
| SD CMD / CLK / DAT0 | 11 / 12 / 13 | raw microSD breakout |
| Previous / play / next | 1 / 2 / 4 | normally-open switches to ground |
| Volume down / up | 10 / 15 | normally-open switches to ground |
| Speaker enable | 17 | MAX98357A `SD` |
| DAC mute | 18 | PCM5102 `MUTE`, after polarity confirmation |

The SparkFun BOB-00544 does **not** expose a mechanical card-detect switch. Its `CD/DAT3` label is the SD DAT3/chip-select signal, so prototype GPIO14 is left unused and card insertion/removal is handled through mount/read failures. This is intentionally different from the final switched socket.

ESP32-S3-DevKitC-1 v1.1 drives its RGB LED from GPIO38. Do not connect the provisional final-board `POWER_HOLD` function to GPIO38 during the prototype. GPIO19/20 remain reserved for native USB, GPIO43/44 for UART0, and strapping/flash/PSRAM GPIO remain unused.

## SD pull-ups

Espressif specifies 10 kΩ pull-ups on CMD and DAT0–DAT3 even in 1-bit mode, and states that the DevKitC-1 provides none. Install five external resistors:

| Card signal | Connection |
|---|---|
| CMD | 10 kΩ to 3.3 V, plus GPIO11 |
| DAT0 | 10 kΩ to 3.3 V, plus GPIO13 |
| DAT1 | 10 kΩ to 3.3 V; otherwise unconnected |
| DAT2 | 10 kΩ to 3.3 V; otherwise unconnected |
| DAT3 | 10 kΩ to 3.3 V; otherwise unconnected |

CLK connects to GPIO12 without a pull-up. Card VCC is 3.3 V only. Source: [ESP32-S3 SD pull-up requirements](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/sd_pullup_requirements.html).

## Headphone signal path

The PCM5102 breakout produces line level and explicitly requires a load of at least 1 kΩ. Do not plug headphones into its jack.

For the TI TPA6132A2EVM2:

- connect ground before VDD;
- power VDD from 3.3 V;
- connect PCM5102 left/right analog pads to `INL`/`INR`;
- install JP1 and JP2 shunts for single-ended input;
- select 0 dB gain (`G1=0`, `G0=1`);
- begin with the source muted and digital volume at zero;
- connect headphones only to the EVM output jack.

The official EVM guide permits 2.5–5.5 V and documents those jumper settings. Do not replace it with an unidentified marketplace module. `docs/headphone-prototype-options.md` selects the documented Adafruit 6309 TLV320DAC3100 breakout as a provisional separate headphone-test path; it requires its own reviewed wiring and ESP-IDF I2C driver and does not validate the PCM5102 plus TPA6132A2 chain. An open MIT TPA6132A2 breakout remains a fabrication alternative only after schematic and assembly review.

## Speaker signal path

The MAX98357A `+` and `-` outputs form a bridge-tied load. Connect them only across the 8 Ω speaker. Neither terminal may be grounded, probed with a ground-referenced oscilloscope clip, or fed into another amplifier.

Start with `SD` low, software volume at zero, and the breakout powered from a current-limited 5 V source. Enable only after clocks and zero PCM data are stable. The default board configuration mixes left and right to mono.

## Voltage domains

| Domain | Nominal | Loads | Rule |
|---|---:|---|---|
| USB/bench | 5 V | DevKitC input; MAX98357A VIN | current-limited during staged bring-up |
| Logic | 3.3 V | SD; OLED; PCM5102; TPA6132A2 EVM; GPIO | never expose raw SD to 5 V |
| I2S/I2C/GPIO | 3.3 V logic | all digital signals | no level shifting required for selected boards |
| PCM analog | ground-centered line level | DAC to headphone EVM | keep short and away from speaker output wiring |
| Speaker BTL | switching differential output | 8 Ω speaker only | neither lead is ground |

## Planning current envelope

These ranges size the bench source and help catch gross wiring faults. They are not measurements and must not be reported as results.

| Load/state | Expected planning range |
|---|---:|
| DevKitC, radios disabled | 50–150 mA from 5 V |
| microSD active reads | 20–100 mA at 3.3 V, with possible higher bursts |
| PCM5102 breakout active | 15–30 mA at 3.3 V |
| TPA6132A2 EVM | 2–5 mA idle; roughly 5–40 mA playing depending on load |
| OLED, content dependent | 10–40 mA at 3.3 V |
| MAX98357A and 8 Ω speaker | 2–10 mA idle; roughly 50–300 mA at quiet/moderate output; higher peaks possible |
| Whole prototype | roughly 80–220 mA idle; 150–700 mA during speaker playback |

Use a source capable of at least 1 A continuous; 2 A gives margin. Set a lower current limit for each isolated first-power stage, then raise it only when the observed draw is understood.

## Unresolved physical facts

Before board-specific firmware is merged, record photographs and labels for every actual module, the exact DevKitC revision, microSD make/model, headphone impedance, power-source arrangement, OLED I2C address, and whether the PCM5102 `MUTE` and MAX98357A `SD` pins behave as documented.
