# Phase 3 firmware bring-up

This is the exact current digital handoff. The firmware builds for ESP32-S3 in CI, but no physical command below has been run on the user's hardware yet.

See `docs/audio-validation.md` for the Phase 4 entry audit and prerequisite
repairs. Use the repaired build for the first WAV bench test. On the N8R8
baseline, verify 8 MB flash in menuconfig if reusing an older sdkconfig.

## Build and flash

Use ESP-IDF 6.1 and the USB-to-UART connector on the confirmed `ESP32-S3-DevKitC-1-N8R8`:

```powershell
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p COM_PORT flash monitor
```

Replace `COM_PORT` with the observed port. Do not connect a battery. Start with every peripheral disconnected and power only from USB or a current-limited 5 V bench source.

## Expected DevKitC-only log

The first valid boot contains lines equivalent to:

```text
diagnostics: BOOT OK
diagnostics: project=nightwave version=... idf=v6.1...
diagnostics: target=ESP32-S3 revision=... cores=2 flash=... reset_reason=...
diagnostics: heap_free=... heap_min=... psram=...
buttons: BUTTONS READY ...
storage: SD mount failed: ...
nightwave: SD unavailable; console and button diagnostics remain active
nightwave>
```

Return the complete boot log, not just `BOOT OK`. A mount failure is expected when SD is not connected. Stop if there is a brownout/reset loop, unexpected heat, smoke, or a rail fault.

## Button test

With power off, wire each normally-open button from its GPIO to ground according to `hardware/prototype-wiring.csv`. Reapply power, press and release every button ten times, and capture lines such as:

```text
buttons: BUTTON PLAY_PAUSE PRESSED t_ms=...
buttons: BUTTON PLAY_PAUSE RELEASED t_ms=...
```

Expected mapping is Previous GPIO1, Play/Pause GPIO2, Next GPIO4, Volume Down GPIO10, Volume Up GPIO15. The software debounce is an unmeasured 25 ms starting value; return any missed/double events rather than tuning by guess.

## SD test

Power off before attaching or removing the raw SD breakout. Follow `docs/prototype-wiring.md`, including all five required 10 kohm pull-ups and 3.3 V-only power. Copy `.generated/test-media/*.wav` to the FAT32 card root.

At the prompt:

```text
sd
bench /sdcard/02_left_right_stereo_44100.wav
status
```

Expected evidence includes card identity/capacity from ESP-IDF, the enumerated audio files, and one line containing `SD BENCH bytes=... elapsed_ms=... avg_kib_s=... worst_read_us=... errors=0`. Repeat across three cold boots, then perform the five-minute read procedure in `docs/prototype-bringup.md` before calling SD stable.

## I2S tone test

The boot state keeps GPIO17 and GPIO18 low and does not initialize I2S. Do not use `tone line` until the exact line/headphone module and enable polarity are confirmed. For the documented MAX98357A speaker breakout, power off, follow the wiring table, ensure neither BTL speaker lead is grounded, and start with a current-limited 5 V supply.

Then run:

```text
tone speaker 440
```

The firmware configures 44.1 kHz, 16-bit stereo I2S with 8 x 256 DMA frames, writes zero PCM, enables the speaker path, generates a two-second ramped tone at 1,200 counts peak (about 3.7% full scale), writes silence, disables the amp, and tears down I2S. Return the log plus a description/video of actual behavior. Stop on severe pops, reset, excessive current, or heat.

## WAV test

After SD and tone pass:

```text
wav speaker /sdcard/01_silence_stereo_44100.wav
status
wav speaker /sdcard/02_left_right_stereo_44100.wav
status
```

The parser accepts RIFF/WAVE PCM, 16-bit mono/stereo, and 22.05/32/44.1/48 kHz; it skips bounded unknown chunks and rejects inconsistent/truncated files. Playback uses a storage task, a 16,384-frame PCM ring, an audio task, and I2S DMA. Any `AUDIO UNDERRUN` line is a test failure to investigate, not a message to hide.

Use `stop` to request cancellation. Normal cleanup mutes and stops I2S after
the producer closes its file. If `STOP PENDING` appears, cleanup has not finished;
the player retains live resources and refuses another track. Capture the log
and retry `status`/`stop`; do not remove the card or start a new hardware test.
Card removal recovery has not been implemented or tested.

## Evidence to return

- clear photos of both sides of the DevKitC and every connected module;
- board/module labels and revisions;
- microSD make, model, capacity, and FAT format;
- power source/current limit and any observed current;
- complete boot, button, SD benchmark, tone, WAV, and status logs;
- whether each physical sound was silent/correct/wrong, with no inferred measurements;
- exact wired headphone model and impedance before headphone testing.

