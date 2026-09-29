# PRD.md

# Nightwave — Offline Music Player (T4 Target)

## 1. Product definition

**Nightwave** is a standalone, battery-powered physical music player that plays a user's own audio files from removable storage through built-in speakers or wired headphones, with no phone, app, network connection, or streaming dependency.

This project is intentionally designed beyond a module-carrier music player. The technical target is an embedded audio system with direct filesystem access, software audio decoding, DMA-driven I²S playback, custom power and audio electronics, a custom PCB, editable enclosure CAD, and measured runtime/performance evidence.

> T4 Nexus is a target, not a guaranteed rating. Pixl reviewers assign tiers based on ambition, technical depth, and how well the project is put together.

## 2. Problem statement

A basic offline player can be built by commanding a DFPlayer-style module, but that outsources most of the technically meaningful work: filesystem handling, audio decoding, buffering, audio transport, and output control.

Nightwave should instead demonstrate that the builder understands and implements the audio pipeline end-to-end while still producing a usable physical product.

## 3. Trial brief and Nightwave acceptance requirements

The current Pixl Trial says the battery must last “a full night” and the achieved runtime must be stated; it does not publish a numeric duration. Nightwave defines a stricter internal acceptance profile of at least eight continuous hours. This is a project decision, not quoted Trial text.

Nightwave MUST:

- load the user's own audio files from removable microSD storage;
- play audio without a phone, web service, app, Wi-Fi, Bluetooth, cellular connection, or streaming service;
- provide physical controls for play/pause, previous/next track, volume up, and volume down;
- play through a built-in physical speaker;
- play through wired 3.5 mm headphones;
- run from a rechargeable battery;
- achieve and document a measured continuous runtime of at least **8 hours** in a declared "full-night" test profile;
- include source firmware;
- include a readable schematic and custom PCB source;
- include a complete editable enclosure source and exported STEP;
- include a BOM with prices/links/total;
- include build and assembly instructions;
- include a final demo video;
- include real build-progress evidence.

## 4. T4-target technical thesis

The project's core technical identity is:

**microSD → filesystem → compressed audio parser/decoder → buffered PCM → optional DSP → I²S + DMA → stereo DAC/headphone chain + speaker amplifier**

The T4-target scope MUST include:

- direct microSD access by the ESP32-S3;
- direct filesystem management using FatFS or equivalent;
- software decoding for at least MP3 plus uncompressed PCM/WAV;
- a producer/consumer audio pipeline with ring buffers or queues;
- DMA-driven I²S output;
- explicit underrun/overflow handling;
- FreeRTOS task separation for audio/storage/UI/input/power;
- custom mixed-signal PCB;
- custom battery charging/power-path subsystem;
- battery fuel-gauge telemetry;
- custom enclosure with real mounting features;
- controlled performance and battery-runtime validation.

## 5. Target users

Primary user: a person who wants a simple offline music device for a night of playback without relying on a phone or network.

Secondary user: another builder who clones the repository and wants enough information to reproduce the device.

## 6. Jobs to be done

When I have my own music files, I can copy them to a microSD card, insert the card, power on Nightwave, browse/play them, and listen from the speaker or wired headphones.

When I use the player for a long session, I can see battery state and rely on it to continue playing for a full-night test period.

When I clone the repository, I can understand the electronics, firmware, enclosure, BOM, flashing process, and assembly without private knowledge from the original builder.

## 7. Goals and measurable success criteria

| Goal | Acceptance measure |
|---|---|
| Offline playback | Device plays local files with radios/network disabled |
| Own audio files | User can replace microSD contents without reflashing firmware |
| Physical controls | Dedicated play/pause, previous, next, volume+, volume- controls work |
| Speaker playback | Built-in speaker produces intelligible continuous playback |
| Headphone playback | 3.5 mm stereo output produces correct left/right channels |
| Audio pipeline depth | Firmware performs storage read, decode, buffering, and I²S output itself |
| Robustness | SD removal/corrupt file/decoder failure do not crash into unrecoverable state |
| Battery | Measured runtime >= 8 h in documented full-night profile |
| Reproducibility | Repo contains all required source, CAD, BOM, firmware, and instructions |
| Mechanical completeness | PCB, battery, speaker, controls, display are mechanically secured |
| Build evidence | Progress photos/journals and demo video show real physical build |
| T4 evidence | Architecture, profiling, failure handling, custom hardware, and validation are visible in repo |

## 8. Non-goals

Nightwave V1 MUST NOT include:

