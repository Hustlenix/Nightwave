# Prototype Plan

## Objective

Prove direct storage + software decode + buffered DMA I2S playback before final schematic/PCB/CAD decisions.

## Baseline bench system

```text
USB/bench power
      |
ESP32-S3-DevKitC-1-N8R8
  |-- microSD breakout (initially 1-bit SDMMC if breakout permits)
  |-- PCM5102A I2S DAC breakout -> stereo headphone amplifier breakout -> headphones
  |-- MAX98357A I2S speaker-amplifier breakout -> 8 Ω speaker
  |-- 1.3 in I2C OLED
  +-- five normally-open buttons to GPIO/GND
```

Lithium power is deliberately excluded from the first audio proof. Use USB/bench power only.

## Candidate prototype parts

The exact dated order list is [`hardware/prototype-BOM.csv`](../hardware/prototype-BOM.csv), the connection table is [`hardware/prototype-wiring.csv`](../hardware/prototype-wiring.csv), and the staged procedure is [`prototype-bringup.md`](prototype-bringup.md).

| Subsystem | Candidate | Voltage/interface | What it proves | What it does not prove |
|---|---|---|---|---|
| MCU | ESP32-S3-DevKitC-1-N8R8 | USB 5 V; 3.3 V GPIO | toolchain, PSRAM, tasks, GPIO, I2S/SD timing | final power/layout/antenna/size |
| storage | SparkFun BOB-00544 raw 3.3 V socket plus five 10 kΩ pull-ups | SDMMC 1-bit | mount/read/latency | no mechanical card detect; final socket/removal behavior |
| DAC | Adafruit 6250 PCM5102 breakout | shared I2S, 3.3 V | stereo DAC clocks and line audio | custom analog layout/noise |
| headphone amp | TI TPA6132A2EVM2 | analog stereo, 3.3 V | safe headphone drive, gain, mute/pops | final QFN layout/ESD/jack detect; procurement remains gated |
| speaker amp | Adafruit 3006 MAX98357A breakout | shared I2S, 5 V | mono mix/channel mode, speaker playback | final MAX98360C package/EMI/layout |
| display | Adafruit 938 1.3 in SSD1306 breakout | I2C 3.3 V | bench UI scheduling/basic text | final BD-15 TFT driver, readable lyrics, connector/current/fit |
| controls | five momentary buttons | GPIO to GND with pulls | debounce/UX event model | final switch height/ergonomics |
| speaker | Adafruit 6486 8 Ω/2 W bench speaker | differential Class-D | load/audio/acoustic experiments | final CMS-28528N-L152 cavity and grille |

## Bring-up sequence

1. Confirm exact development-board and breakout markings; do not infer pinouts from similar boards.
2. Run button and display tests.
3. Mount a known-good FAT32 card and measure sequential throughput/worst read latency.
4. Generate a digital I2S tone; verify stereo L/R at DAC/headphone path at low volume.
5. Verify speaker amplifier with the differential output connected only to the speaker.
6. Add WAV PCM streaming through both ring buffers.
7. Add MP3 frame decode and capture timing/heap/buffer diagnostics.
8. Stress skip/pause/sample-rate changes/SD removal.
9. Run a two-hour USB-powered playback test and record actual underruns/resets.

## Required measurements before Phase 6

- SD sequential throughput and maximum observed read latency across several cards;
- decode median/p95/max per frame for representative MP3s;
- PCM minimum watermark and underruns during UI and skip stress;
- free/minimum heap, PSRAM use, and task stack watermarks;
- headphone L/R identity, noise, gain and pop behavior;
- speaker audibility, distortion/rattle observations, and amplifier temperature;
- USB-powered current for idle/display/headphone/speaker conditions.

## HUMAN GATE

### Action
Obtain the exact prototype parts, wire the bench system from a reviewed pin-by-pin wiring table, and run the independent peripheral tests.

### Why
Purchasing, physical wiring, listening, and electrical measurements cannot be completed digitally.

### Procedure
Do not connect a lithium battery. Use USB or a current-limited bench supply. Verify module labels and grounds before power. Start with MCU/display/buttons, then SD, DAC/headphones at minimum volume, and finally the Class-D speaker path. Never connect either Class-D speaker output to ground.

### Expected result
Each peripheral enumerates/responds independently; I2S test tones reach the correct channel; no rail overheats or collapses.

### STOP if
There is reversed polarity, visible damage, smoke/odor, unexpected heating, unstable rail voltage, excessive current, or uncertainty about a breakout pinout.

### Return to Work
Provide clear photos of exact module labels and wiring, board revision, power source, console logs, measured rail voltages/current, and observed channel/audio behavior. Do not share credentials.
