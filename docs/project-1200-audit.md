# Project 1200 audit — 2026-10-02

2026-10-03 repository-only update: BD-15 now selects Waveshare 24382 non-touch
and BD-16 selects TCA9535PWR input expansion. This does not refresh the dated
authenticated project-page observations below. The current BOM includes both,
but charger/pack/BT rail/support costs and hardware sources remain incomplete.

This is AI-assisted documentation of observed state, not a builder journal,
human time record, design approval or evidence of physical construction.

## Current evidence

- Authenticated project: https://pixl.hackclub.com/project/1200.
- Repository: https://github.com/Hustlenix/Nightwave.
- Trial: A Music player for the saloon, Dustline.
- Kind/type: hardware. Funding requested; finished-built-hardware unchecked.
- Before cleanup, three journals claimed 13.0, 6.4 and 4.0 hours (23.4 total).
- The builder-supplied discussion states the hours were based on journal text
  length. That does not establish work duration. No reliable replacement
  durations were provided in the initial audit.
- Cleanup set each original entry to 0h while preserving its prose and images.
  A page reload confirmed 0.0h TRACKED and the linked repository. This removes
  unsupported claims; it does not assert that no genuine personal work occurred.
- AI assistance includes firmware, tests, documentation, research and website
  work. Repository output and agent elapsed time do not establish human hours.
- ESP32-S3 plus BM83SM1-00TA is selected; display BD-15 remains pending.
- Final schematic, PCB, editable enclosure and physical results are missing.

The project-page repository/status/AI notes require accurate metadata. Existing
journal prose must be corrected by the builder: the inspiration entry still
describes ESP32-C3/DFPlayer, whereas current Nightwave uses ESP32-S3 and software
decoding. A website screenshot is not evidence of working hardware.

## What is required for funding

The current [hardware requirements](https://pixl.hackclub.com/docs/hardware-requirements)
allow design submission before physical construction. They still require a
complete, original builder-authored design, readable README with design images,
full CAD assembly and attachment methods, PCB/source exports where applicable,
firmware, linked complete BOM with total, and another person's sanity check.

The authenticated form additionally requires a repository, demo link,
thumbnail, uploaded BOM CSV, cart screenshots including shipping, and a funding
amount. Its help text says a working link can serve at the funding stage; a
video is required when finished hardware is selected. A passing form checklist
does not waive the design requirements or establish Trial completion.

The current BOM is preliminary and lacks selected display/battery and support
costs. Do not upload it as a complete funding BOM or substitute fabricated cart
screenshots, runtime numbers, build photos or CAD sources.

## Remaining work in order

1. BD-14/15/16 are now selected. Review the combined numbered map, current power/
   charger/pack/BT rail and remaining schematic-critical support selections.
2. Propagate the selection into GPIO, firmware adapter, BOM, mechanical and power
   budgets. Keep estimates distinct from measurements; qualify the BM83 route.
3. Builder authors schematic, PCB and enclosure, using `builder-tasks.md` and
   `schematic-requirements.md`. Review actual sources and ERC/DRC outputs.
4. Complete component/support/manufacturing/shipping costs and assembly exports.
5. Obtain and address an independent human design review. Capture real design
   images and supplier carts; funding submission then becomes reviewable.
6. Build and test physically. Record real progress photos, audio behavior,
   battery/runtime results and a working video for final build/Trial approval.

## Journal repair and future sessions

The builder's supplied Pixl-help discussion explicitly says not to use AI to
write journals. Preserve original builder prose rather than humanizing AI text.
Correct misleading personal claims yourself. Claim only real, eligible personal
time, and do not duplicate Hackatime/Lapse time. Character count is a form limit,
not a way to estimate time spent.

For a new entry, first perform a real session, then write your own account of
the action, result, difficulty and next step. Record start/end or actual active
minutes and attach evidence produced by that session. Do not invent retrospective
sessions merely to increase the number of entries.

Useful future sessions, not completed work or ready-to-post journal text:

- Trace the current audio chain and explain each stage in your own words; attach
  your own annotated diagram and unresolved questions.
- Compare the two display candidates, choose one, and record your own reasons
  and dimensions/interface implications.
- After architecture/power prerequisites are resolved, author the first KiCad
  block, run ERC, and attach your actual source/screenshot and remaining issues.

Software evidence remains revision-specific in `software-validation.md`. This
cleanup does not count as a physical test, human design session or claimed hours.
