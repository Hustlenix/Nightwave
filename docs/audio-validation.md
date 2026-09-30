# Audio validation and Phase 4 entry gate

Audit date: 2026-09-30.

## Current result

**Phase 4 has not started.** The Phase 4 request explicitly requires physical
Phase 3 evidence before entry. The repository contains no returned device logs,
build photos, or measured audio results. Both existing measurement CSVs contain
headers only. Firmware and project-quality CI passed for `a66576a`; this establishes
build/host-test health, not physical playback.

| Entry evidence | Verified result |
|---|---|
| Exact ESP32-S3 board and connected audio hardware | Not provided |
| Device boots; serial console responds | Not physically verified |
| Real SD mounts and files enumerate | Not physically verified |
| I2S tone reaches the speaker | Not physically verified |
| Supported WAV plays on that speaker | Not physically verified |
| MP3 plays or remaining blocker is understood | MP3 decoder is not integrated; integration awaits physical WAV evidence |
| Power source | USB/bench power is specified; actual source not provided |
| 30-minute / two-hour stability | Not run |

## Phase 3 preflight repairs

Source inspection before the bench handoff found prerequisite defects:

- A 4096-byte automatic read array occupied a task with a 4096-byte stack.
  The array now belongs to the player object, leaving stack space for FatFS,
  logging, and calls. The workers report minimum free stack on exit; actual
  stack adequacy still needs device measurement.
- Workers could execute before both task creations succeeded. Both now wait
  for a notification before touching track resources, so creation rollback
  can safely close the file and destroy I2S.
- Audio errors could leave the storage worker stuck on a full ring. The audio
  worker now cancels the producer and waits for producer cleanup before
  publishing idle. `stop` refuses to tear down live worker resources if its
  one-second wait expires; a new track remains refused while cleanup is pending.
- The defaults specified 16 MB flash despite the documented N8R8 development
  board. Defaults now specify 8 MB flash. Exact hardware must still be confirmed.
- Short WAV reads now produce an explicit error and cancel playback instead
  of being mistaken for successful EOF.

These repairs do not establish production engine reliability. There is still
no MP3 decoder, compressed byte stage, pause/resume implementation, or track
controller in the device build. The standalone host state-machine tests do not
prove those playback features.

ESP-IDF documents task stack sizing/watermarks in bytes in its
[RAM usage guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/ram-usage.html).

## HUMAN GATE: first returned Phase 3 evidence

### Objective

Verify actual board boot, serial diagnostics, SD access, tone, and WAV output.
Provide existing results first if these tests have already been performed.

### Wiring/configuration

Use the exact confirmed board and breakouts from `hardware/prototype-wiring.csv`.
The documented baseline is ESP32-S3-DevKitC-1-N8R8, SparkFun BOB-00544 raw SD,
Adafruit MAX98357A 3006, and an 8 ohm speaker. Confirm received labels/revisions
before wiring. Use USB/bench power without a lithium battery. The five external
SD pull-ups and 3.3 V SD power are required. Both speaker leads connect only to
the amplifier's BTL outputs.

### Firmware commit/build

Use the commit containing this preflight repair and its passing Firmware CI run.
The startup `project=nightwave version=...` line records the built Git revision.
Build with ESP-IDF 6.1, target `esp32s3`, and the 8 MB flash defaults for N8R8.
Do not use an old generated sdkconfig retaining the 16 MB setting: verify
`Flash size = 8 MB` in menuconfig before flashing the N8R8 baseline.

### Procedure

Follow `docs/phase3-firmware-bringup.md` in order. First boot the DevKitC alone
and save the complete serial output. Confirm the board label and observed
flash/PSRAM capacities. After the documented unpowered wiring checks, copy
the generated WAV fixtures to a FAT32 card and run:

```text
board
sd
bench /sdcard/02_left_right_stereo_44100.wav
tone speaker 440
wav speaker /sdcard/02_left_right_stereo_44100.wav
status
stop
```

Wait for each tone/file to finish before the next playback command. Capture
button presses too if the buttons are already connected. Leave headphones
disconnected for this first speaker test.

### Expected result

`BOOT OK`, correct board diagnostics, file enumeration, an error-free SD
benchmark, a quiet two-second tone, WAV playback, and a responsive console.
The speaker mixes stereo; it cannot prove headphone left/right separation.
No task-stack panic, reset loop, or unexplained underrun is acceptable evidence
of a successful first pass.

### What to measure

Record actual card identity, benchmark throughput/worst read latency/errors,
WAV underruns/errors, and worker stack watermarks. Return any observed current
only with its supply conditions and instrument. No measurement rows are filled
until the corresponding device log or instrument result exists.

### STOP if

The device resets unexpectedly, the stack checker panics, wiring disagrees with
the exact module, the rail is shorted, a module heats unexpectedly, or audio
produces severe pops. Power off before rewiring or removing the SD card.

### Return to Work

Exact board/audio-module labels, clear front/back photos, SD make/model/capacity,
power-source details, firmware version, complete serial log, and actual audible
observations. Failed logs are useful too. Once real WAV playback is understood,
finish the MP3 prerequisite; then evaluate Phase 4 entry again.
