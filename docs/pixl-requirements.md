# Pixl Requirements Verification

Checked: 2026-09-29 (Asia/Calcutta)

This record separates published Pixl requirements from Nightwave's own stricter engineering goals. The live project page for Pixl project 1200 was checked while signed in, and the public official documentation was checked independently.

## Music Player Trial

| Requirement | Status | Source | Checked | Mandatory or recommended |
|---|---|---|---|---|
| Build a physical music player | Verified | Live Pixl project 1200, full Trial brief | 2026-09-29 | Mandatory |
| Load the builder's own audio files from SD card or flash | Verified | Live Pixl project 1200 | 2026-09-29 | Mandatory |
| Play through real speakers without phone, app, or streaming | Verified | Live Pixl project 1200 | 2026-09-29 | Mandatory |
| Provide physical play, pause, skip, and volume controls | Verified | Live Pixl project 1200 | 2026-09-29 | Mandatory |
| Battery lasts “a full night” and achieved runtime is stated | Verified | Live Pixl project 1200 | 2026-09-29 | Mandatory; no numeric duration is published |
| Repo includes wiring diagram or schematic, modifiable enclosure CAD (`.STEP` or `.blend`), parts list, build steps, and demo video | Verified | Live Pixl project 1200 | 2026-09-29 | Mandatory |
| ESP32/Pi Pico/Arduino plus DAC/amp is a suggested starting point | Verified | Live Pixl project 1200 | 2026-09-29 | Recommended |
| Technical requirements are intentionally not very fixed; ask `#pixl-help` before starting | Verified | Live Pixl project 1200 | 2026-09-29 | Recommended clarification path |

The live project is already associated with “A Music player for the saloon · Dustline.” The project page says the Trial picker flags it when shipping; that is the current Trial flagging mechanism.

## Hardware Requirements

Pixl requires an original, reproducible, plausibly working design. The hardware submission page requires a good README, a complete CAD assembly with all components, a concrete attachment method rather than tape/glue, applicable firmware, and another person’s sanity check. Source: [Pixl Hardware Ship Requirements](https://pixl.hackclub.com/docs/hardware-requirements).

## Required Repository Files

- public, live GitHub repository;
- README explaining what it is, how it is used, and why it exists;
- images of complete CAD and the PCB or a clear wiring diagram;
- BOM in CSV form with links and a total-cost line;
- PCB source when a PCB is used, including project/schematic/layout and manufacturing data;
- editable CAD source plus STEP export (the Trial text also names `.blend` as acceptable modifiable CAD);
- firmware source;
- build-progress images, finished-build images, and a demo video for the build submission;
- source updated to match any physical bodges.

Sources: [Hardware Ship Requirements](https://pixl.hackclub.com/docs/hardware-requirements), [Ship Requirements](https://pixl.hackclub.com/docs/rules).

## CAD / PCB Requirements

The official hardware page requires a complete assembly showing all components and their real retention, editable CAD plus STEP when 3D models are used, and full PCB source/manufacturing files when a PCB is used. A PCB is not explicitly required by the Trial, but it is a Nightwave project requirement and a central part of its technical case.

## AI / Authorship Requirements

The public ship rules permit AI tools when their use is disclosed honestly. The hardware rules are stricter about authorship: the design must be original and custom, not made entirely by AI, and fully AI-generated design files may be rejected. Nightwave therefore treats AI output as research/scaffolding/review material. The human builder must understand, materially review or author, assemble, measure, journal, photograph, and obtain the required second-person review. Sources: [Ship Requirements](https://pixl.hackclub.com/docs/rules), [Hardware Ship Requirements](https://pixl.hackclub.com/docs/hardware-requirements).

## Hackatime / Tracking

- Install Hackatime before substantive work and register the project using the correct Hackatime project name.
- Only hours after 2026-07-18 count under the current rules.
- Trial hour minimums, if any, are shown in the project dashboard.
- Journals are used to justify hours; entries claiming tracked hours require at least 100 characters per hour, with an absolute floor of 100 characters.
- Autonomous AI work must not be represented as human work time.

Sources: [Build Your First Project](https://pixl.hackclub.com/docs/first-project), [Ship Requirements](https://pixl.hackclub.com/docs/rules).

## Shipping / Demo Requirements

The live project page currently asks for a linked GitHub repo, demo link, thumbnail, uploaded BOM CSV, cart screenshots, and funding amount before shipping. It labels at least one tracked hour as optional for hardware. The Trial picker is already set to the music-player Trial. Public rules also require an accessible demo and AI disclosure. No submission was made in this phase.

## Tier Guidance

Reviewers assign T1–T4 after reviewing the build. Current guidance describes T4 Nexus as serious systems work with involved architecture and real depth; padding hours or features does not raise a tier. Nightwave targets that depth but cannot claim a tier. Source: [Restoration Energy and Levels](https://pixl.hackclub.com/docs/energy).

## Hardware Funding

The live Trial page lists “Hardware Funding (at normal rate) and unlocks the music grant in shop,” plus normal-rate pixels beyond the Trial basis; after approval, the builder may choose the Trial prize or all hours as pixels. Funding is not pre-approved purchasing authority. Nightwave will not buy or order anything without a human gate. Source: live project 1200 and [What Do You Actually Get](https://pixl.hackclub.com/docs/get).

## Relevant Dates and Deadlines

- Countable Hackatime cutoff: work after 2026-07-18.
- The live project page displayed Operation Blackout ending 2026-09-30 00:00 local time, but Nightwave was not entered and that event is not a general Trial deadline.
- No music-player Trial closure date was published on the checked project page or public documentation.

## Conflicts With Existing Nightwave Docs

1. The official Trial does **not** define “full night” as eight hours. Nightwave keeps `>= 8 h` as an internal acceptance threshold, not as quoted Pixl text.
2. The Trial accepts SD card or flash; Nightwave deliberately requires removable microSD.
3. The Trial does not require a custom PCB, fuel gauge, FreeRTOS pipeline, MP3 software decoder, or ESP32-S3; these are Nightwave engineering decisions.
4. The official rules explicitly reject fully AI-generated hardware design files. The existing documents already require human authorship/review and second-person sanity checking; the AI-disclosure language has been made explicit.

No contradiction invalidates the current architecture. The PRD wording is amended so the eight-hour value is clearly Nightwave's own test definition.
