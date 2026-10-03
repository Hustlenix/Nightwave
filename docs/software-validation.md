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

The initial merged checkpoint adbb92e failed its optimized Release compilation
in run 36962394436: GCC inlining reported possible snprintf truncation in the
cache-path helper. The join was replaced by checked lengths/memcpy; warning
policy was not weakened. Sanitizer success alone does not replace Release or
device compilation. Record the repaired run's results only after execution.

At 3ce6014 (run 36962585890), optimized compilation passed; 7/8 Release suites
passed. The new catalog location assertion used CTest's `tests/../` fixture path
instead of its canonical root. The fixture lookup now uses the canonical root,
matching the saved-path contract. ASAN was skipped by that failed run; it is
not recorded as a pass. Targeted WSL tests with canonical arguments had passed.

Validated checkpoint `14b10c02e433cf87f54a9f238603f0969b49ef90`:

- [ESP32-S3 build + size/components](https://github.com/Hustlenix/Nightwave/actions/runs/36962788706): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36962788677): success, 8/8 Release and 8/8 ASAN/leak suites.
- New frontend assertions verify catalog traversal does not advance while audio
  owns workers. The fake-audio suite does not prove on-device SD starvation bounds.
- Link report: DIRAM 204,882/341,760 bytes, remaining 136,878; .bss 144,960;
  image 437,340 bytes. Not runtime heap/PSRAM/CPU measurements.
- The device build includes upstream ESP-IDF Wi-Fi/supplicant CMake dependency
  warnings; it successfully builds. No project warning was suppressed.
- Concurrent origin/main CI/setting-migration/website changes were preserved
  via merge adbb92e, not overwritten. The original adbb92e device run was
  superseded/cancelled; it is not counted as successful validation.

The subsequent c05ba8d documentation-only ledger left firmware/test source identical
to this validated checkpoint. The full product is still not funding-ready;
library-index.md records lookup/fallback performance and physical acceptance gaps.

2026-10-02 v2 lookup continuation: targeted WSL catalog tests passed with
optimized -O3/_FORTIFY_SOURCE=3 and -Wall/-Wextra/-Wpedantic/-Werror, and again
with AddressSanitizer/leak detection. The catalog builds actual 10,000-file
metadata-only fixtures, checks zero-read group pages, ≤16-read filtered pages,
one-read selection/correct resume hints, stale hints, malformed footers,
1,024/1,025-group boundaries, legacy migration and 1,000 query/cancel cycles.
The expanded frontend suite passed ASAN/leak detection with actual catalog/UI
sources and fake audio/devices, including cold/legacy/malformed resume without
boot autoplay. Lookup allocation is 227,488 bytes per generation (bounded
explicit PSRAM on target, host heap in tests), not internal static storage.
These operation counts are not measured SD/UI/acoustic latency. A new full
eight-suite Release/ASAN run and ESP32-S3 build are required for this source;
record their actual results below, not the older checkpoint's success.

Validated/pushed v2 library checkpoint df301661256671e3db892fd3cd6450fb65588c4f:

- [ESP32-S3 build](https://github.com/Hustlenix/Nightwave/actions/runs/36994661327): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36994661341): success; 8/8 Release and 8/8 ASAN/leak.
- Link DIRAM 204882/341760, .bss 144960, image 445484 bytes. Not runtime heap.
- Concurrent website-only origin commits were preserved by merge, not overwritten.

Next digital continuation adds idle MP3 frame-duration/seek caches, real-Helix
indexed-seek equivalence checks, injected display frames, bounded Bluetooth
source control, unknown-safe power interfaces and engineering JSON. Targeted
local index Release/ASAN-leak and real-streaming Release tests passed. The
15-second indexed seek matches decode-from-start PCM hash/sample count and uses
fewer decode calls/SD bytes. This is synthetic host output, not physical latency.
Full new-source CI/device results must be recorded separately after execution.

Validated/pushed digital source `8c7015e6aac9ec7024e46e5aa58eeba0afa8604b`:

- [ESP32-S3 build/size](https://github.com/Hustlenix/Nightwave/actions/runs/37002164509): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/37002164501): success, 10/10 Release plus 10/10 ASAN/leak suites; validator/arithmetic passed.
- MP3 index/streaming, display injection and BT/power/engineering HAL coverage
  are included, not inferred from the earlier eight-suite run.
- Link DIRAM 205394/341760, remaining 136366; .bss 145456; image 453300 bytes.
  These are link figures, not runtime free heap, stack/PSRAM or physical proof.
- Original b28228c host tests passed; its [device run](https://github.com/Hustlenix/Nightwave/actions/runs/37001601554)
  failed on initializer-list unsigned-int versus target uint32_t type deduction.
  A fixed-width array repaired it; warnings were not disabled. Website quality
  passed at b28228c. Failed build is retained, not counted as a pass.

Subsequent manufacturer-defined AT codec source requires its own run below;
neither wire-format tests nor this build proves wireless transmission.

Final firmware source `ee2f4fcf73614d2327ecfeccc653c36a05663846` is pushed:

- [ESP32-S3 firmware](https://github.com/Hustlenix/Nightwave/actions/runs/37003131904): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/37003131911): success; 10/10 Release and 10/10 ASAN/leak, validator/arithmetic passed.
- Includes the manufacturer-defined fixed AT command/status codec and 1000
  repeated valid/invalid status cycles, without enabling the radio backend.
- All large-library, settings reload/stale resume, corruption/capacity/migration,
  MP3 seek and display/BT/power/report tests ran in those suites. Indexed real-
  decoder PCM comparison is a generated-host fixture, not an acoustic result.

Later diagnostics-reader/documentation updates leave firmware/C++ tests identical
to ee2f4fc; their own project-quality run is recorded after execution. The reader
now validates engineering counters/flags/null battery fields, rejects overflow/
excessive nesting, and keeps physical acceptance NOT_ESTABLISHED even if a
captured record falsely says physical_pass=true.

Final diagnostics-reader checkpoint `3316ff55d83511158287a02ada794164c2e98f55`
is pushed. [Project quality 37003778650](https://github.com/Hustlenix/Nightwave/actions/runs/37003778650)
succeeded: validator, engineering/diagnostic parser self-tests, 10/10 Release
and 10/10 ASAN/leak suites. `git diff ee2f4fc 3316ff5 -- firmware tests` is empty,
so the successful ee2f4fc ESP32-S3 build applies to identical firmware/C++ sources.
This subsequent evidence-ledger-only commit changes no software source.

## Runtime checkpoint and finished-device continuation

Verified on 2026-10-03 for published source `8bb9232c2c0647568259bf5ba04034ea728a4218`:

- [ESP32-S3 firmware](https://github.com/Hustlenix/Nightwave/actions/runs/37021176284): success.
- [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/37021176476): success, 11/11 Release and 11/11 ASAN/leak suites, nine synthetic shipping-inventory tests and project/calculation checks.
- Includes the sampled runtime recorder and boot-lifetime counters. Synthetic
  playback windows are not physical battery runtime; unknown power keeps battery
  evidence unestablished.

The 2026-10-03 continuation adds a separate finished-device evidence inventory and
malformed-PCM Bluetooth fail-closed regression. Null/empty/oversized/mismatched
blocks must fault, disconnect once, invalidate the epoch and request a stop; no
invalid block reaches the backend. Targeted strict optimized and ASAN/leak HAL
tests passed in WSL. Full Release/ASAN and firmware CI for this changed source
must be recorded separately after execution. No real radio/backend is enabled.

Selected-product continuation records the builder's BD-15 Waveshare 24382 and
BD-16 TCA9535PWR choices. New display geometry/tile and input-expander adapter
tests pass targeted strict Release and ASAN with leak detection in WSL.
All 27 Python tests (9 funding, 12 Trial inventory, 6 power arithmetic) and the
project validator pass locally. The new source requires 13/13 full C++ suites
and its own ESP32-S3 CI build; the pending run must not be labelled successful.

Published engineering source `a7edc5f215600a4436387c0fcfee70ace1dda816` has
passed [Project quality 37126760412](https://github.com/Hustlenix/Nightwave/actions/runs/37126760412):
**13/13 Release, 13/13 ASAN with leak detection**, all 27 Python tests (9 funding,
12 finished-device inventory, 6 selected-product power), validator and engineering
calculations. CI logs were read on 2026-10-03, not inferred from queued status.
This includes 10,000-track artists/albums/filtered pages, corrupt/legacy cache,
group/low-memory fallback, stale resume, settings reload, malformed BT PCM,
selected-display geometry and actual ESP-IDF input adapter against host fakes.
[ESP32-S3 firmware 37126760414](https://github.com/Hustlenix/Nightwave/actions/runs/37126760414)
also completed successfully for that exact source, including compilation of the
new input adapter. No host/CI
result establishes physical radio/display/SD, battery runtime, CAD fit or authorship.

Follow-up inventory verification found a stale selection-label allowlist: it
recognized BD-14 but not actual BD-15/16 or retained-S3 labels. The checker now
accepts those explicit labels without accepting unknown decisions or treating
selection as completed pricing/review. A tenth funding regression preserves
missing-price/final-total blockers. All 28 Python tests pass locally.

Final verified software source `cb3db92af2a889a90981650fe004e3e03244d0a7`:

- [Project quality 37127240521](https://github.com/Hustlenix/Nightwave/actions/runs/37127240521): success; 13/13 Release, 13/13 ASAN/leak, 10 funding +12 Trial +6 power Python tests, project validator and calculations.
- [ESP32-S3 firmware 37127240508](https://github.com/Hustlenix/Nightwave/actions/runs/37127240508): success for that exact commit.
- Firmware/C++ sources remain identical to a7edc5f; the follow-up changes only the funding checker, its Python regression and documentation. This final ledger follow-up changes documentation only, not software/config/BOM/tests.
- Actual funding inventory remains BLOCKED with 34 findings and no final total. Finished-device inventory remains BLOCKED with 41 findings, runtime null and physical_pass=false. No fabricated evidence was added to make either inventory green.
