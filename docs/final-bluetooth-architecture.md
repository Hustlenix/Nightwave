# Bluetooth architecture closure record - 2026-10-06

Conclusion: S3 decoded PCM -> BM83 I2S input -> A2DP headphones/speaker is
documented, but current firmware still uses UnavailableBluetooth. Capability
is not completed integration or physical proof. No PCB changes are authorized.

## SKU and firmware

Selected BM83SM1-00TA ships with AT v1.0 in Host mode. Its source path uses
SBC44.1/48 kHz, I2S or analog input. Tx AVRCP and HFP audio gateway are absent.
Use one receiver with concurrent BLE/dual-link disabled initially: published
errata include reconnection audio breaks and particular TWS incompatibility.
Pairing/discovery and auto reconnect exist, but a confirmed link event must
precede PCM release. [AT release notes](https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/ReleaseNotes/BM83_AT_v1.0_Release_Notes.pdf).

New sourcing issue: [Mouser TA listing](https://www.mouser.in/en/ProductDetail/Microchip-Technology/BM83SM1-00TA?qs=W%2FMpXkg%252BdQ4DzNNx0q4h4Q%3D%3D)
marks TA NRND. Its indexed quantity-one INR1136.80 is a snapshot, not a quote.
[TB listing](https://www.mouser.in/en/ProductDetail/Microchip-Technology/BM83SM1-00TB?qs=XAiT9M5g4x%2FwGa5Oh0NFkA%3D%3D)
shows INR1165.59 and2140 stock (retrieved2026-10-06; shipping/tax unknown).
TB has [ATv1.0.1 release notes](https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/ReleaseNotes/BM83-AT-v1.0.1-Release-Notes.pdf).
Recommendation: investigate TB for a new design. Builder decision requested;
no silent substitution. Firmware/package compatibility remains engineering work.

Actual local archive inspection: downloaded IS2083_Turnkey_1.2.0.zip contains
Config GUI1.3.08, isUpdate, SPKCommandSet206.003 and MSPK2v1.3 images; it does
NOT establish possession of an AT image. Do not flash MSPK image as a substitute.
TA documentation names Turnkey1.1.0, CommandSet2.07, Config1.2.25, isUpdate297.
Recoverable AT image and matching configuration workflow still need verification.

## Electrical/software interface requirements

S3 I2S clock host, BM83 client is the proposed single-clock configuration.
DR1 receives PCM; RFS1/SCLK1 are frame/bit clocks. MCLK1 is BM83 OUTPUT, never
drive it from S3. Use PCM16 stereo44.1/48 kHz initially; general silicon I2S
formats do not imply AT source encoder support at every rate. Low-rate tracks
need tested conversion; current HAL rejects unsupported rates.
[Module datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/WSG/ProductDocuments/DataSheets/BM83-Bluetooth-Stereo-Audio-Module-Data-Sheet-DS70005402.pdf).

Reserve UART host/wake/reset and independently accessible programming pins,
ground and supply; service must work without booting normal S3 software.
Provision source/client format then read back mode/input/rate. Supply3.6 V is
the reference target, not direct SYS. Disable/isolate internal charging and
prevent I2S/UART backfeed when rail is off. Decoupling, pull states and exact
service pin map must be reconciled against the selected firmware/module revision.
Both RF modules need manufacturer all-layer antenna exclusions; no invented
universal inter-antenna distance. Metal battery/speaker/fasteners count as obstacles.

50/100 mA AT-source loads are ESTIMATED. Published internal-codec receiver
current and continuous RF test currents are not source-playback measurements.
Shutdown cannot be modelled as zero until off-state pin leakage/backfeed is bounded.

## Digital implementation status

Implemented: bounded eight-device discovery, event epochs, connection/start
timeouts, PCM validation/backpressure, disconnect stop request, reconnect and
pairing states. Added checksummed versioned64-byte preferred-endpoint NVS record,
failure-safe decode and commit-result handling. Clearing the preference is NOT
erasing BM83 bond keys. New host tests cover every corrupted record byte, partial
records, NVS failures, stale events and timer wrap. No automatic speaker fallback.

Still engineering work: actual UART command queue/ACK/error recovery, variable
discovery event parsing, bond erase confirmation, output-router mute sequencing,
UI/persistence integration, PCM clock/rate conversion and diagnostics wiring.
Keep firmware backend unavailable until those are implemented; do not represent
these software tasks as awaiting physical hardware alone.

Best architectural fallback remains replacing S3 with ESP32-WROVER-E-N8R8:
single Classic-Bluetooth-capable MCU/antenna, but native USB is lost and GPIO,
decoder/SBC memory, timing and firmware need migration. Keeping S3 plus another
ESP32 adds a second firmware/transport and does not reduce RF integration work.
Not recommended over qualified BM83 solely to avoid resolving provisioning.
[Espressif A2DP API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_a2dp.html).