- Wi-Fi streaming;
- Bluetooth audio;
- Spotify/YouTube/online services;
- companion phone application;
- cloud accounts;
- touch screen;
- internet radio;
- microphone/voice assistant;
- DRM playback;
- wireless headphones;
- an operating system unrelated to playback needs;
- features added only to inflate scope without increasing engineering depth.

FLAC, album art, search, and advanced playlists are post-core features unless the core pipeline is already stable.

## 9. Product controls

The baseline product uses:

- POWER switch or controlled power button;
- PREVIOUS button;
- PLAY/PAUSE button;
- NEXT button;
- VOLUME DOWN button;
- VOLUME UP button.

A rotary encoder MAY be added for navigation/volume but MUST NOT remove the required obvious physical playback controls unless the resulting UX is clearly better and still satisfies the Trial.

## 10. Display and interaction

A compact OLED is the default UI.

Required screens:

### Boot
Show project name, firmware version, and initialization status.

### Now playing
Show:
- track/file title where available;
- play/pause state;
- elapsed time where available;
- volume;
- battery percentage;
- speaker/headphone output state.

### Library / file browser
At minimum:
- folders;
- supported files;
- current selection;
- open/back actions.

### System states
Explicit screens or overlays for:
- no SD card;
- unsupported/corrupt file;
- low battery;
- charging;
- SD read error;
- audio underrun diagnostic if debug mode is enabled.

## 11. Audio requirements

MUST support:

- WAV/PCM playback;
- MP3 playback;
- stereo headphone output;
- mono speaker output derived from stereo source without destructive clipping;
- volume ramping or muting strategy that avoids severe pops/clicks;
- pause/resume;
- previous/next;
- track-end transition;
- automatic speaker mute or disable when headphones are inserted if the selected jack supports detection.

Target baseline output format:
- 44.1 kHz and/or 48 kHz;
- 16-bit stereo PCM internally or a documented equivalent.

The architecture MUST allow varying source sample rates without corrupt playback. The exact resampling strategy is an implementation decision documented in Architecture.md.

## 12. Storage requirements

MUST:

- use removable microSD;
- mount a standard filesystem readable by desktop computers;
- discover playable files at runtime;
- tolerate empty cards and unsupported files;
- avoid requiring a generated proprietary database before playback;
- handle card removal safely enough that firmware remains responsive.

SHOULD:
- index metadata lazily or on boot;
- persist library metadata cache only if measured boot time justifies it.

## 13. Firmware requirements

MUST use ESP-IDF C/C++ unless a later engineering decision demonstrates a materially better stack.

MUST implement:
- storage task;
- decoder task;
- audio/output task;
- UI task;
- input task;
- power-monitor task;
- inter-task queues/events;
- ring buffer(s);
- audio underrun counter;
- persistent settings;
- structured logging;
- explicit error states.

MUST NOT put the entire application in one monolithic loop/source file.

## 14. Electronics requirements

Baseline architecture:

- ESP32-S3 module on final PCB;
- microSD socket;
- external stereo I²S DAC for headphones/line path;
- stereo headphone amplifier;
- digital Class-D amplifier for built-in speaker;
- 1-cell Li-ion/LiPo battery;
- charger with power-path capability;
- battery fuel gauge;
- 3.3 V regulated digital/analog rail;
- USB-C power/charging input;
- ESD/protection where required;
- test points for major rails and buses.

Current candidate ICs, subject to BOM/availability validation before schematic freeze:

- PCM5102A stereo DAC;
- TPA6132A2 or validated equivalent headphone amplifier;
- MAX98357A or validated equivalent I²S Class-D speaker amplifier;
- BQ24074/BQ25185-class 1-cell charger with power path;
- MAX17048-class fuel gauge.

No candidate part is considered locked until:
- datasheet reviewed;
- package/footprint verified;
- supply-chain availability checked;
- fabrication/assembly capability checked.

## 15. Battery and power requirements

Definition of "full night" for acceptance:

**>= 8 hours continuous playback from a fully charged battery using the built-in speaker at a declared fixed volume setting, with the display using the normal screen-timeout policy.**

Engineering target:
- >= 10 hours speaker profile;
- >= 12 hours headphone profile.

These are design targets only. The README MUST publish measured results, not estimated results, after testing.

Runtime test MUST record:
- battery model/capacity;
- firmware version;
- playback format/playlist;
- output mode;
- volume setting;
- display behavior;
- start time;
- periodic battery/SOC readings;
- shutdown time;
- measured runtime.

## 16. Mechanical requirements

The final enclosure MUST:

- include a complete editable CAD source;
- include exported STEP;
- contain the PCB, battery, speaker, buttons, display, jack, and USB connector;
- mechanically secure the PCB;
- mechanically secure the battery without puncture/compression hazards;
- mechanically secure the speaker;
- provide button actuation;
- expose necessary ports;
- include tolerances for printing/assembly;
- avoid depending on tape/glue as the primary structural system;
- be openable for repair where practical.

