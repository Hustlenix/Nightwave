# Phase 2 bring-up checklist

This checklist stops at every point where power, soldering, listening, or measurement requires the human builder. There is no battery in this phase.

## Equipment

- current-limited 5 V bench source or known 2 A USB source;
- digital multimeter with continuity and DC-current capability;
- computer with data-capable USB cable and serial monitor;
- soldering equipment for breakout headers;
- 8–32 GB microSDHC card;
- powered speakers or high-impedance input for the DAC-only stage;
- 32 Ω or higher wired headphones for the EVM stage;
- 8 Ω, 2 W speaker;
- camera for clear module-label and wiring photographs.

## 0 — Unpowered inspection

- [ ] Photograph both sides of every module and record exact labels/revisions.
- [ ] Confirm the board says `ESP32-S3-DevKitC-1-N8R8` and record v1.0/v1.1.
- [ ] Confirm every ground rail is continuous.
- [ ] Confirm 3.3 V and 5 V rails are not shorted to ground or each other.
- [ ] Confirm SD pull-ups measure approximately 10 kΩ to 3.3 V.
- [ ] Confirm no speaker terminal is connected to ground.
- [ ] Check that all GPIO matches `hardware/prototype-wiring.csv`.

Stop on any short, unknown label, damaged cell, or disagreement with the wiring table.

## 1 — DevKitC only

- [ ] Disconnect every peripheral.
- [ ] Power through the USB-to-UART connector.
- [ ] Capture boot log, chip revision, flash size, PSRAM size, reset reason, and firmware commit.
- [ ] Confirm GPIO38 RGB LED behavior for the recorded board revision.

Pass: repeated boots produce stable logs with no brownout/reset loop.

## 2 — Inputs and OLED

- [ ] Power off, connect OLED and buttons, then power on.
- [ ] Run an I2C scan and record the OLED address.
- [ ] Exercise every button at least ten times.
- [ ] Record bounce duration and choose debounce timing from observations.
- [ ] Display solid/off/checkerboard screens and note current variation.

Pass: every button maps correctly and the OLED updates without resets.

## 3 — microSD

- [ ] Power off and connect the raw 3.3 V socket and all five pull-ups.
- [ ] Insert a freshly FAT32-formatted test card before power-on.
- [ ] Mount, enumerate, and stream-read a large file.
- [ ] Record card make/model, capacity, mount time, sequential throughput, and read errors.
- [ ] Power down before removing the card until removal recovery firmware exists.

Pass: three cold boots and a five-minute sequential read complete without error.

## 4 — PCM5102 line output

- [ ] Connect BCLK/WSEL/DIN and DAC power with `MUTE` asserted.
- [ ] Connect only powered speakers, an audio analyzer, or another input rated for line level.
- [ ] Start zero PCM, configure 44.1 kHz/16-bit stereo I2S, then unmute.
- [ ] Play silence, left/right identification, dual-tone, and sweep files.
- [ ] Stop clocks only after ramping digital samples to zero and muting.

Pass: channel identity is correct, silence has no obvious severe noise, and repeated start/stop does not create severe pops.

## 5 — TPA6132A2 headphones

- [ ] Verify EVM VDD polarity, JP1/JP2, and 0 dB gain before power.
- [ ] Keep DAC muted and volume zero while inserting headphones.
- [ ] Unmute and increase volume one step at a time.
- [ ] Verify left/right identity at a safe listening level.
- [ ] Repeat mute/unmute and track-rate changes while noting clicks or imbalance.

Pass: stereo channels are correct, volume control is predictable, and no dangerous transient or thermal fault occurs.

## 6 — MAX98357A speaker

- [ ] Use a separate current-limited 5 V source for the amplifier with common ground.
- [ ] Verify speaker impedance and that neither BTL output is grounded.
- [ ] Hold `SD` low; start I2S with zero samples; then enable.
- [ ] Raise volume gradually while recording supply current.
- [ ] Run ten minutes at the intended prototype limit and inspect temperature, resets, rattles, and clipping.

Pass: mono mix is intelligible and stable, and no reset/overcurrent/thermal problem appears.

## 7 — Combined stress

- [ ] Stream from SD while updating OLED and polling buttons.
- [ ] Alternate DAC/headphone and speaker enable sequences.
- [ ] Record underruns, minimum free heap, queue depth, SD latency, and reset reason.
- [ ] Run at least 60 minutes before calling the prototype stable.

Pass: zero unexplained resets, zero silent errors, and every underrun is counted and visible.

## Evidence returned to ChatGPT Work

- clear photos of both sides of every module and the complete wiring;
- exact labels/revisions and purchase links if different from the BOM;
- console logs for each stage;
- measured 5 V and 3.3 V rails before and during playback;
- current readings for idle, DAC-only, headphones, and speaker;
- SD throughput and error count;
- listening result for left/right, noise, pop/click, clipping, and speaker stability;
- any deviation or bodge.
