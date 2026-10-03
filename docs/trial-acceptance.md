# Nightwave Trial acceptance and full systems scope

Updated 2026-10-03. Target: a working, standalone physical player with substantial
systems depth. T4 is requested, not awarded. The device is not built or accepted.

| Trial requirement | Existing source | Proof still required |
| --- | --- | --- |
| Load own audio from SD/flash | Direct SDMMC/FatFS; software MP3/WAV decoder; original generated fixtures; README loading steps | Actual user-controlled files copied to a real card and played; exact board/card/firmware log |
| Play through real speakers without a phone/app/stream | S3 storage/decode/PCM/I2S tasks and local Class-D speaker route | Physical audible speaker test with phone/network absent; final reviewed amplifier/rail/wiring |
| Physical play/pause, skip and volume | Five-button monitor/debounce, frontend and streaming controls | Button-driven demonstration without serial commands; controls tested during menus and playback |
| Full-night battery and state runtime | Eight-hour internal acceptance profile; bounded runtime recorder; power contracts and estimates | Reviewed real power hardware; actual continuous test with conditions, raw logs and precise achieved runtime |
| Wiring/schematic, editable enclosure and README parts/build steps | Provisional bench wiring, preliminary BOM and linked bring-up/build docs | Builder-authored final schematic/PCB; complete mounted CAD and editable source/STEP; reconciled BOM and final build steps |
| Demo video | Working interactive concept website | Film the actual finished device, local file, speaker, controls and achieved runtime; link in README and Pixl |

The full product retains these connected systems:

1. Local SD filesystem, bounded card catalog, songs/folders/artists/albums/M3U.
2. Software MP3/WAV decoding, sparse checked seeking and accepted-media clock.
3. Storage/decode/audio task separation, encoded/PCM rings, I2S DMA and profiling.
4. Ramped volume and mutually exclusive speaker/stereo-headphone routing.
5. Builder-selected BM83 A2DP source; control/framing contracts are host-tested.
   Combined playback routing, rate conversion and actual UART radio adapter remain
   unimplemented; provisioning/PCM transport are unqualified. S3 keeps software
   decoding. Local-route source is independent; physical audio is still unproven.
6. Synchronized LRC, metadata, repeat/shuffle, resume/settings and sleep timer.
7. Physical controls and readable TFT/display interface; BD-15 driver/pins pending.
8. Reviewed protected rechargeable USB-C power path, regulator, gauge, low-battery
   handling and shutdown; final circuit and hardware adapters still pending.
9. Custom mixed-signal PCB and serviceable enclosure, with real mounts, RF/audio/
   power clearances, port alignment and manufacturing sources.
10. Runtime/fault/memory/latency evidence and a reproducible repository/recovery path.

Uploaded per-song animation is a separate proposal. It is not implemented,
included in a runtime budget or represented as working hardware. Extra features
must fit measured audio deadlines, memory, power and interface budgets before
joining the release. Complexity is established by integrated behavior and evidence.

Funding design approval and finished-build Trial approval are separate. Neither
green checklist items nor passing synthetic CI replace the missing physical work.
Do not submit the incomplete funded draft as a finished device.

Official design/build requirements:
https://pixl.hackclub.com/docs/hardware-requirements

## Demo evidence to capture after actual assembly

Show the finished device and own SD files; boot and start from physical controls;
audible speaker playback with no phone; pause/resume, previous/next and volume;
wired stereo and reviewed Bluetooth routes; readable lyrics/library; real PCB/CAD
assembly; achieved runtime with declared conditions. Use real footage throughout.
Additional shots explain the full scope but cannot conceal a failing core function.

## Finished-device evidence inventory

Run `python3 tools/check_trial_readiness.py` (optionally
`--output .generated/trial-inventory.json`). This is separate from the
[funding-design inventory](shipping-audit.md). The pending
[`measurements/trial-evidence.json`](../measurements/trial-evidence.json) contains
no physical claims. Populate it only from the builder's actual observations,
review records and files, never from generated fixtures or this checklist.

Exit 2 reports missing/invalid evidence. Exit 0 means
`EVIDENCE_PRESENT_HUMAN_REVIEW_REQUIRED`, not a passed physical Trial.
Both outcomes retain `physical_pass: false` and physical acceptance
`NOT_ESTABLISHED`. The inventory checks bounded typed records and artifact
presence, not whether someone heard a speaker, really used buttons, measured a
battery or filmed genuine footage. A human must inspect the raw evidence and the
actual device. Neither dummy CAD/video files nor a truthful-looking JSON record
establish those facts.

Nightwave's internal acceptance remains at least eight continuous hours on the
built-in speaker, fixed declared volume and normal declared display policy,
without USB/another source supplying power. Eight hours is not quoted Pixl wording.
Keep start/end times, exact firmware/board/card/pack, volume, display policy,
ambient conditions, actual cutoff and raw logs. Use the safety-reviewed
[runtime procedure](runtime-test.md); no battery connection is authorized by
adding an evidence record. Report the achieved result exactly, including failures.

### Completion order

1. BD-14/15/16 choices are recorded. Review the combined GPIO map, BM83 provisioning
   and reviewed power/charger/pack/BOM requirements; do not treat screening as approval.
2. Finish route/rate/display/physical-power software integrations and regression tests.
3. Builder authors the schematic, PCB and enclosure; review sources, footprints,
   clearances, manufacturing exports and independent sanity feedback.
4. Assemble and prove standalone SD speaker playback and physical controls with
   reviewed USB-powered bench wiring first.
5. Only after power review, test rechargeable operation and actual overnight
   speaker runtime; retain logs and real build/demo footage.
6. Reconcile final sources/BOM/build steps and README links against the actual
   assembled device. Human review and Pixl's decision remain separate.

No stage is skipped because a synthetic test or artifact-presence check is green.
Schematic builder Task 1 stays gated until the pre-schematic decisions and budgets
are resolved; this is not an instruction to start CAD/PCB now.

### Builder question for #pixl-help

Draft only; not sent and not an approval record:

> I'm building Nightwave, an offline ESP32-S3 music player with my own SD MP3/WAV
> files, a built-in speaker, wired/Bluetooth outputs, physical controls, lyrics,
> a rechargeable battery and builder-authored PCB/enclosure. Before final hardware
> design, can you confirm the current Trial evidence expected for a full-night
> battery run and demo, whether editable CAD plus STEP is sufficient, and any
> additional requirements for the expanded scope? I will disclose AI firmware/
> research/review assistance and do the actual hardware engineering myself.

Keep the real reply/permalink and its date when available. Do not fabricate a
moderator message, authorship history, tracked hours or tier acceptance.
