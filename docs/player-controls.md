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
| Hold Next | Current/next LRC view | Current/next LRC view |
| Hold Volume - | Settings | Mode / sleep / saved resume / output settings |

In Settings, Previous/Next selects a row and Play cycles normal/shuffle/repeat-all/
repeat-track, sleep off/15/30/45/60/end-track, explicitly resumes saved music, or
cycles auto/speaker/wired output. Bluetooth is unavailable until architecture
selection; selecting unavailable BT via API is refused without speaker fallback.
Two additional settings rows open LIBRARY and rebuild its SD-backed index
(stop playback first). Library has Songs, Artists, Albums and Folders/Playlists.
Previous/Next crosses 16-row catalog pages; Hold Previous goes up. Group Play
opens its songs, and song Play starts the full collection/filter queue. See
[catalog limits and timing](library-index.md); filtered queries are incremental,
not yet production-fast on a 10,000-track card.

Short actions occur on debounced release; holding suppresses the short action.
When the screen sleeps after 30 s, the first gesture only wakes it. No-SD/mount
error screen offers Play to retry. A corrupt file or I2S fault offers Next or
hold Play to recover. Track changes stop and wait for all three workers before
starting another file; a timed-out stop refuses restart and retains resources.

The browser lists directories before supported files, skips hidden entries and
unsupported extensions, and limits a folder to 128 entries and paths below
256 bytes. It explicitly displays a limit warning. Navigation is per-folder;
the separate catalog recursively indexes up to 10,000 tracks with explicit
depth/entry/directory bounds. A folder scan examines at most 512 directory entries.
Normal EOF stops at the last queued
track; repeat/shuffle modes change automatic advancement. Playlist entries form
an independent play queue that browser refreshes do not overwrite. ID3/WAV INFO
provide title/artist/album when supported, otherwise filename fallback. WAV
duration is computed; MP3 duration remains unknown. The ASCII OLED renderer
uppercases lowercase and substitutes unsupported characters; source paths are
not modified by display truncation.

Headphone detect uses active-low GPIO16 with 50 ms stability. The actual jack
switch wiring must implement that polarity; unconnected prototype input defaults
to speaker. A raw switched audio contact is not automatically a safe digital
detect signal. Routing is break-before-make; plug transients require bench tests.

Volume/mode/output, last path/position and playlist/folder strings persist in a
versioned length/checksum-validated NVS record after 2 s without change. Playback
checkpoints position once per minute; button pause and explicit stop also save.
Boot never auto-plays. Resume explicitly restores saved M3U context if available,
otherwise reconstructs a canonical folder queue when possible, or restores
indexed collection/artist/album context incrementally from a valid cache.
Missing/unusable context falls back to the single saved track. Browser cursor
restoration and fast MP3 seeking remain open.
Default 8% is intentionally
low, not a guaranteed acoustic-safe level. NVS errors leave settings volatile;
firmware does not erase the partition silently. Legacy volume-only storage is
migrated into the new record when it is saved.

LRC uses the same media stem, current/next timestamp lookup, stable sorting,
multi-tags and bounded offset. Missing/invalid LRC clears lyric state but does
not stop valid music. Lyrics follow accepted PCM position, not wall time. I2S
DMA lead and 5 Hz display refresh mean acoustic sync is still unverified.
Sleep fades/holds playback, stops after a 200 ms control delay and sleeps the
screen; it does not yet shut down a physical power latch.

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
`volume 0..100`, `stop`, `status`, `mode normal|shuffle|all|track`,
`sleep off|15|30|45|60|end`, `seek 0..1800000`, `last`, `selftest`, `help`.
WAV seek is direct; MP3 seeks by cancellable decode/discard from track start,
capped at 30 minutes. No fast-seek index is claimed. Boot/status/selftest emit
schema-1 JSON records; tools/analyze_diagnostics.py analyses real captured logs
without inferring physical acceptance from a software self-test.

UI and console commands are serialized. Storage/decoder/audio workers never
wait on the UI/console mutex. Tone and SD benchmarking are diagnostics, not
normal product controls. Follow prototype wiring/bring-up before enabling
outputs; headphone path hardware remains unfinished.

Protocol references:
[SH1106 manufacturer datasheet](https://buydisplay.com/download/ic/SH1106.pdf),
[ESP-IDF I2C API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html).
