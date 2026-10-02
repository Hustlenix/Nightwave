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
- Bounded LRC sorting/offset/multi-tags/duplicate timestamps/UTF-8, malformed and
  overlong documents, local M3U root containment, ID3 UTF-8/UTF-16 and WAV duration;
  1000 parser/fuzz/shuffle operations and sleep-timer unsigned-clock rollover.
- New real-frontend metadata/lyrics/settings/sleep/M3U controls with fake playback,
  sample clock freeze/direct WAV/MP3 decode-discard seek in real streaming tests,
  and actual checked NVS blob serialization/corruption handling.

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

CI supplies full C++/ESP-IDF evidence; no Windows C++ compiler is on PATH.
Ubuntu WSL has g++ but no CMake; targeted frontend/ASAN tests can also run there.
Execution records below must name a successful commit/run,
not merely the presence of workflow files.

Executed evidence at `13ed6be` (2026-10-01):

- [ESP32-S3 firmware build](https://github.com/Hustlenix/Nightwave/actions/runs/36864360208): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36864360149): five test executables, 5/5 passed.

At `a2aa741`, [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36865138075)
ran all six executables successfully, including actual OLED/NVS adapters against
mock APIs; [firmware build](https://github.com/Hustlenix/Nightwave/actions/runs/36865138056)
also passed.

At `6349cd0`, [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36865602237)
passed all six suites in Release and again with AddressSanitizer and leak
detection (6/6 in each configuration). This includes the decoder's corrupt
corpus and actual streaming/frontend/adapter sources. No sanitizer finding was
reported by that run; this is not proof against every possible malformed file
or target-only race.

At `ea393ac`, [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36874323047)
passed repository/BOM/wiring checks, the 17-task handoff structure check,
engineering-calculation self-tests, and all six suites in both Release and
AddressSanitizer with leak detection. This validates software and calculated
screening examples, not schematic/footprint correctness or physical behavior.
Device firmware source is unchanged from the previously successful device build.

At `35b7ab4`, [device build](https://github.com/Hustlenix/Nightwave/actions/runs/36878328683)
and [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36878328795)
succeeded, with 7/7 Release and 7/7 AddressSanitizer/leak suites. Initial
54ba552 exposed a clipped corrupt-path error title; fixed to show the filename.
The later playlist-resume test exposed a noncanonical CTest fixture root
(tests/../) conflicting with the canonical SD-root contract. Test roots now use
weakly_canonical, and suite-owned generated playlists are cleared before each
run to make Release/ASAN reruns independent. No user media is removed.

Current UI lyrics/queue allocate explicitly in S3 PSRAM after boot; host tests
allocate them on the heap. The radio/display/power/load limits and unfinished
expanded-scope features are documented in engineering-budgets.md. A passing
build/sanitizer result does not establish physical sync, timing or production
readiness. Performance counters saturate at UINT32_MAX; discard derived averages
after saturation. Stack minima are meaningful only after workers report them.

At `50df90c`, [ESP32-S3 build plus size/component reports](https://github.com/Hustlenix/Nightwave/actions/runs/36882011549)
and [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36882011690)
passed. All seven suites passed in Release and ASAN with leak detection,
including canonical-root M3U resume, isolated test fixtures and heap UI assets.
Targeted frontend and media suites also ran under ASAN/leak detection in Ubuntu
WSL; both passed. At `92c17ea`, [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36882687260)
passed again after diagnostic-parser array/saturation validation fixes. Firmware
source is unchanged from 50df90c. Local project/calculation/parser self-tests pass.

2026-10-02 catalog continuation: targeted actual catalog + frontend sources
compiled in Ubuntu WSL with g++17, -Wall/-Wextra/-Wpedantic/-Werror and ASAN/leak
detection, and both executed successfully. Coverage includes 262-track catalog
and 260-track UI fixtures, paging, groups, full queues, repeat/seek, context
resume, backup fallback, corrupt checksums/paths, limits and 1,000 query/cancel
cycles. Repository validator and diff whitespace checks also passed. A new
device build and full eight-suite Release/ASAN CI run are required; their actual
results will be recorded below after execution. No physical acceptance inferred.
