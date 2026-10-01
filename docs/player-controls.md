# OLED + five-button interaction

Implemented digitally; no screen/button on a board has been physically verified.
Default driver targets the provisional SH1106 128x64 display at I2C 0x3c, with
two-column offset. SSD1306 is a distinct supported driver option, not an assumed
drop-in protocol. Exact display supply, address, reset and mounting remain BOM
verification items. External I2C pull-ups to 3.3 V are required.

| Control | Browser | Playing / paused / error |
| --- | --- | --- |
| Previous / Next short | Move cursor with wraparound | Previous / next file in current folder |
| Play/Pause short | Open folder or play file | Toggle pause; when stopped return to browser |
| Volume - / + short | Change volume by 2% | Same |
| Hold Play/Pause (700 ms) | Return to now-playing if active | Open/refresh browser |
| Hold Previous | Parent directory | Parent directory/browser |
| Hold Volume + | Diagnostics | Diagnostics |

Short actions occur on debounced release; holding suppresses the short action.
When the screen sleeps after 30 s, the first gesture only wakes it. No-SD/mount
error screen offers Play to retry. A corrupt file or I2S fault offers Next or
hold Play to recover. Track changes stop and wait for all three workers before
starting another file; a timed-out stop refuses restart and retains resources.

The browser lists directories before supported files, skips hidden entries and
unsupported extensions, and limits a folder to 128 entries and paths below
256 bytes. It explicitly displays a limit warning. Navigation is per-folder;
there is no whole-card recursive index, shuffle, seek or repeat UI yet. File
names provide track titles, not parsed ID3 metadata. The ASCII OLED renderer
uppercases lowercase and substitutes unsupported characters; source paths are
not modified by display truncation.

Headphone detect uses active-low GPIO16 with 50 ms stability. The actual jack
switch wiring must implement that polarity; unconnected prototype input defaults
to speaker. A raw switched audio contact is not automatically a safe digital
detect signal. Routing is break-before-make; plug transients require bench tests.

Volume is persisted in NVS after 2 s without change. Default 8% is intentionally
low, not a guaranteed acoustic-safe level. NVS errors leave settings volatile;
firmware does not erase the partition silently. Other legacy Settings fields
(shuffle/repeat/resume) are not persisted or exposed as implemented features.

Battery displays `BAT ?` / `BATTERY UNMEASURED`; there is no fabricated SOC,
fuel-gauge readout, charge indicator, power-latch or low-battery shutdown claim.
The existing power-state interface is reserved for the reviewed final circuit.

Boot, browser, now-playing, no-SD, corrupt/error, route indicator, volume,
display sleep and diagnostics are present. Diagnostics show encoded/PCM depth,
underruns, errors, worst instrumented read/decode times, heap and PSRAM. Actual
values require hardware; host fake values are not measurements.

## Bench-only serial commands

`board`, `sd`, `bench [path]`, `tone speaker|line [Hz]`,
`play speaker|line /sdcard/file.wav|mp3` (`wav` alias), `pause`, `resume`,
`volume 0..100`, `stop`, `status`, `help`.

UI and console commands are serialized. Storage/decoder/audio workers never
wait on the UI/console mutex. Tone and SD benchmarking are diagnostics, not
normal product controls. Follow prototype wiring/bring-up before enabling
outputs; headphone path hardware remains unfinished.

Protocol references:
[SH1106 manufacturer datasheet](https://buydisplay.com/download/ic/SH1106.pdf),
[ESP-IDF I2C API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html).
