# Nightwave

Nightwave is an in-progress, standalone pocket music player for the Pixl “A Music player for the saloon” Trial. It is intended to read a user's MP3 and PCM/WAV files directly from microSD, decode them on an ESP32-S3, and play them through either wired stereo headphones or a built-in speaker—without a phone, app, network, Bluetooth, or streaming service.

> Status: Phase 0 and Phase 1 digital engineering are complete. The architecture and component choices are provisional until the physical audio prototype is built and measured. No physical runtime, working hardware, final PCB, enclosure, or tier has been claimed.

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
                                PCM5102A DAC                       MAX98357A amp
                                      |                                   |
                              TPA6132A2 headphone amp                mono speaker
                                      |
                               3.5 mm stereo jack
```

The ESP32 owns file enumeration, storage reads, MP3/WAV decoding, buffering, playback state, I2S output, buttons, UI, settings, diagnostics, and power policy. Nightwave does not use DFPlayer or another module that hides the storage/decode/playback pipeline.

## Current implementation

- repository and required engineering-document structure;
- current Pixl/Trial rules verification and honest AI-disclosure strategy;
- datasheet-backed provisional component comparison, BOM, pin budget, power tree, risk register, prototype plan, and decoder study;
- ESP-IDF 6.1 C++ firmware skeleton with typed interfaces for state, events, playback commands, audio data, storage, decoding, I2S, input, UI, power, settings, and diagnostics;
- CI definition for an ESP32-S3 firmware build.

The skeleton does **not** yet mount an SD card, decode audio, drive I2S, render a display, or prove any physical subsystem. Those behaviors begin with the bench prototype in Phase 2 onward.

## Build the firmware skeleton

Target toolchain: ESP-IDF `v6.1`, target `esp32s3`.

```sh
cd firmware
idf.py set-target esp32s3
idf.py build
```

The development baseline is `ESP32-S3-DevKitC-1-N8R8`; the provisional final module is `ESP32-S3-WROOM-1-N16R8`. Pins and component choices remain provisional until prototype validation.

## Engineering documents

- [Pixl requirements](docs/pixl-requirements.md)
- [Architecture notes](docs/architecture-notes.md)
- [Component comparison](docs/component-comparison.md)
- [Decoder study](docs/decoder-study.md)
- [Decision log](docs/decisions.md)
- [Pin budget](docs/pin-budget.md)
- [Power tree](docs/power-tree.md)
- [Prototype plan](docs/prototype-plan.md)
- [Risk register](docs/risks.md)
- [Preliminary BOM](hardware/BOM.csv)
- [AI disclosure](docs/ai-disclosure.md)

## Physical validation still required

Direct-SD playback, decoder timing, buffer sizing, audio quality, channel correctness, output power, noise, thermal behavior, current draw, charge-while-play behavior, battery capacity, full-night runtime, PCB manufacturability, and enclosure fit all require physical evidence. The project-defined acceptance test is at least eight hours of continuous speaker playback under a declared profile; the current Pixl Trial itself says “a full night” and requires the achieved runtime to be stated, but does not publish an exact hour count.

## AI disclosure

AI assistance is being used for research synthesis, repository scaffolding, interface design, documentation, and software checks. The builder must understand and materially review the electrical and mechanical design, physically assemble and test the device, maintain honest journals and photos, and obtain a second-person sanity check before hardware submission. Fully AI-generated hardware design files are not acceptable under the current Pixl hardware rules.

## License

Nightwave's original repository content is provided under the MIT License. Third-party libraries keep their own licenses and notices.
