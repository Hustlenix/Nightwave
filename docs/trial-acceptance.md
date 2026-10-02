# Nightwave Trial acceptance and full systems scope

Updated 2026-10-02. Target: a working, standalone physical player with substantial
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
5. Builder-selected BM83 A2DP source, with AT provisioning and PCM transport still
   unqualified; S3 keeps software decoding. Local outputs work independently.
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
