# Portable product interfaces

2026-10-02. These are host-tested contracts, not working wireless/display/power
hardware adapters. ESP32-S3 + BM83SM1-00TA is builder-selected; BD-15 is pending.

## Bluetooth

`BluetoothBackend` submits non-blocking discovery/connect/start commands and
reports PCM accepted/would-block/unavailable/fault. A production I2S adapter must
own its clock/DMA path and preserve accepted-sample accounting; it cannot just
return accepted before data is safely owned. Caller buffers are borrowed for
the write call only. Queuing adapters must copy to bounded storage.

`BluetoothSource` has one serialized owner, eight checked discovery entries,
explicit device choice, event epochs, wrap-safe phase timeouts and loss-triggered
stop requests. Backend connect must cancel discovery. Vendor callbacks must
queue fixed-size events; the control object is not ISR/thread-safe. No automatic
speaker fallback. Current 22.05/32 kHz blocks are explicitly rejected for BM83
until a converter is implemented and qualified; local routes still accept them.

`Bm83Uart` covers only documented packet framing/checksum, a 256-byte body cap
and incomplete-packet timeout. `Bm83AtCodec` covers documented fixed commands/
status reports in public command-set v2.08. Exact TA-version compatibility,
connect/discovery payloads, ACK/retry/wake policy and provisioning require further
qualification. The generic MSPK guide alone is not an AT command specification.
Actual AT image/tool execution/licensing remain unresolved; public documentation
access is now established, not treated as a physical-hardware dependency.
The production default remains unavailable and cannot claim pairing/transmission.

## Power

`PowerHal` samples timestamped, validity-tagged voltage/SOC/source/charging and
requests a real controlled shutdown. The default returns unknown and refuses
physical shutdown. No invented SOC or charging status. `BatteryPolicy` refuses
invalid/stale samples and defaults disabled; fixture cutoff/hysteresis values
are not approved settings for a selected cell. Real gauge/charger/latch adapters
follow the builder-reviewed power circuit and updated model after BD-15.

## Display

`DisplaySink` presents capabilities and a synchronous frame containing bounded
full title/artist/album, current/next lyric and timing/state, plus a legacy text
fallback. String pointers are borrowed only during present; a DMA/queued adapter
must copy bounded fields. The OLED bench adapter consumes the fallback. The TFT
must render the full fields and declare glyph/Unicode policy. Preserved UTF-8 is
not complete Unicode rendering. Exact panel init/offset/PWM/sleep awaits BD-15.

## Engineering diagnostics

The fixed 1024-byte report includes library size/read counts, stream seek/index
and storage/decode counters plus internal/PSRAM heap snapshots. Unknown battery
values are JSON null; backend readiness and physical_pass stay false. Formatting
refuses insufficient capacity instead of publishing truncated JSON. Console
unsigned parsing rejects overflow/sign/junk; overlong lines are discarded, not
executed in fragments. None of these software checks is runtime/power evidence.
`analyze_diagnostics.py` accepts checked engineering records and exposes the
latest snapshot; unknown battery remains null. Counter/flag/voltage/SOC bounds
are validated, and even a claimed physical_pass cannot establish acceptance.

CI runs all ten host suites in Release and ASAN with leak detection, the project
validator, engineering arithmetic and the ESP32-S3 firmware build separately.
Executed checkpoint evidence belongs in software-validation.md, not inferred
from test definitions or prior revisions.
