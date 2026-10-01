# Digital software validation

No physical playback, timing, fit, current, temperature or battery result is
claimed by these tests. Native device build and host simulations are different
evidence from a bench test.

The serial/player path uses `StreamingPlayer`: StorageTask -> 32768-byte SPSC
encoded ring -> DecoderTask -> 16384-frame stereo PCM ring -> AudioOutputTask
-> I2S DMA. Buffers live off worker stacks. The older two-task WavPlayer remains
a historical bring-up implementation and is not the active product path.

Helix 1.0.3 handles CBR/VBR Layer III; bounded ID3v2 skipping, ID3v1 tail skip,
framing, corrupt-frame recovery and typed EOF/errors surround the dependency.
Supported output rates are 22.05/32/44.1/48 kHz; mono duplicates to stereo. Free
format, MPEG2.5, unsupported rates and mid-track rate changes are rejected.
Rate changes between tracks reconfigure the sink after teardown. Tags above
16 MiB are rejected; resync/decode-error recovery is limited to 8192 bytes per
track. There is no unbounded retry of an unchanged compressed frame.

The output worker owns gain ramping, pause fade/drain/mute, resume fade, route
changes and teardown. Natural EOF drains DMA with silence. Errors cancel both
producers; completion publishes only after resources are closed. A bounded
stop timeout is not permission to delete live resources or start a new track.

## Host regression coverage

- WAV bounds, RIFF chunks, unsupported formats, generated PCM fixture.
- MP3 stereo CBR, mono VBR and MPEG2 mono, fragmented input, ID3 and truncated
  frames; deterministic corrupt byte corpus with bounded progress.
- Real StreamingPlayer with host-thread RTOS and simulated I2S: 1000 alternating
  WAV/MP3 starts/cancels, overlap refusal, pause/resume control cycles, an observed
  held/muted pause, resume, EOF, output fault/recovery, partial task creation and
  delayed-stop retention. Those are simulated scheduling tests, not 1000 physical
  track-change measurements.
- SPSC full/wrap/empty plus 100000 ordered transfers across two host threads;
  overflow-safe mono mixing, Q15 volume and ramp endpoints.
- Real PlayerFrontend with fake devices and actual host directory scans:
  folders, five-button commands, file changes, corrupt recovery, route indicator,
  delayed setting writes, diagnostic view, sleep/wake and mount retry.
- Actual I2S adapter with mocked ESP driver: configuration, preload, silence
  drain, write errors and output-enable behavior.
- Actual OLED/NVS adapters with mocked APIs: SH1106 page/offset versus SSD1306
  window addressing, render bytes, sleep/wake, bus errors/cleanup, volume
  persistence, malformed stored values and open/set/commit/init failures.

CI generates its own WAV tones and encodes MP3 with FFmpeg; no songs are fetched.
Host Helix math substitutes equivalent C primitives; device builds use Xtensa
source. Read [dependency terms](decoder-licenses.md).

## Reproduce on Linux

```
python3 tools/validate_project.py
python3 tools/power_budget.py
python3 tools/generate_test_media.py --output .generated/test-media
# Encode the three original MP3 fixtures exactly as project-quality.yml does.
cmake -S tests -B build/host-tests -DCMAKE_BUILD_TYPE=Release
cmake --build build/host-tests --parallel
ctest --test-dir build/host-tests --output-on-failure
```

CI is the executed C++/ESP-IDF evidence in this Windows run; no local C++
compiler is on PATH. Execution records below must name a successful commit/run,
not merely the presence of workflow files.

Executed evidence at `13ed6be` (2026-10-01):

- [ESP32-S3 firmware build](https://github.com/Hustlenix/Nightwave/actions/runs/36864360208): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36864360149): five test executables, 5/5 passed.

At `a2aa741`, [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36865138075)
ran all six executables successfully, including actual OLED/NVS adapters against
mock APIs. A further AddressSanitizer pass is being added and is not claimed
complete until its successful run is recorded.
