# Final-master execution checkpoint

2026-10-01. This document is a completion ledger, not a claim that everything
is finished. Existing history is preserved. No final hardware source was
AI-authored, no orders were placed and no human hours/results were invented.

| Requested feature | Actual checkpoint | Remaining acceptance |
| --- | --- | --- |
| Direct microSD MP3/WAV | Existing real three-task engine retained | Real SD/decoder/output latency and hot-remove recovery |
| Lyrics | Bounded LRC loader and current/next OLED view; sample clock | Acoustic alignment, wrapping/Unicode on chosen display |
| Metadata | ID3v2.3/v2.4/v1 and WAV INFO; PCM duration | MP3 duration/index, unsupported tag variants |
| Library | File/folder and M3U/M3U8; separate bounded play queue | Whole-card songs/artists/albums, incremental/paged index |
| Modes | Normal/shuffle/repeat-all/repeat-track; EOF advance | Long on-board mixed-media stress |
| Resume | Checked versioned NVS; explicit resume; M3U context | Fast MP3 seek, complete browser/folder context restoration |
| Sleep | Off/15/30/45/60/end-track; pause ramp, stop, display sleep | Measured low power / physical master shutdown |
| Outputs | Speaker/wired/auto preference, faded switch | Chosen Bluetooth transport; actual jack circuit |
| Battery | Unknown/unmeasured displayed honestly | Gauge/bus HAL, charging/fault/low-battery shutdown after power choice |
| Diagnostics | JSON boot/stream/performance/self-test + parser | Task CPU, route counters, BT/battery telemetry, physical evidence |
| Display | Provisional OLED plus new candidate comparison | Builder display choice, true TFT driver/UI/glyph testing |
| Engineering/manufacturing | Updated budgets and source-review gates | Complete BOM, builder sources, ERC/DRC/fit/exports, human review |

## Current validation

At 35b7ab4: [device build](https://github.com/Hustlenix/Nightwave/actions/runs/36878328683)
and [Project quality](https://github.com/Hustlenix/Nightwave/actions/runs/36878328795)
succeeded. Seven suites passed in Release and again with AddressSanitizer/leak
detection, including actual LRC/metadata/playlist sources and 1000 parser/shuffle
cycles. Original fixtures are generated locally/in CI, never copyrighted songs
or lyrics. Host scheduling/drivers are simulated, not hardware measurements.

Later source changes require their own successful run; see software-validation.md.
At 630e3ec device compilation passed; its new playlist-resume assertion failed
with a noncanonical host fixture root. Canonicalised fixtures and isolated
generated-playlist cleanup address that regression; rerun evidence is recorded
only after execution. A targeted frontend test also passed locally in Ubuntu WSL.

Latest validated source: 50df90c device build/size reports and 92c17ea Project
quality, both successful. Seven Release + seven ASAN/leak suites passed. The
targeted frontend/media suites passed ASAN/leak detection locally in WSL too.
See software-validation.md for exact run links and limits; firmware source has
not changed after 50df90c. Future documentation-only evidence commits do not
change the built firmware.
Initial 54ba552 tests caught corrupt-path display clipping; repaired in 35b7ab4.
Do not hide a failed run or treat an unexecuted checklist as a pass.

## Actual next human gate

Choose BD-14 Bluetooth architecture and BD-15 display using their comparison
documents. Do not create final schematic/PCB/CAD until both are chosen. Code
for a chosen radio/display cannot be represented as completed before a choice.
The assistant's conditional recommendations are S3 + qualified BM83 AT and
non-touch 1.69-inch TFT; exact firmware/SKU and drawings still need qualification.

After choices: reconcile GPIO/SRAM/PSRAM/power and implement selected adapters,
full-card index/UI and fuel/power HAL. Finish/prioritise remaining software; then
builder authors one subsystem at a time using builder-tasks.md. Reviews identify
specific issues, builder fixes and returns sources; independent human review
and actual manufacturing exports remain hard funding blockers.
