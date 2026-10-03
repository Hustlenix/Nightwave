# Provisional Power Tree

Current product: ESP32-S3 + selected BM83SM1-00TA and Waveshare 24382 non-touch TFT.
The tree below documents the old local-output/OLED prototype, not a final
three-output circuit. Add a separately qualified BM83 supply within its 3.2–4.2 V
operating range with switched-off/backfeed analysis; do not attach it to an
unbounded SYS rail or power peripherals from SYS_PWR. Disable/isolate its
internal charger when using the product's external protected-pack charger.
Current full screening is [current-power-model.md](current-power-model.md);
no pack, charger or regulator circuit is locked. BQ25185 nominal adapter SYS
is 4.5 V, outside BM83's operating rail range: add a separately reviewed BT
regulator/isolation branch, not a direct SYS connection.
The selected S3 retains native USB service access. Its actual USB circuit,
source-current permission and ESD must be builder-authored and reviewed.

No battery capacity or charge current is locked. The 2026-10-01 conservative
model identifies a 6600 mAh candidate, but the larger pack conflicts with the
preliminary charger's low-current safety timer and peak-load allowance.
See [power budget](power-budget.md) and [constraints](power-assumptions.md).
This tree is provisional and is NOT a complete manufacturable power circuit.

Mentor review update: [power-options.md](power-options.md) compares a switching
charger without selecting it. Builder decides source policy and charger before
this tree becomes authoritative. The native-USB service wording in the BOM
supersedes the historical power-only assumption below; resolve circuitry in BD-01/02 rather
than claiming that GPIO19/20 are already wired.
MAX17048 VDD connects to protected battery positive and senses voltage; it is
not powered from 3V3_MAIN. Only its logic pull-up rail is discussed below.

```text
USB-C VBUS (historical 5 V sink, power-only prototype)
  |-- CC1 5.1 kΩ to GND
  |-- CC2 5.1 kΩ to GND
  |-- connector shield/chassis strategy (review)
  |-- VBUS TVS + input capacitor + optional resettable fuse
  v
BQ25185 charger + dynamic power path
  |-- TS <- pack NTC or validated fixed network only for prototype; final must monitor cell
  |-- status -> ESP32 GPIO
  |-- BAT <-> keyed JST-PH -> protected 1S Li-ion/LiPo
  |                 |
  |                 +-> MAX17048 fuel gauge -> I2C + ALERT -> ESP32
  |
  +-> SYS (~battery/adapter-derived regulated system rail)
       |
       +-> load switch / local bulk -> MAX98360C speaker amp -> differential speaker
       |
       +-> TPS63802 buck-boost -> 3V3_MAIN
              |
              +-> ESP32-S3 module
              +-> microSD (local filtering / inrush review)
              +-> display (power-gate candidate)
              +-> PCM5102A DAC (local filtering; analog layout)
              +-> TPA6132A2 headphone amplifier
              +-> MAX17048 logic/I2C pull-ups as datasheet permits
```

## Design answers

- **USB-C input:** current product requires a reviewed 5 V sink and native USB service path. The old power-only prototype does not satisfy that requirement. Separate CC sink termination/current permission, data ESD, both connector orientations and mechanical stakes must be reviewed.
- **Charger/power path:** BQ25185 is provisional, configured for a standard 4.2 V cell and a charge current no greater than the cell's rating. System load receives priority while the remainder charges the battery.
- **Battery/protection:** use a protected 1S pack with keyed connector and documented polarity. On-cell protection does not replace charger safety or NTC validation.
- **Speaker supply:** provisionally use SYS so the Class-D path can benefit from adapter/system voltage; validate power versus 8 Ω, low-cell behavior, and absolute limits. Do not route the bridge outputs to ground.
- **3.3 V:** TPS63802 buck-boost supplies digital/low-power audio rails. It must survive ESP32 + SD transients at minimum battery voltage.
- **DAC/headphone/display:** 3.3 V with local decoupling. Consider ferrite/RC isolation only after impedance/noise analysis; do not split grounds blindly.
- **Charge while play:** supported architecturally, but charger thermal foldback, input limit, speaker peaks, and audible noise must be measured.
- **Shutdown:** firmware mutes audio, saves bounded settings, powers down display and amps, then releases `POWER_HOLD` if the final circuit uses a load switch/latch. Charger/factory-mode behavior must preserve charging.
- **Power gating:** speaker amp, headphone amp, display, and possibly SD can be disabled. The fuel gauge remains attached to the cell. Exact switches are not selected yet.
- **Measurement points:** USB_VBUS, SYS, BAT, 3V3_MAIN, speaker-amp supply, DAC supply, and headphone-amp supply receive named test points. Provide a removable current link or zero-ohm shunt in prototype/final measurement paths where it does not compromise safety.

## Runtime sizing method

For a measured average load `I_avg` at the cell-equivalent voltage, choose usable capacity using:

```text
C_required_mAh = I_cell_equivalent_mA × target_hours / usable_capacity_fraction
```

Cell-equivalent current already includes conversion losses; do not divide by
regulator efficiency a second time. For rail-power estimates use the separate
energy model in power-budget.md. Apply aging, temperature, protection cutoff,
and load margins. Validate the pack with the declared speaker profile.

## Thermal constraint

BQ25185 is linear: approximate dissipation during charging is `(VBUS - VBAT) × I_charge` plus system-path losses. A nominal 1 A data-sheet capability is not a default setting. Copper area, enclosure temperature, simultaneous playback, cell maximum charge current, and NTC behavior determine the safe programmed value.
