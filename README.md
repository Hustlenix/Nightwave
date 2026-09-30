# Nightwave

Nightwave is an in-progress, standalone pocket music player for the Pixl “A Music player for the saloon” Trial. It is intended to read a user's MP3 and PCM/WAV files directly from microSD, decode them on an ESP32-S3, and play them through either wired stereo headphones or a built-in speaker—without a phone, app, network, Bluetooth, or streaming service.

> Status: Phase 0/1 and the Phase 2 digital package are complete. Phase 3 now has build-oriented board, button, SD, I2S tone, and buffered WAV firmware plus portable host tests. None of those hardware paths has been physically exercised yet. No physical runtime, working hardware, final PCB, enclosure, or tier is claimed.

## Architecture

```text
microSD / FAT
      |
      v
StorageTask -> compressed ring buffer -> DecoderTask -> PCM ring buffer
                                                        |
                                                        v
                                                AudioOutputTask
                                                        |
                                                   I2S + DMA
                                      +-----------------+-----------------+
                                      |                                   |
                                PCM5102A DAC                       MAX98360C amp
                                      |                                   |
                              TPA6132A2 headphone amp                mono speaker
                                      |
                               3.5 mm stereo jack
```

The ESP32 owns file enumeration, storage reads, MP3/WAV decoding, buffering, playback state, I2S output, buttons, UI, settings, diagnostics, and power policy. Nightwave does not use DFPlayer or another module that hides the storage/decode/playback pipeline.

The bench prototype uses an available MAX98357A breakout as a functional speaker-path proxy; the provisional final-board selection is MAX98360C.

## Current implementation

- repository and required engineering-document structure;
- current Pixl/Trial rules verification and honest AI-disclosure strategy;
- datasheet-backed provisional component comparison, BOM, pin budget, power tree, risk register, prototype plan, and decoder study;
- ESP-IDF 6.1 C++ firmware with safe-muted boot, chip/reset/heap/PSRAM diagnostics, debounced five-button logging, 1-bit SDMMC mount/enumeration/benchmarking, I2S DMA output, and a low-level tone test;
- robust host-tested WAV parsing for 16-bit mono/stereo PCM at 22.05/32/44.1/48 kHz, including unknown-chunk skipping and explicit errors;
- two-task WAV streaming through a 16,384-frame SPSC PCM ring to the I2S output task with visible underrun telemetry;
- portable tests for WAV parsing, generated fixtures, ring wrap/full behavior, stereo-to-mono arithmetic, volume scaling, button debounce, and playback state;
- CI definitions for both ESP32-S3 firmware and portable host tests;
- exact Phase 2 prototype BOM, pin-by-pin wiring table, voltage/current plan, deterministic test-media generator, and staged bring-up checklist.

The firmware is digitally implemented but does **not** prove that a card mounts, a waveform reaches a physical module, a speaker/headphone produces correct audio, or the chosen buffer survives real card latency. OLED, settings/NVS, headphone-codec control, MP3 decoding, and removal recovery are not implemented yet.

## Build the firmware

Target toolchain: ESP-IDF `v6.1`, target `esp32s3`.

```sh
cd firmware
idf.py set-target esp32s3
idf.py build
```

The development baseline is `ESP32-S3-DevKitC-1-N8R8`; the provisional final module is `ESP32-S3-WROOM-1-N16R8`. Pins and component choices remain provisional until prototype validation.

The serial diagnostic console exposes `board`, `sd`, `bench`, `tone`, `wav`, `stop`, and `status`. Follow [Phase 3 firmware bring-up](docs/phase3-firmware-bringup.md) before issuing any command that enables a physical output.

## Engineering documents

- [Pixl requirements](docs/pixl-requirements.md)
- [Architecture notes](docs/architecture-notes.md)
- [Component comparison](docs/component-comparison.md)
- [Decoder study](docs/decoder-study.md)
- [Decision log](docs/decisions.md)
- [Pin budget](docs/pin-budget.md)
- [Power tree](docs/power-tree.md)
- [Prototype plan](docs/prototype-plan.md)
- [Prototype BOM](hardware/prototype-BOM.csv)
- [Prototype wiring](docs/prototype-wiring.md)
- [Prototype bring-up checklist](docs/prototype-bringup.md)
- [Deterministic test media](docs/test-tracks.md)
- [Phase 3 firmware bring-up](docs/phase3-firmware-bringup.md)
- [Audio buffer analysis](docs/audio-buffer-analysis.md)
- [Headphone prototype options](docs/headphone-prototype-options.md)
- [MP3 integration plan](docs/mp3-integration-plan.md)
- [Risk register](docs/risks.md)
- [Preliminary BOM](hardware/BOM.csv)
- [AI disclosure](docs/ai-disclosure.md)

## Physical validation still required

Direct-SD playback, decoder timing, buffer sizing, audio quality, channel correctness, output power, noise, thermal behavior, current draw, charge-while-play behavior, battery capacity, full-night runtime, PCB manufacturability, and enclosure fit all require physical evidence. The project-defined acceptance test is at least eight hours of continuous speaker playback under a declared profile; the current Pixl Trial itself says “a full night” and requires the achieved runtime to be stated, but does not publish an exact hour count.

## AI disclosure

AI assistance is being used for research synthesis, repository scaffolding, interface design, documentation, and software checks. The builder must understand and materially review the electrical and mechanical design, physically assemble and test the device, maintain honest journals and photos, and obtain a second-person sanity check before hardware submission. Fully AI-generated hardware design files are not acceptable under the current Pixl hardware rules.

## License

Nightwave's original repository content is provided under the MIT License. Third-party libraries keep their own licenses and notices.
