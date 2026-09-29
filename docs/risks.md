# Risk Register

Status values remain `OPEN` until evidence closes them.

| Risk | Probability | Impact | Evidence | Mitigation | Verification | Status |
|---|---|---|---|---|---|---|
| ESP32/SD peak current collapses 3.3 V | Medium | High | ESP32 radio-class peaks and SD inrush exceed quiet averages even with radios disabled | 2 A-class buck-boost, local bulk/decoupling, radios disabled in product | scope 3V3 at boot, scan, skip, max decode/UI load | OPEN |
| TPS63802 transient or layout instability | Medium | High | fast converter with small inductor/exposed pad is layout-sensitive | copy datasheet layout/current loops; validated L/C; margin | load-step and ripple measurement across battery range | OPEN |
| switching regulator noise reaches headphones | Medium | High | buck-boost switching can couple by rail/ground/field | zoning, return-path control, forced PWM experiment, local filtering | noise spectrum/listening at silence and load transitions | OPEN |
| charger overheats | Medium | High | linear dissipation rises with 5 V input, low battery, playback load | conservative charge current, thermal copper, NTC/thermal regulation | temperature during worst charge + playback | OPEN |
| charge-while-play starves load or adds noise | Medium | High | input limit shared by charger/system | power-path config, charge-current reduction, UI status | current/rail/audio test while plugging/unplugging USB | OPEN |
| battery lacks protection or polarity differs | Low–Medium | Critical | JST-PH polarity is not universal | protected named pack; keyed/marked connector; polarity DMM gate | visual + polarity check before connection | OPEN |
| battery capacity misses full-night target | Medium | High | no current measurements exist | defer capacity; measure modes; apply derating | continuous declared runtime test | OPEN |
| headphone level unsafe or clips | Medium | High | 2.1 Vrms DAC and gain selection can exceed comfortable level | conservative digital max/gain; ramp; optional limiter | resistive-load scope + listening at staged volume | OPEN |
| headphone load incompatibility | Low–Medium | High | real headphones vary 16–300 Ω and cable behavior | TPA6132A2 rated path; test common loads | distortion/level/thermal across loads | OPEN |
| DAC/headphone gain or DC/pop issue | Medium | High | independent startup/shutdown sequencing | enable/mute state machine and ramps | hot-plug/start/stop scope/listening | OPEN |
| Class-D output tied to ground | Low | Critical | BTL outputs are both active | keyed 2-wire connector, schematic labels, docs | continuity review before power | OPEN |
| Class-D EMI couples into DAC/SD | Medium | High | high dV/dt speaker loop | tight differential route, zoning, edge control, optional filter pads | EMI/noise/SD-error stress at high output | OPEN |
| I2S fanout edge/ringing | Low–Medium | Medium | one ESP output drives DAC + speaker amp | short routes, source series-resistor options | logic-analyzer/scope edges and error-free playback | OPEN |
| SD latency causes underruns | Medium | High | cards and fragmentation vary | two-stage buffers, chunk prefetch, measurements | worst-latency + two-hour stress | OPEN |
| SD removal crashes or deadlocks | Medium | High | file/block driver can fail mid-read | card detect, generation cancel, bounded timeouts | repeated removal at read/decode/idle states | OPEN |
| MP3 decode misses real-time deadline | Low–Medium | High | library not yet benchmarked on exact build | fixed-point Helix candidate, dedicated task, telemetry | representative bitrate/frame timing | OPEN |
| corrupt MP3 causes infinite resync | Medium | Medium | sync bytes can appear in ID3/corrupt data | explicit ID3 skip and bounded resync budget | fuzz/truncated corpus tests | OPEN |
| sample-rate transition pops or misclocks | Medium | Medium | I2S needs reconfiguration | drain/mute/reconfigure/prefill/ramp | alternating 44.1/48 kHz stress | OPEN |
| PCM stereo-to-mono overflow | Low | High | 16-bit L+R can overflow | 32-bit sum, divide, saturate, tests | max-amplitude vector tests | OPEN |
| analog ground/return mistakes add hum | Medium | High | mixed signal + USB + headphone cables | continuous planes, controlled current paths, no blind splits | silence/noise test battery and USB-powered | OPEN |
| QFN/WSON/FC2QFN assembly defects | Medium | High | several exposed-pad fine-pitch parts | fab assembly capability, stencil, inspection/test points | X-ray/visual + electrical bring-up | OPEN |
| wrong footprint/pin-1/orientation | Medium | Critical | connector and leadless drawings are easy to misread | manufacturer land patterns; independent cross-check | print/1:1 overlay and second-person review | OPEN |
| connector orientation blocks enclosure | Medium | High | USB/SD/jack are edge-critical | co-design PCB/CAD, STEP models, datum table | assembled CAD/interference and physical fit | OPEN |
| speaker acoustic cavity rattles/muffles | High | Medium | bare speaker specs do not predict enclosure response | gasket, rigid mount, cavity/grille iterations | sweeps/listening/fit prototype | OPEN |
| enclosure tolerance damages battery | Low–Medium | Critical | pouch cells must not be pinched/punctured | cradle clearance, no sharp bosses, serviceable retention | CAD section review + unpowered fit test | OPEN |
| OLED current/burn-in hurts runtime | Medium | Medium | content and on-time control OLED power | timeout/dim, limited static icons, power gate option | display current/on-time measurement | OPEN |
| USB-C sink misconfiguration | Low–Medium | High | both CC pins need Rd; connector pin duplication easy | separate 5.1 kΩ Rd, exact footprint, ESD review | CC/VBUS test with flipped cables/sources | OPEN |
| component unavailable in purchase region | Medium | High | N16R8 and older MAX98357A listings showed supply constraints | backups, regional recheck, no lock before order | Phase 7 supplier/lead-time audit | OPEN |
| current BOM misses passives/mechanics | High | Medium | Phase 1 block-level BOM only | expand after reference schematics and CAD | schematic-BOM-PCB cross-check | OPEN |
| AI authorship violates Pixl rules | Medium | Critical | fully AI-generated hardware designs can be rejected | disclosure, human material review/authorship, second-person sanity check | documented human review before submission | OPEN |
