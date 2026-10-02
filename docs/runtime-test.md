# Continuous runtime recording

The Trial says "a full night" without a numeric duration. Nightwave's internal
acceptance target is at least eight continuous hours, not a quoted Pixl minimum.
No physical runtime has been measured. This recorder does not award a pass.

## Software operation

Once supported local audio is playing at nonzero volume, use:

```text
runtime start
runtime status
runtime stop
```

`runtime stop` stops the recorder, not the music. Repeated start is refused while
a recording is active. Start/end/status and minute summaries emit bounded
`nightwave_runtime` JSON. Observation runs about once per second from the UI
owner, outside the audio task. No SD writes, NVS wear or heap allocation are added.

The engine supplies boot-lifetime accepted-media, error and underrun counters,
which survive track/seek changes. Accepted frames exclude starvation silence;
the frame counter wraps modulo 32 bits and observations handle that wrap.
This measures software acceptance by I2S, not sound leaving the speaker.

- `elapsed_ms`: wall time since the explicit start in this boot.
- `observed_play_ms`: sampled intervals with active, unpaused, nonzero-volume
  playback, stable route/volume/rate, sufficient accepted-frame progress and no
  newly observed fault. DMA lead allows up to 4096 frames of scheduling variation.
- `battery_play_ms`: those intervals additionally have fresh, plausible battery
  telemetry from a separately qualified power adapter and an acoustically verified
  route. The current `UnavailablePower`/untested bench integration gives **zero**.
- Gaps over two seconds, pauses/stalls/faults and configuration changes remain
  visible. Backwards time terminates recording. Duplicate timestamps add no time.

Every report retains `physical_pass:false` and `human_review_required:true`,
including the synthetic eight-hour regression fixture. A scheduler observation
cannot independently prove uninterrupted audible playback, calibrated voltage,
source authenticity, battery capacity or measured runtime. Short between-sample
events can be missed; retain external observations and original device logs.
The eight-hour test in CI is a rapidly simulated timeline, not an eight-hour run.

## Later physical protocol — not executed

First complete and review the exact protected battery, charger, source-current,
regulator, fuel-gauge and shutdown design. Do not connect a cell to the provisional
USB-only bench wiring. Follow the reviewed production power/bring-up procedure.

For the full-night profile, record the actual board/firmware hash, battery model
and capacity, full-charge conditions, speaker/load, fixed volume, same-rate local
playlist/repeat mode, display timeout/backlight, ambient conditions and logger.
The speaker route must work without any phone/app/network or Bluetooth receiver.
Use a separately reviewed logging setup that does not power or charge the device;
USB-powered playback is not battery-runtime evidence.

Capture actual start/stop times externally, real periodic voltage/SOC readings,
fault/underrun logs and whether audio continues. Run to the reviewed controlled
cutoff; never defeat battery protection or discharge beyond reviewed limits.
Stop for abnormal heat, unstable rails, protection activation or playback faults.
If power loss prevents a final report, the last minute record is only a lower
bound, not an invented stop time. Record shutdown externally.

Return the raw logs, exact conditions, real readings and video/time evidence.
State the achieved runtime precisely. If it misses eight hours, keep the failure,
identify the load/design cause and repeat after an actual revision.
