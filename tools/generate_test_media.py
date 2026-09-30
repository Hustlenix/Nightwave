#!/usr/bin/env python3
"""Generate deterministic, original Nightwave WAV bench-test fixtures."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import wave
from pathlib import Path
from typing import Callable


SampleFunction = Callable[[float], tuple[float, ...]]


def ramp(t: float, duration: float, edge_seconds: float = 0.02) -> float:
    return min(1.0, max(0.0, t / edge_seconds), max(0.0, (duration - t) / edge_seconds))


def tone(frequency_hz: float, t: float, amplitude: float = 0.18) -> float:
    return amplitude * math.sin(2.0 * math.pi * frequency_hz * t)


def write_wav(
    path: Path,
    sample_rate: int,
    channels: int,
    duration: float,
    sample_function: SampleFunction,
) -> None:
    frame_count = int(round(sample_rate * duration))
    with wave.open(str(path), "wb") as output:
        output.setnchannels(channels)
        output.setsampwidth(2)
        output.setframerate(sample_rate)
        frames = bytearray()
        for index in range(frame_count):
            t = index / sample_rate
            samples = sample_function(t)
            if len(samples) != channels:
                raise ValueError(f"{path.name}: expected {channels} samples, got {len(samples)}")
            for sample in samples:
                value = int(round(max(-1.0, min(1.0, sample)) * 32767.0))
                frames.extend(struct.pack("<h", value))
        output.writeframes(frames)


def generate(output_dir: Path) -> list[dict[str, object]]:
    output_dir.mkdir(parents=True, exist_ok=True)

    specifications: list[tuple[str, int, int, float, SampleFunction]] = [
        ("01_silence_stereo_44100.wav", 44100, 2, 1.0, lambda _t: (0.0, 0.0)),
        (
            "02_left_right_stereo_44100.wav",
            44100,
            2,
            6.0,
            lambda t: (
                tone(440.0, t) * ramp(t % 2.0, 2.0) if t < 2.0 or t >= 4.0 else 0.0,
                tone(660.0, t) * ramp(t % 2.0, 2.0) if t >= 2.0 else 0.0,
            ),
        ),
        (
            "03_mono_1000hz_22050.wav",
            22050,
            1,
            3.0,
            lambda t: (tone(1000.0, t) * ramp(t, 3.0),),
        ),
        (
            "04_dual_tone_stereo_48000.wav",
            48000,
            2,
            3.0,
            lambda t: (
                tone(440.0, t) * ramp(t, 3.0),
                tone(880.0, t) * ramp(t, 3.0),
            ),
        ),
        (
            "05_log_sweep_stereo_44100.wav",
            44100,
            2,
            10.0,
            lambda t: (
                0.14
                * math.sin(2.0 * math.pi * 20.0 * 10.0 / math.log(1000.0) * (1000.0 ** (t / 10.0) - 1.0))
                * ramp(t, 10.0),
            )
            * 2,
        ),
        (
            "06_tone_gap_edges_44100.wav",
            44100,
            2,
            8.0,
            lambda t: (
                (tone(523.25, t) * ramp(t % 1.0, 1.0)) if int(t) % 2 == 0 else 0.0,
                (tone(523.25, t) * ramp(t % 1.0, 1.0)) if int(t) % 2 == 0 else 0.0,
            ),
        ),
    ]

    manifest: list[dict[str, object]] = []
    for filename, sample_rate, channels, duration, function in specifications:
        path = output_dir / filename
        write_wav(path, sample_rate, channels, duration, function)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        with wave.open(str(path), "rb") as generated:
            assert generated.getnchannels() == channels
            assert generated.getframerate() == sample_rate
            assert generated.getsampwidth() == 2
            assert generated.getnframes() == round(sample_rate * duration)
        manifest.append(
            {
                "file": filename,
                "sha256": digest,
                "sample_rate_hz": sample_rate,
                "channels": channels,
                "bits_per_sample": 16,
                "duration_seconds": duration,
            }
        )

    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = generate(args.output)
    print(f"Generated {len(manifest)} verified WAV fixtures in {args.output}")


if __name__ == "__main__":
    main()
