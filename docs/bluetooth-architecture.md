# Bluetooth architecture — builder decision required

Research checked 2026-10-01. No radio/MCU replacement is selected and no Bluetooth
audio is implemented or bench-proven. This gate precedes final schematic work.

Nightwave must be an **A2DP SOURCE** transmitting decoded local music to ordinary
headphones/speakers. A receiver/sink module, BLE control link, or an advertising
demo does not meet that requirement. ESP32-S3 has LE, not Classic Bluetooth;
the current S3 cannot supply A2DP in software. [Espressif support matrix](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/bt-architecture/overview.html).

## CURRENT OPTION — A: retain S3, add BM83 with Audio Transceiver firmware

- ALTERNATIVE: B, replace the main MCU with original ESP32-WROVER-E-N8R8;
  or C, retain S3 plus an original ESP32 audio coprocessor.
- BENEFITS: keeps the tested S3 storage/Helix/I2S pipeline and native USB;
  dedicated module handles radio/SBC rather than taking the main MCU's CPU/RAM.
- DRAWBACKS: additional supply/control/antenna area, firmware provisioning and
  UART protocol. It is not a plug-in replacement for the current DAC.
- COST: BM83SM1-00AA research listing $13.18 USD, 2,429 listed in stock; module
  only, excludes carrier/programmer/passives/shipping. That SKU is **not proven
  factory-configured as a transmitter**. Exact AT variant/firmware remains open.
  [DigiKey snapshot](https://www.digikey.com/en/products/detail/microchip-technology/BM83SM1-00AA/10444954).
- POWER: allocate a provisional 50 mA cell-side radio allowance, not a verified
  AT-source consumption figure. Datasheet A2DP figures around 12 mA are under
  different codec/role conditions and must not be substituted for our use case.
- COMPLEXITY: medium/high; AT host control, module update access, I2S clock/rate
  matching, shutdown/backfeed and error recovery need qualification.
- T4 VALUE: builder still owns filesystem/decode/buffers/clock/UI/power. Radio
  offload alone neither guarantees nor invalidates a tier.
- RISK: AT firmware/tools acquisition and headphone interoperability are open.
- RECOMMENDATION: preferred **family direction** for preserving completed S3 work,
  conditional on qualifying the exact AT firmware and a one-headphone demo.
  This is an assistant recommendation, not the builder's selection.

Microchip's AT v1.0 release notes explicitly identify A2DP source, I2S/Aux input,
SBC encoding at 44.1/48 kHz, host discovery filtering and reconnection. They
identify BM83SM1-00TA as AT-preprogrammed. That release's transmitter does **not**
support AVRCP and lists dual-link/TWS interruptions. Use one sink, no simultaneous
BLE session; do not promise remote headset transport controls or AAC transmission
from generic receiver codec advertising. Latest usable firmware/package and its
licence/access have not been established. [AT release notes](https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/ReleaseNotes/BM83_AT_v1.0_Release_Notes.pdf).

The module is 32 × 15 × 2.5 mm with an integrated antenna and documented modular
approvals; finished-product compliance is not automatically conferred. Preserve
vendor keep-outs and separate two antennas if S3 is retained. UART host control,
I2S input, reset/wake/provisioning and proper rail limits require exact-reference
tracing. SYS_PWR is internal, not a supply for Nightwave peripherals. Do not
parallel its charger with the chosen Nightwave power path. [BM83 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/BM83-Bluetooth-Stereo-Audio-Module-Data-Sheet-DS70005402D.pdf).

## ALTERNATIVE — B: original ESP32-WROVER-E-N8R8 main MCU

- BENEFITS: one main module/antenna, native Classic A2DP source and 8 MB PSRAM.
- DRAWBACKS: existing S3 GPIO, USB service and octal-PSRAM configuration must be
  replaced; no native S3-style USB. Requires UART bridge/service design.
- COST: module $5.88 USD, 16,392 listed in stock; not a complete board price.
  [DigiKey snapshot](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-WROVER-E-N8R8/11613126).
- POWER: main-MCU radio/CPU load must be profiled; no defensible comparison to
  current S3 decode-only current exists yet. Budget simultaneous MP3 decode/SBC.
- COMPLEXITY: high migration/testing burden; GPIO restrictions, SDMMC, I2S,
  SRAM/IRAM, display DMA and radio contention all change.
- T4 VALUE: more in-house audio transport work, but tier still reviewer-assigned.
- RISK: squeezed internal memory and additional SBC/resampling work.
- RECOMMENDATION: choose only if lower hardware cost/single-module simplicity
  outweighs migration risk and the builder accepts the platform change.

ESP-IDF 6.1 documents source connect/send APIs and an official source example.
Its send API accepts **encoded** audio buffers, with MTU/ownership/queue rules.
Do not paste an older PCM callback example into 6.1 and assume it works. Pin and
test the chosen IDF/example/encoder combination first. Sink delay reports can
inform latency compensation, but are not acoustic ground truth.
[ESP-IDF A2DP source API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_a2dp.html).

## ALTERNATIVE — C: S3 plus original ESP32 coprocessor

BENEFITS: preserves S3 and uses vendor-supported A2DP APIs. DRAWBACKS: two custom
firmware images, I2S-receive/clock bridge, queue/framing/UART protocol and two
antennas. COST: above $5.88 module-only plus support circuitry. POWER: second
MCU/radio must be measured; allocate a larger allowance than BM83 until proven.
COMPLEXITY/RISK: highest two-system integration load. T4 VALUE: genuine work,
not a reason to accept excessive complexity. RECOMMENDATION: fallback if BM83
AT package cannot be qualified and retaining S3 is essential.

## Integration contract after choice (not implemented radio behavior)

Discovery -> bounded device list -> explicit selection -> pair/connect -> start
stream -> disconnect/error -> pause/mute. Never blast the speaker on BT loss.
Persist preferred device identity only after a successful choice. Bound UART
frames/timeouts and connection retries; callbacks enqueue events, never read SD
or allocate a lyric document. Outputs are mutually exclusive.

A needs 44.1/48 kHz conversion for current 22.05/32 kHz support, validated
anti-alias filtering, clocks, buffering and acceptance-based media clock. Use
measured/reported BT latency with a user correction; declare unsynchronised
lyrics until calibrated. Allocate 100–300 ms as an **initial test window**, not
a vendor guarantee. Measure latency, drift and reconnect across actual receivers.

## Stop/decision gate

Builder must choose A, B or C and state actual reasons in BD-14. No final MCU,
GPIO, Bluetooth BOM, power or antenna layout lock before that choice. Then
qualify firmware/source role, tool availability, clocks/rates and costs before
schematic approval. No purchase, PCB order or vendor message is authorised here.
