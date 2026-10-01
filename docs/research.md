# Phase 1 Research Notes

Checked: 2026-09-29

2026-10-01 update: historical research below is not the current component lock.
Model-based battery/power work now proceeds before physical measurements under
the funding request; see power-budget.md and power-assumptions.md for the larger
candidate and unresolved charger/peak-load constraints.

## Research method

Final-candidate facts were taken first from manufacturer product pages, datasheets, mechanical drawings, or official ESP-IDF documentation. Distributor pages were used only for availability and one-off price snapshots. Prices are USD, exclude tax/shipping/tariffs, and must be rechecked before purchasing.

## Firmware and MCU

- Framework baseline: ESP-IDF v6.1, the current official release checked on 2026-09-29.
- Final-module candidate: `ESP32-S3-WROOM-1-N16R8` (16 MB flash, 8 MB octal PSRAM). The memory provides headroom for media buffers while internal SRAM remains reserved for latency-critical DMA/queue structures.
- Prototype board: `ESP32-S3-DevKitC-1-N8R8`. It exposes enough safe GPIO while matching the 8 MB PSRAM characteristic.
- GPIO26–32 are reserved for flash/PSRAM use. GPIO33–37 must also be avoided with octal PSRAM. GPIO0, 3, 45, and 46 are strapping pins; 19/20 are reserved for native USB/JTAG; 43/44 are retained for UART0 diagnostics.

Sources: [ESP32-S3-WROOM-1 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf), [DevKitC-1 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/), [ESP32-S3 GPIO guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html), [ESP-IDF releases](https://github.com/espressif/esp-idf/releases).

## Storage

ESP32-S3 can route either SDMMC slot through the GPIO matrix and supports 1- and 4-line SD modes at 20/40 MHz. Nightwave provisionally selects **1-bit SDMMC**: CLK, CMD, D0, and card-detect consume four pins, provide native SD protocol and CRC, and avoid the extra D1–D3 routing of 4-bit mode. SDSPI remains a firmware fallback for prototype breakouts and could share an SPI bus, but it uses four bus signals before card-detect and adds bus-arbitration concerns. Buffer sizes remain measurement-derived.

Sources: [ESP32-S3 SDMMC host](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32s3/api-reference/peripherals/sdmmc_host.html), [SD pull-up requirements](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/sd_pullup_requirements.html).

## Audio architecture

- Headphones: shared I2S enters a `PCM5102A` stereo DAC, then a `TPA6132A2` stereo headphone amplifier, then a switched TRS jack. The DAC's 2.1 Vrms line output is not connected directly to low-impedance headphones.
- Speaker: the same BCLK/LRCLK/DOUT fanout enters a mono digital Class-D amp. `MAX98360C` is the provisional final candidate because it is in production, supports I2S and a safe L/2+R/2 mix, needs no MCLK, has a 13 ms turn-on/off ramp, and operates from 2.5–5.5 V. A MAX98357A breakout remains appropriate for the bench prototype; its final TQFN supply risk is recorded.
- Firmware owns digital volume and saturation-safe stereo-to-mono policy. The speaker amplifier's differential bridge outputs never connect to ground.

Sources: [PCM5102A](https://www.ti.com/product/PCM5102A), [TPA6132A2](https://www.ti.com/product/TPA6132A2), [MAX98360A–D](https://www.analog.com/en/products/MAX98360A.html), [MAX98357A](https://www.analog.com/en/products/max98357a.html).

## Power

`BQ25185` is the provisional charger/power-path IC. It is active, supports a 1-cell pack, up to 1 A programmed charging, thermal regulation, input/current limits, battery thermistor monitoring, and up to 3.125 A system discharge. Initial charge current will be conservative and matched to the chosen cell.

`TPS63802` is the provisional 3.3 V converter. Its 1.3–5.5 V input, 2 A output capability at relevant conditions, 11 µA typical quiescent current, load disconnect, and buck-boost operation preserve usable battery energy below 3.3 V. Forced PWM is available for noise experiments; layout/filtering and audio-noise measurements are still required.

`MAX17048G+T10` is the provisional fuel gauge. It is recommended for new designs, draws 3 µA in hibernate, uses ModelGauge without a sense resistor, and exposes SOC/voltage over I2C.

Sources: [BQ25185](https://www.ti.com/product/BQ25185), [TPS63802](https://www.ti.com/product/TPS63802), [MAX17048](https://www.analog.com/en/products/max17048.html).

## Mechanical components

- microSD: Molex `104031-0811`, push-pull, 1.42 mm height, card-detect switch, 10,000-cycle rating, with published drawing.
- headphone jack: Same Sky `SJ-3503-SMT-TR`, 3.5 mm TRS, right-angle mid-mount, two internal switch contacts. Detect behavior and the exact switch truth table must be validated from its drawing and prototype.
- USB-C: GCT `USB4105-GF-A`, horizontal 16-signal USB 2.0 receptacle with through-board shell stakes. V1 uses power only, with 5.1 kΩ Rd on both CC pins and unused data pins left unconnected after ESD/layout review.
- speaker: Same Sky `CMS-28528N-L152`, 28 mm × 5.2 mm, 8 Ω, 2 W nominal, wire leads. It requires a rigid mount, grille clearance, gasket/cavity experiments, and listening measurements.
- battery candidates: protected Adafruit 1200 mAh (34 × 62 × 5 mm), 2000 mAh, and 2500 mAh packs with JST-PH. Capacity is intentionally not selected until measured load profiles exist.

Sources: [Molex 104031-0811](https://www.molex.com/en-us/products/part-detail/1040310811), [Same Sky SJ-3503](https://www.sameskydevices.com/product/interconnect/connectors/audio-connectors/jacks/sj-3503-smt-tr), [GCT USB4105 specification](https://gct.co/files/specs/usb4105-spec.pdf), [CMS-28528N-L152](https://www.sameskydevices.com/product/audio/speakers/miniature-%2810-mm~40-mm%29/cms-28528n-l152), [Adafruit batteries](https://www.adafruit.com/category/574).

## Assembly implications

The MCU module and TSSOP DAC are hand-assembly friendly relative to wafer-level packages. The TPA6132A2 WQFN, BQ25185 WSON, TPS63802 VSON-HR, MAX17048 TDFN, and MAX98360C FC2QFN require stencil/reflow, exposed-pad inspection, and an assembly service or controlled hot-air process. Final lock must confirm footprint land patterns and assembly capability.
