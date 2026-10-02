# BD-14 Bluetooth architecture

Checked 2026-10-02. **Builder selected A in chat: retain S3 + BM83SM1-00TA.**
The comparison is preserved for traceability. Selection is not transmitter-tool,
interoperability, power or physical validation. Personal reasons were not supplied.

| Item | A — S3 + BM83 (selected) | B — replace main MCU | C — S3 + ESP32 coprocessor |
| --- | --- | --- | --- |
| Exact hardware | Existing ESP32-S3-WROOM-1-N16R8 plus Microchip **BM83SM1-00TA** AT-preprogrammed module | **ESP32-WROVER-E-N8R8**, 8 MB flash/8 MB PSRAM | Existing S3 plus **ESP32-WROVER-E-N8R8** |
| Profiles/source | AT v1.0: A2DP 1.3 **source**; Tx AVRCP/HFP AG **not supported** | Classic BT 4.2; source A2DP and AVRCP via selected/tested stack | Same Classic source capabilities, second custom firmware |
| S3 interface | PCM I2S + UART host control, MFB/P0_0 wake and provision/reset access | None: platform replacement. Native S3 USB lost; UART service needed | I2S receive or framed SPI PCM + UART/control; clock/queue protocol required |
| Firmware work | Medium/high: bounded AT protocol/events, provision tools, disconnect/mute, rate handling | High: port GPIO/SD/I2S/USB/PSRAM plus SBC/source queues and resource profiling | Highest: two images, transport/clock bridge plus SBC, recovery and updates |
| Audio | AT SBC encode **44.1/48 kHz**, I2S/Aux input. No AAC/AVRCP Tx promise | PCM16 decode -> SBC at qualified 44.1/48 rates; pin encoder/API version | Same, plus bridge framing/clock-domain handling |
| Power | Exact AT-source average unknown. **50 mA at 3.7 V is an estimate**, not a module rating; qualification must replace it | Whole MCU + concurrent decode/SBC/radio unknown; Wi-Fi datasheet current is not BT playback current | Additional MCU/radio load; no credible measured advantage over A |
| Module price/stock | **$12.20; 30** listed; standard lead 17 weeks | **$5.88; 16,392** listed; 8-week standard lead | Extra **$5.88** module, same snapshot; additional support circuitry |
| Area | BM83 **32x15x2.5 mm**, 480 mm² added before pads/keep-out/support | **18x31.4x3.3 mm**, 565.2 mm² main module before keep-out | Adds 565.2 mm² plus bridge/passives to S3 area |
| RF | Integrated antenna; two antenna locations, vendor keep-outs and disabled unused S3 radio | One PCB antenna; follow module land/placement guidance | Two antennas; separation/coexistence qualification |
| Latency | No universal vendor guarantee; endpoint + SBC/buffers dominate | Same, plus configured source queues | Same, plus bridge queues |
| Main risks | AT tool/package access, 22.05/32 conversion, headphone interop, supply/backfeed, Tx has no AVRCP | Platform migration, no native USB, scarce internal RAM, decode/SBC/display contention | Highest firmware/scheduling/clock risk and two-system failure handling |

Prices are USD quantity-one seller snapshots, not quotes; exclude carrier,
programmer, passives, delivery, import tax and assembly. Availability can change.
[BM83 seller](https://www.digikey.com/en/products/detail/microchip-technology/BM83SM1-00TA/12807564),
[WROVER seller](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-WROVER-E-N8R8/11613126).

Recommendation was A to preserve the validated S3 pipeline. The builder has now
chosen it; do not switch to B/C or generic BM83SM1-00AA without a new choice.
S3's BLE does **not** supply Classic A2DP or LE Audio.
[Espressif audio support](https://docs.espressif.com/projects/esp-adf/en/latest/solution-center/bluetooth-audio.html).

## Selected-path qualification and constraints

AT v1.0 identifies UART CommandSet 2.07, ConfigTool 1.2.25, isUpdate 297 and
SPKCommandSet 202.253 under IS2083 Turnkey 1.1.0/AT v1.0. Actual usable package,
access/licence and exact command/event mapping remain unverified. Do not invent
UART opcodes. Release notes identify dual-link/TWS and BLE coexistence problems
and FreeBuds 3 incompatibility. Start with one receiver and no concurrent BLE.
[Microchip AT release notes](https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/ReleaseNotes/BM83_AT_v1.0_Release_Notes.pdf).

BM83 supply/IO limits, MFB/P0_0 wake direction, I2S master/slave clocks and
provisioning access must be traced to the exact module drawing. SYS_PWR is
internal, not a peripheral supply. Disable/isolate the module charger; never
parallel it with Nightwave's charger. Preserve antenna keep-out and modular
approval constraints; module certification is not final-product compliance.
[BM83 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/BM83-Bluetooth-Stereo-Audio-Module-Data-Sheet-DS70005402D.pdf).

22.05/32 kHz tracks need a validated converter for this route; current HAL
explicitly rejects them rather than misclocking. Local outputs retain those
rates. Allocate an initial **100–300 ms test window**, not promised latency;
measure drift/reconnect/endpoint delay and lyric correction. No automatic loud
speaker fallback after radio loss. Unsupported adapter remains unavailable.

Portable `BluetoothSource` bounds discovery to eight devices, checks event
epochs, requires explicit choice, times out discovery/connect/start, propagates
PCM backpressure and requests stop on loss. Callbacks must queue fixed events;
backend connect cancels discovery. It is not a working BM83 UART/radio adapter.
`Bm83Uart` adds fixed 256-byte framing/checksum and fragmented-input timeout
tests from the manufacturer protocol example. It implements no guessed AT
discovery/source command semantics.
[Microchip host UART guide, sections 5.2–5.3](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/BM83_Host_MCU_Firmware_Development_Guide_DS50002896A.pdf).
Do not claim pairing, transmission or runtime before actual qualification.

Alternative ESP32 implementation must pin and test its IDF/SBC combination:
current IDF source send APIs take encoded buffers with MTU/ownership rules,
not an older PCM callback copied blindly.
[ESP-IDF A2DP source API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_a2dp.html),
[WROVER dimensions/RF guidance](https://documentation.espressif.com/esp32-wrover-e_esp32-wrover-ie_datasheet_en.html).
