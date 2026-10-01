# MP3 integration plan

Status: pinned decoder integrated and three-task product path implemented.
The 2026-10-01 funding
request authorizes digital integration before physical WAV tests. Bench evidence
remains pending and must not be inferred from CI or software decoding tests.

## Selected decoder

`chmorgan/esp-libhelix-mp3` 1.0.3 is pinned in the firmware component manifest.
Its wrapper is Apache-2.0 but the bundled Helix decoder retains RPSL/RCSL terms;
see decoder-licenses.md. Nightwave owns files, bounded framing/resync, buffers,
task scheduling, output gain, I2S and errors.

ESP-IDF and host decoding tests have passed; software-validation.md records
the actual runs. This does not establish physical playback readiness.

## Adapter boundary

The adapter implements `AudioDecoder` and consumes retained contiguous staging
bytes from the ring. The implemented boundary is:

1. parse and skip ID3v2 using a bounded synchsafe length;
2. retain incomplete frames across ring wrap;
3. validate Layer III headers and call `MP3Decode` with a finite recovery budget;
4. validate reported sample rate, channel count, and sample count;
5. emit stereo PCM frames, duplicating mono without destructive gain;
6. reject unsupported/mid-track rate changes; reconfigure between tracks after teardown;
7. expose consumption/frame results, errors, bounded recovery and worst decode timing;
8. terminate cleanly on EOF, truncated data, unsupported free-format streams, or stop.

## Physical verification still required

- one supported WAV fixture plays through the selected physical output;
- left/right identity is correct where applicable;
- SD benchmark output is captured;
- WAV playback reports no unexplained reset and its underrun behavior is understood;
- the exact ESP32-S3 board/module and audio breakouts are confirmed by label/photo.

Dependency pinning, license review and CBR/VBR/corrupt host fixtures may proceed
now; the listed physical checks are required before claiming hardware success.

