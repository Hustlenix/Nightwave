# Component Comparison

Checked: 2026-09-29. Prices are indicative one-off USD snapshots where an official/distributor page exposed a price; `recheck` means a purchase-time quote is required. None of these parts has been purchased.

## MCU module and development board

2026-10-01 update: this table remains a historical price/comparison snapshot.
The power model now identifies Adafruit 353 / 6600 mAh as a provisional candidate
and a BQ25185 charge-timer conflict. See power-budget.md and power-assumptions.md;
neither charger nor final capacity is locked by the older table below.

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| ESP32-S3 module, 16 MB/8 MB | ESP32-S3-WROOM-1-N16R8 | 3.0–3.6 V | dual-core LX7, 16 MB flash, 8 MB octal PSRAM, PCB antenna | 41-pad module | peaks require regulator margin | memory headroom; certified module; many GPIO | GPIO33–37 consumed by octal PSRAM; antenna keepout | Medium, castellated/LGA ground pad | Active; DigiKey showed 0 with incoming stock | 6.76 | [Espressif](https://www.espressif.com/en/products/modules/esp32/1000) | PROVISIONAL SELECT |
| ESP32-S3 module, 8 MB/8 MB | ESP32-S3-WROOM-1-N8R8 | 3.0–3.6 V | 8 MB flash, 8 MB octal PSRAM | module | similar | enough for V1; may have better stock | less flash margin | Medium | Active, recheck | recheck | [datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) | BACKUP |
| Prototype board | ESP32-S3-DevKitC-1-N8R8 | USB 5 V / 3.3 V | matching S3 + 8 MB PSRAM, USB/JTAG, headers | dev board | USB powered | official docs; exposed GPIO; no custom soldering | bulky; pin exposure differs from final | Low | Active, recheck exact variant | recheck | [guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/) | PROVISIONAL SELECT |

## microSD interface

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| 1-bit SDMMC | ESP32-S3 peripheral | 3.3 V | CLK/CMD/D0; 20/40 MHz; CRC | internal | card dependent | native protocol, 3 bus pins, leaves D1–D3 free | dedicated bus and pull-ups | Low electrically; routing care | built in | 0 | [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32s3/api-reference/peripherals/sdmmc_host.html) | PROVISIONAL SELECT |
| 4-bit SDMMC | ESP32-S3 peripheral | 3.3 V | CLK/CMD/D0–D3 | internal | card dependent | maximum bandwidth headroom | six bus pins; extra pull-ups/routing with no proven need | Medium | built in | 0 | [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32s3/api-reference/peripherals/sdmmc_host.html) | REJECT for V1 |
| SDSPI | ESP32-S3 SPI host | 3.3 V | CS/SCLK/MOSI/MISO | internal | card dependent | broad module compatibility; shareable bus | arbitration, lower practical throughput, no native 4-bit path | Low | built in | 0 | [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/sdmmc.html) | BACKUP |

## Stereo DAC

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| TI stereo DAC | PCM5102APWR | 3.3 V analog/digital | 2.1 Vrms, 112 dB SNR, 8–384 kHz, 32-bit I2S, internal PLL/no MCLK | TSSOP-20 | recheck operating draw | proven, hardware controlled, easy package | line output still needs headphone amp; newer TI alternatives exist | Low–Medium | Active | recheck | [TI](https://www.ti.com/product/PCM5102A) | PROVISIONAL SELECT |
| TI integrated newer DAC | TAD5242 | check datasheet | 120 dB class, integrated line/headphone driver | QFN | recheck | potentially removes headphone amp | newer integration increases software/supply risk; not prototype-matched | High | Active | recheck | [TI comparison](https://www.ti.com/product/PCM5102A) | BACKUP |
| Cirrus stereo DAC | CS4344-CZZ | 3.3/5 V domains | 24-bit, up to 192 kHz, I2S | TSSOP-10 | recheck | compact and established | MCLK/clocking and output filtering must be checked | Low–Medium | recheck | recheck | [Cirrus](https://www.cirrus.com/products/cs4344-45-48/) | BACKUP |

## Headphone amplifier

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| DirectPath stereo amp | TPA6132A2RTER | 2.3–5.5 V | 25 mW/ch into 16 Ω at 2.3 V, –6/0/+3/+6 dB, pop suppression | 16-WQFN 3×3 mm | 2.1 mA typ; <1 µA shutdown | capless 0 V outputs; differential inputs; high PSRR | QFN/exposed pad; output level/gain needs listening and safety validation | High | Active; 1,760 seen in stock | 1.10 | [TI](https://www.ti.com/product/TPA6132A2) | PROVISIONAL SELECT |
| TI output-cap amp | TPA6111A2DGN | 2.5–5.5 V | stereo headphone amplifier | HVSSOP | recheck | easier exposed-lead package | output capacitors and pop/size penalties | Medium | Active, recheck | recheck | [TI](https://www.ti.com/product/TPA6111A2) | BACKUP |
| ADI DirectDrive amp | MAX97220A | 2.5–5.5 V | stereo headphone amplifier, click/pop controls | TDFN | recheck | capless output alternative | leadless package and supply check | High | Production, recheck | recheck | [ADI](https://www.analog.com/en/products/max97220a.html) | BACKUP |

## Speaker amplifier

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| Ramped digital Class-D | MAX98360CEFB+T | 2.5–5.5 V | 3.2 W into 4 Ω at 5 V; I2S; mono mix; no MCLK; 13 ms ramp | 10-pin FC2QFN 0.5 mm | 2.2 mA IQ; 92% at 1 W/8 Ω | current production; ramp; lower noise; compact | fine-pitch leadless assembly; no easy generic breakout | High | Production | 1.33 class list price | [ADI](https://www.analog.com/en/products/MAX98360A.html) | PROVISIONAL SELECT |
| Common digital Class-D | MAX98357AETE+T | 2.5–5.5 V | 3.2 W/4 Ω at 5 V; I2S; mono mix; no MCLK | 16-TQFN 3×3 mm | 2.4 mA IQ; 92% at 1 W/8 Ω | excellent breakout ecosystem and prototype evidence | DigiKey base listing constrained/discontinued; older device | High | Production at ADI; supplier risk | 4.08 | [ADI](https://www.analog.com/en/products/max98357a.html) | BACKUP / PROTOTYPE |
| 14 V digital Class-D | MAX98365AEWC+T | 3–14 V | up to 13.8 W/8 Ω at 14 V; I2S | WLCSP | 30 mW IQ | recommended for new designs | unnecessary rail/power/assembly complexity for pocket 1S system | Very high | Recommended for new designs | 1.01 list | [ADI](https://www.analog.com/en/products/max98365.html) | REJECT |

## Charger / power path

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| Modern standalone power-path charger | BQ25185DLHR | 3–18 V input | 1-cell, up to 1 A charge, 4.5 V SYS, thermistor/thermal regulation, 3.125 A system discharge | 10-WSON 2.2×2 mm | 4 µA battery-only IQ | low IQ; robust input; charge/system control | tiny exposed-pad package; linear heat at high charge current | High | Active | recheck | [TI](https://www.ti.com/product/BQ25185) | PROVISIONAL SELECT |
| Established power-path charger | BQ24074RGTR | 4.35–10.2 V input | 1-cell, up to 1.5 A, fixed 4.2 V, 4.4 V system output | 16-VQFN 3×3 mm | 6.5 µA battery sleep max cited | proven, larger QFN, many examples | higher IQ/older; heat; fixed battery voltage | High | Active | recheck | [TI](https://www.ti.com/product/BQ24074) | BACKUP |
| Load-sharing charger | MCP73871-2CCI/ML | USB/adaptor input | 1-cell linear charger with system load sharing | QFN-20 | recheck | established; flexible source selection | larger BOM/package; older architecture | High | In production, recheck | recheck | [Microchip](https://ww1.microchip.com/downloads/en/DeviceDoc/MCP73871-Data-Sheet-DS20002090F.pdf) | BACKUP |

## 3.3 V regulator

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| 2 A buck-boost | TPS63802DLAR | 1.3–5.5 V | 3.3 V adjustable, 2 A for VIN≥2.3 V, load disconnect, forced PWM | 10-VSON-HR 3×2 mm | 11 µA IQ | uses full 1S range; ESP peak margin; low IQ | demanding layout/0.47 µH inductor; switching-noise validation | High | Active | recheck | [TI](https://www.ti.com/product/TPS63802) | PROVISIONAL SELECT |
| Wide-input buck-boost | TPS63070RNMR | 2–16 V | up to 2 A, 2.4 MHz, PFM/PWM, load disconnect | 15-VQFN-HR | 50 µA IQ | robust output; easy headroom | higher IQ; over-wide input; more loss/complexity | High | Active | recheck | [TI](https://www.ti.com/product/TPS63070) | BACKUP |
| 750 mA buck | TPS62840DGRR | 1.8–6.5 V | 750 mA, 100% duty | VSON | 60 nA IQ | excellent idle efficiency, RF-friendly | loses 3.3 V regulation as battery falls; marginal peak margin | High | Active | recheck | [TI](https://www.ti.com/product/TPS62840) | REJECT for main rail |
| 600 mA LDO | AP2112K-3.3TRG1 | up to 6 V | 3.3 V, 600 mA | SOT-25 | low IQ class | cheap/easy/quiet | wastes energy; dropout loses low-SOC capacity; current margin weak | Low | Active | 0.28 snapshot | [Diodes](https://www.diodes.com/part/view/AP2112) | REJECT for main rail |

## Fuel gauge

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| ModelGauge 1S | MAX17048G+T10 | cell 2.5–4.5 V | SOC/voltage/rate over I2C, no sense resistor | 8-TDFN 2×2 mm | 3 µA hibernate; 23 µA active | low BOM and power; alert output | cell model/temperature compensation still needs validation | High | Recommended for new designs | 1.64 at 1k list | [ADI](https://www.analog.com/en/products/max17048.html) | PROVISIONAL SELECT |
| Advanced ModelGauge | MAX17055 | 1S | ModelGauge m5, current sensing/coulomb features | TDFN/WLP | recheck | richer diagnostics and accuracy potential | sense resistor/BOM/config complexity | High | Recommended, recheck | recheck | [ADI](https://www.analog.com/en/products/max17055.html) | BACKUP |
| OCV gauge | LC709203F | 1S | I2C, no sense resistor | WDFN | low power | common module ecosystem | lifecycle/supply must be checked at lock | High | recheck | recheck | [onsemi](https://www.onsemi.com/products/power-management/battery-management/battery-fuel-gauges/lc709203f) | BACKUP |

## Display

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| 1.3 in OLED module | SP12864-13 | module 3.3 V target | 128×64, SH1106, I2C/SPI/parallel | module | content dependent | readable, low pin count, mechanical module | vendor price/connector must be confirmed; OLED burn-in | Low | Vendor quote/check | recheck | [SantoP](https://santopdisplay.com/1-3-inch-oled-display/) | PROVISIONAL SELECT |
| 0.96 in SSD1306 module | exact vendor TBD | 3.3 V | 128×64, I2C | module | content dependent | ubiquitous and cheap | too small for filenames; clones vary | Low | broad but inconsistent | recheck | [SSD1306 datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) | BACKUP |
| 1.54 in memory LCD | exact Sharp part TBD | 3.3 V | reflective, very low static power | FPC/module | low static | sunlight readable, low power | cost/library/mechanical complexity; no deep black UI | Medium | recheck | recheck | [Sharp displays](https://global.sharp/products/device/lineup/selection/lcd/memory/) | REJECT for first prototype |

## Connectors, speaker, battery, USB-C

| Candidate | MPN | Supply | Important Specs | Package | Power | Pros | Cons | Assembly Risk | Availability | Approx Cost | Source | Decision |
|---|---|---|---|---|---|---|---|---|---|---:|---|---|
| switched 3.5 mm jack | SJ-3503-SMT-TR | passive | TRS, two internal switches, right-angle mid-mount | SMT/mechanical | none | physical detect contacts; published CAD | board-edge/CAD coordination; switch truth table validation | Medium | Active; recheck stock | recheck | [Same Sky](https://www.sameskydevices.com/product/interconnect/connectors/audio-connectors/jacks/sj-3503-smt-tr) | PROVISIONAL SELECT |
| microSD socket | 104031-0811 | 3.3 V bus | push-pull, 1.42 mm, card detect, 10k cycles | SMT/mechanical | none | exact drawing; retention tabs | fine contacts and edge orientation | Medium | Published; check supplier | recheck | [Molex](https://www.molex.com/en-us/products/part-detail/1040310811) | PROVISIONAL SELECT |
| 28 mm speaker | CMS-28528N-L152 | amplifier load | 8 Ω, 2 W nominal, 28×5.2 mm, 99 dB at 1 W/0.1 m | wire leads | program dependent | slim; useful power margin | needs cavity/gasket/listening validation | Low electrical | Active; check supplier | recheck | [Same Sky](https://www.sameskydevices.com/product/audio/speakers/miniature-%2810-mm~40-mm%29/cms-28528n-l152) | PROVISIONAL SELECT |
| protected LiPo candidate | Adafruit 258 | 3.7 V nominal | 1200 mAh, 34×62×5 mm, JST-PH, protection | pouch | 4.5 Wh nominal | documented size and protection | likely insufficient for target; 500 mA max charge recommendation | Physical safety gate | In stock when checked | 9.95 | [Adafruit](https://www.adafruit.com/product/258) | BACKUP |
| protected LiPo candidate | Adafruit 328 | 3.7 V nominal | 2500 mAh, about 10 Wh, JST-PH, protection | pouch | measured later | larger energy baseline | capacity and enclosure cannot be locked before measurement | Physical safety gate | In stock when checked | 14.95 | [Adafruit](https://www.adafruit.com/category/574) | PROVISIONAL FORM-FACTOR CANDIDATE |
| USB-C receptacle | USB4105-GF-A | 5 V VBUS | USB2.0 16-signal, 5 A rated, shell stakes, 20k cycles | SMT + through-hole stakes | passive | robust mechanical retention; exact drawing/CAD | 16 pins more than power-only minimum | Medium | Active; supplier lead time shown | recheck | [GCT](https://gct.co/files/specs/usb4105-spec.pdf) | PROVISIONAL SELECT |

## Decision summary

The final module, storage width, audio/power ICs, display, jack, socket, speaker, and battery are all **provisional**. Prototype measurements, supply checks in the purchase region, footprint verification, and assembly-service capability must precede Phase 7 lock.
