# MP3 integration plan

Status: decoder selected and adapter contract prepared; integration is deliberately blocked until real WAV playback is stable.

## Selected decoder

`chmorgan/esp-libhelix-mp3` 1.0.3 remains the provisional selection. It provides an ESP-IDF-oriented Apache-2.0 wrapper around the integer Helix decoder. Nightwave will own the file, buffers, bounded resynchronization, task scheduling, volume/mixing, I2S, and errors.

The dependency is not yet added to the firmware manifest. Adding it now would imply that the MP3 path is ready before the required hardware WAV gate has passed.

## Adapter boundary

The future adapter will implement `AudioDecoder` and consume a byte-ring view. It must:

1. parse and skip ID3v2 using a bounded synchsafe length;
2. retain incomplete frames across ring wrap;
3. call `MP3FindSyncWord` and `MP3Decode` with a finite resync-byte budget;
4. validate reported sample rate, channel count, and sample count;
5. emit stereo PCM frames, duplicating mono without destructive gain;
6. report format changes so output can mute, drain, reconfigure, prefill, and ramp;
7. expose bytes consumed, frames decoded, failures, resync bytes, and decode timing;
8. terminate cleanly on EOF, truncated data, unsupported free-format streams, or stop.

## Gate to begin integration

- one supported WAV fixture plays through the selected physical output;
- left/right identity is correct where applicable;
- SD benchmark output is captured;
- WAV playback reports no unexplained reset and its underrun behavior is understood;
- the exact ESP32-S3 board/module and audio breakouts are confirmed by label/photo.

Only after that gate should the component registry dependency be pinned, its license notices committed, and CBR/VBR/corrupt fixtures exercised.

