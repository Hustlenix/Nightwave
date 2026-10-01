# USB-only software bring-up gate (not executed)

This is optional prototype verification after the builder has real documented
modules. It does not author the final design or waive Bluetooth/display choices.
No lithium battery, charger experiment or fabricated measurement is permitted.

## Objective

Verify SD/decode/sample clock/LRC/modes/settings/sleep on the existing S3 bench
prototype before chosen-radio/TFT/power integration.

## Exact wiring

Use hardware/prototype-wiring.csv and docs/prototype-wiring.md unchanged:
SD CLK12/CMD11/D0 13 (required pull-ups), I2S BCLK5/WS6/DATA7,
I2C SDA8/SCL9, buttons1/2/4/10/15, speaker enable17. Verify every module's
pin labels and voltage before power. USB/current-limited bench only. PCM5102A
is line-level, not a headphone driver; use the reviewed headphone amplifier
plan. MAX98357A speaker outputs are BTL: neither speaker lead goes to ground.
GPIO16 detect is provisional; no raw audio switch into GPIO without review.

## Firmware commit

Flash only a source revision with successful device CI recorded in
software-validation.md. Initial new-feature validated source is 35b7ab4;
later source supersedes it only after its own successful build. Record the exact
commit, binary/hash, tool version, module variant and boot JSON, not just "latest".

## Commands

Generate original audio: `python tools/generate_test_media.py --output .generated/test-media`.
Put its WAV on SD with a user-authored synthetic same-stem .lrc such as
`[00:00.000]First test line` / `[00:01.000]Second test line`.
Use `board`, `sd`, `selftest`, `play speaker /sdcard/02_left_right_stereo_44100.wav`,
`status`, `pause`, `status`, `resume`, `mode all`, `sleep end`, `status`.
Test wired line path only with suitable amplifier/load and low volume. Test
M3U, missing/corrupt file, long title, stop/restart, modes, explicit `last`
after reset, and 15-minute timer before long unattended runs.

## Expected behavior

Sample position advances with accepted media, freezes after pause fade/drain,
lyrics use that clock, EOF respects selected mode; timer mutes/stops and sleeps
screen. Bad sidecar clears lyrics without stopping valid music. Self-test reports
Bluetooth pending, battery unmeasured and physical_pass false. No auto-blasting
resume at boot. `last` explicitly resumes; MP3 seeking can take noticeable time.

## Measurements

Record SD average/worst/throughput, instrumented decoder average/worst, queue
low-water/underruns/errors, free/minimum/largest internal heap, PSRAM, stack
minima, reset cause, elapsed pause/drain, lyric/audio offset and USB current.
Use `python tools/analyze_diagnostics.py measurements/your-real-log.txt` for
JSON analysis, retaining the original log and scope/serial captures. Annotate
all synthetic host data. Real battery runtime/temperature/charging is a separate
reviewed power gate, not this test.

## STOP conditions

Wrong voltage/pin/polarity, hot part/USB current limit, unstable rails/resets,
speaker BTL grounded, audible pop/excessive level, decode/output errors, NVS
failure, stuck stop or unsafe charger/battery connection. Mute and remove bench
power safely; do not repeatedly reconnect lithium or force restart live workers.

## Return files/logs

Commit/hash, exact modules/wiring photographs, complete serial JSON log, SD/media
file names and hashes, measured readings with instrument/setup, observed versus
expected behavior and STOP incidents. No photos/results are currently on file.