Target external size:
- pocketable rather than ultra-miniature;
- exact dimensions driven by battery/speaker/PCB after engineering;
- no artificial size constraint that compromises runtime or safety.

## 17. Hardware safety

Lithium battery work is safety-critical.

The project MUST:
- use a protected cell or an explicitly justified protection design;
- use a proper single-cell charger;
- verify polarity before connection;
- provide current-limited initial power-up where possible;
- inspect for rail shorts before inserting battery;
- never bypass cell protection;
- never intentionally short, puncture, heat, crush, overcharge, or deeply discharge a lithium cell.

Physical battery connection is a HUMAN GATE.

## 18. Failure and edge cases

Firmware MUST address:

- SD absent at boot;
- SD removed during playback;
- unreadable filesystem;
- unsupported extension;
- truncated/corrupt MP3;
- buffer underrun;
- decoder error;
- headphone insertion/removal during playback;
- low battery;
- charging while playing;
- settings corruption;
- unexpected reset.

Where graceful recovery is impractical, the UI MUST explain the fault and offer a restart/retry path.

## 19. Evidence and validation requirements

The repository MUST contain evidence for:

- audio pipeline architecture;
- task architecture;
- buffer sizing decision;
- SD read throughput;
- CPU/memory use during playback;
- underrun count during a long playback test;
- current draw in at least idle, headphone playback, speaker playback, and charging states;
- measured battery-runtime curve;
- headphone left/right channel test;
- speaker test;
- temperature/thermal observations for power/audio ICs;
- enclosure fit validation.

Measurements MUST be clearly distinguished from estimates.

## 20. Pixl / AI requirements

This project is an AI-assisted build, not a fully AI-authored hardware submission.

ChatGPT Work MAY:
- research;
- explain;
- propose;
- calculate;
- scaffold firmware;
- inspect source;
- write tests;
- review schematic/PCB/CAD;
- help diagnose physical measurements;
- organize documentation.

The builder MUST:
- understand and make/approve the engineering decisions;
- personally author or materially edit the submitted design;
- physically assemble and test the device;
- maintain genuine progress journals;
- disclose AI usage accurately;
- request a human sanity check before design submission.

Do not submit fabricated test results, progress, or claims.

## 21. MVP boundary

The minimum shippable T4-target system is complete when all of these work together:

1. ESP32-S3 reads files directly from microSD.
2. Firmware decodes MP3 and WAV.
3. PCM is buffered and delivered through I²S DMA.
4. Wired stereo headphones work.
5. Built-in speaker works.
6. Five physical playback/volume buttons work.
7. OLED UI works.
8. Battery charging and fuel gauge work.
9. >=8 h full-night measured speaker runtime is achieved.
10. Custom PCB is fabricated and assembled.
11. Complete enclosure is printed/assembled.
12. Required repository files and build documentation exist.
13. Final demo video proves the physical system.

## 22. Post-MVP ideas

Only after the MVP is stable:

- FLAC;
- playlists;
- shuffle/repeat;
- sleep timer;
- five-band EQ;
- loudness/limiter;
- album-art preprocessing;
- faster indexed library;
- USB mass-storage mode;
- USB audio;
- theme system;
- desktop playlist/index generator.

## 23. Acceptance criteria / launch definition

Nightwave is "shipped" only when:

- the final physical device plays user-supplied files;
- speaker and headphones are demonstrated;
- all required controls are demonstrated;
- the final custom PCB is used;
- enclosure is assembled;
- full-night runtime has been physically measured;
- source repository is public and reproducible;
- BOM, KiCad, firmware, editable CAD, STEP, images, README, and demo are present;
- physical build progress is documented;
- any hardware bodges are reflected back into source files;
- AI assistance is accurately disclosed;
- a second human has sanity-checked the design.

---

## Primary external references for implementation

- Pixl Hardware Ship Requirements: https://pixl.hackclub.com/docs/hardware-requirements
- Pixl Restoration Energy / tiers: https://pixl.hackclub.com/docs/energy
- Pixl first-project workflow: https://pixl.hackclub.com/docs/first-project
- ESP32-S3 I²S / SDMMC docs: https://docs.espressif.com/
- PCM5102A: https://www.ti.com/product/PCM5102A
- MAX98357A: https://www.analog.com/en/products/max98357a.html
- BQ24074: https://www.ti.com/product/BQ24074
- MAX17048: https://www.analog.com/en/products/max17048.html
