# Firmware test plan

Portable C++ tests live in `/tests` and deterministic audio fixtures are generated into `/.generated/test-media` by `tools/generate_test_media.py`. Generated media is intentionally ignored by Git.

The host suite covers WAV chunk/error handling, a generated 44.1 kHz stereo fixture, ring-buffer wrap and full states, overflow-safe stereo-to-mono mixing, Q15 volume, debounce behavior, and playback state. Hardware-in-the-loop checks remain required for SD hot-plug, I2S clocking, output switching, audio correctness, brownout behavior, and long-duration playback.

No functional physical playback test is claimed at this milestone.
