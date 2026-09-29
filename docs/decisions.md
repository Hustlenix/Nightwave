# Engineering Decision Log

## ADR-001 — Use ESP-IDF and ESP32-S3

Date: 2026-09-29  
Status: provisional

### Context
Nightwave needs direct filesystem access, software decode, DMA I2S, FreeRTOS primitives, diagnostics, and predictable control of memory/peripherals.

### Options considered
ESP-IDF on ESP32-S3; Arduino framework; RP2040-class MCU; DFPlayer module.

### Decision
Use ESP-IDF v6.1 and provisionally select ESP32-S3-WROOM-1-N16R8; use ESP32-S3-DevKitC-1-N8R8 for the bench prototype.

### Why
Native drivers and FreeRTOS support fit the architecture, while PSRAM provides noncritical buffer headroom. DFPlayer would hide the core work.

### Consequences
ESP-IDF tooling and C/C++ are required. Octal-PSRAM pins GPIO33–37 are unavailable.

### Validation still required
CI build, prototype CPU/heap/PSRAM profiling, and final-module stock check.

## ADR-002 — Use 1-bit SDMMC, with SDSPI fallback

Date: 2026-09-29  
Status: provisional

### Context
MP3/WAV bandwidth is modest, but worst-case SD latency matters more than headline throughput.

### Options considered
4-bit SDMMC, 1-bit SDMMC, SDSPI.

### Decision
Use 1-bit SDMMC on the final provisional pin map. Allow an SDSPI board configuration for prototype modules if necessary.

### Why
It retains native SD protocol/CRC with three bus pins and avoids spending D1–D3 before measurements demonstrate a need.

### Consequences
Dedicated pull-ups and routing are required. Final throughput/buffer sizing remain measured decisions.

### Validation still required
Sequential throughput, worst read latency, removal/recovery, and card compatibility.

## ADR-003 — Separate headphone and speaker signal paths

Date: 2026-09-29  
Status: provisional

### Context
Stereo headphones and a mono speaker have different load, topology, muting, and EMI requirements.

### Options considered
PCM5102A + TPA6132A2 plus MAX98360C; one analog DAC feeding both amps; integrated codec.

### Decision
Fan out shared I2S to PCM5102A/TPA6132A2 for headphones and MAX98360C for the mono speaker. Use a MAX98357A breakout in the prototype if it is easier to source.

### Why
The paths stay explicit, the speaker receives a saturation-safe L/2+R/2 mix, and each path can be muted or power-managed independently.

### Consequences
More PCB area and mixed-signal layout work. The leadless packages require assembly planning.

### Validation still required
Gain, hiss, pop/click, L/R correctness, mono mix, Class-D EMI, thermal behavior, and speaker acoustics.

## ADR-004 — Use BQ25185 power path and TPS63802 3.3 V buck-boost

Date: 2026-09-29  
Status: provisional

### Context
Nightwave must play while charging, survive ESP32 current transients, and use useful capacity across a 1S discharge curve.

### Options considered
BQ25185, BQ24074, MCP73871; TPS63802, TPS63070, TPS62840 buck, and an LDO.

### Decision
Use BQ25185 plus TPS63802 provisionally. Feed the speaker amp from the system/battery-derived rail according to prototype voltage/power measurements; feed logic/audio from 3.3 V.

### Why
The charger provides current-limited power-path behavior and low battery-only IQ; the converter provides 2 A-class buck-boost headroom and low IQ.

### Consequences
Both parts are small exposed-pad packages and require careful thermal/switching layout. Charge current cannot be set until the cell is selected.

### Validation still required
Transient response, noise, thermals, charge-while-play behavior, and cell-specific current/NTC settings.

## ADR-005 — Use MAX17048 fuel gauge

Date: 2026-09-29  
Status: provisional

### Context
The UI needs defensible SOC/voltage without a large sense-resistor subsystem.

### Options considered
MAX17048, MAX17055, LC709203F, voltage-only ADC estimate.

### Decision
Use MAX17048G+T10 provisionally.

### Why
It is recommended for new designs, low power, I2C, and does not require a sense resistor.

### Consequences
SOC must still be validated against the selected cell and runtime curve.

### Validation still required
Cell-model suitability, cold/load behavior, alert thresholds, and runtime correlation.

## ADR-006 — Use esp-libhelix-mp3 for provisional MP3 decode

Date: 2026-09-29  
Status: provisional

### Context
Nightwave should own filesystem, buffering, tasking, PCM, and I2S without spending project effort writing MP3 math from scratch.

### Options considered
esp-libhelix-mp3, minimp3, dr_mp3, libmad.

### Decision
Use `chmorgan/esp-libhelix-mp3` provisionally as a decode-only component.

### Why
It is an Apache-2.0 ESP component, fixed-point oriented, and exposes frame decode/sync operations without owning the Nightwave pipeline.

### Consequences
Nightwave must handle ID3 skipping, sync recovery, input refills, output formatting, and errors.

### Validation still required
ESP-IDF 6.1 compatibility, license notice, malformed files, frame timing, memory, and sample-rate coverage.

## ADR-007 — Defer battery capacity and final PCB

Date: 2026-09-29  
Status: accepted

### Context
Capacity and board geometry depend on measured current, acoustics, and a working direct-SD audio prototype.

### Options considered
Select a large cell now; estimate from datasheets; defer to measurements.

### Decision
Do not lock battery capacity and do not route/order the final PCB in Phase 0/1.

### Why
Measured speaker/headphone profiles and charge thermals are required for a defensible choice.

### Consequences
CAD envelope and total cost remain provisional.

### Validation still required
Phase 2–6 hardware prototype and current/runtime measurements.
