# Deterministic prototype test media

`tools/generate_test_media.py` creates original, non-musical WAV fixtures without third-party Python packages. Generated files are intentionally not committed; the manifest includes hashes so a bench run can record exactly what was used.

Run:

```powershell
rtk python tools/generate_test_media.py --output .generated/test-media
```

## Generated WAV set

| File | Purpose |
|---|---|
| `01_silence_stereo_44100.wav` | noise-floor, mute, and pop/click observation |
| `02_left_right_stereo_44100.wav` | left-only, right-only, then dual-channel identification |
| `03_mono_1000hz_22050.wav` | mono path and 22.05 kHz source-rate handling |
| `04_dual_tone_stereo_48000.wav` | 48 kHz reconfiguration and stereo integrity |
| `05_log_sweep_stereo_44100.wav` | broad audible-band path check; not a calibrated response measurement |
| `06_tone_gap_edges_44100.wav` | repeated zero-to-tone transitions and underrun-like silence gaps |

Every tone has a short amplitude ramp to reduce artificial edge clicks. Keep the first listening pass at very low volume.

## MP3 derivatives

After inspecting the WAV files, encode two derivatives with a trusted local FFmpeg installation:

```powershell
rtk ffmpeg -y -i .generated/test-media/02_left_right_stereo_44100.wav -codec:a libmp3lame -b:a 128k .generated/test-media/07_left_right_cbr128.mp3
rtk ffmpeg -y -i .generated/test-media/04_dual_tone_stereo_48000.wav -codec:a libmp3lame -q:a 2 .generated/test-media/08_dual_tone_vbr.mp3
```

Record `ffmpeg -version`, encoder arguments, and SHA-256 values in the measurement notes. MP3 files are decoder fixtures, not listening-quality references.

## Fault fixtures

Create corrupt/truncated inputs only as copies of generated media, never from personal music. The future firmware test suite should cover:

- truncated RIFF header;
- declared data length larger than file;
- unsupported 24-bit PCM;
- renamed non-audio file with `.mp3` extension;
- MP3 truncated mid-frame;
- empty file;
- deeply nested and long filenames.

These fixtures validate explicit errors and recovery. They must never be used to infer audio quality.
