"""Selected S3/BM83/24382 screening model. All outputs are estimates, never measurements."""
from __future__ import annotations
import json
import math


def bounded(value, name, low, high):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or not low <= value <= high:
        raise ValueError(f"{name} must be finite in {low}..{high}")
    return float(value)


def route(mcu_ma=200, sd_ma=40, display_ma=90, analog_ma=28, control_ma=5,
          speaker_w=0, headphone_w=0, bt_ma=0, main_eff=.85, amp_eff=.80,
          bt_eff=.85, overhead_w=.025):
    currents = [bounded(v, n, 0, 2000) for n, v in
                (("MCU", mcu_ma), ("SD", sd_ma), ("display", display_ma),
                 ("analog", analog_ma), ("controls", control_ma), ("BM83", bt_ma))]
    speaker_w = bounded(speaker_w, "speaker watts", 0, 2)
    headphone_w = bounded(headphone_w, "headphone watts", 0, .25)
    if speaker_w and headphone_w:
        raise ValueError("exclusive output route required")
    main_eff = bounded(main_eff, "main efficiency", .1, 1)
    amp_eff = bounded(amp_eff, "amplifier efficiency", .1, 1)
    bt_eff = bounded(bt_eff, "BT rail efficiency", .1, 1)
    overhead_w = bounded(overhead_w, "overhead", 0, 2)
    main_w = 3.3 * sum(currents[:5]) / 1000
    audio_w = speaker_w / amp_eff + headphone_w / amp_eff
    # Separate provisional 3.7 V BM83 rail: not direct connection to SYS or cell.
    bt_w = 3.7 * currents[5] / 1000
    losses = main_w * (1 / main_eff - 1) + bt_w * (1 / bt_eff - 1)
    cell_w = main_w + losses + audio_w + bt_w + overhead_w
    return {"main_3v3_ma": sum(currents[:5]), "main_output_w": main_w,
            "regulator_loss_w": losses, "audio_input_w": audio_w,
            "bt_output_w": bt_w, "cell_equivalent_w": cell_w}


def energy(power_w, capacity_mah=6600, hours=8, usable=.8, aging=.8, margin=1.2):
    power_w = bounded(power_w, "power", .001, 100)
    capacity_mah = bounded(capacity_mah, "capacity", 100, 30000)
    hours = bounded(hours, "hours", .01, 168)
    usable = bounded(usable, "usable fraction", .1, 1)
    aging = bounded(aging, "aging fraction", .1, 1)
    margin = bounded(margin, "load margin", 1, 3)
    wh = capacity_mah / 1000 * 3.7 * usable * aging
    return {"required_capacity_mah": power_w * hours * margin / (3.7 * usable * aging) * 1000,
            "estimated_runtime_hours": wh / (power_w * margin), "usable_energy_wh": wh}


def charge_screen(capacity_mah, battery_charge_ma, source_ma, system_w, conversion_eff=.85, taper_factor=1.3):
    capacity_mah = bounded(capacity_mah, "capacity", 100, 30000)
    battery_charge_ma = bounded(battery_charge_ma, "charge current", 1, 10000)
    source_ma = bounded(source_ma, "source current", 1, 10000)
    system_w = bounded(system_w, "system watts", 0, 100)
    conversion_eff = bounded(conversion_eff, "source conversion efficiency", .1, 1)
    taper_factor = bounded(taper_factor, "taper factor", 1, 3)
    # This is an optimistic ENERGY ceiling for a switching path, NOT the linear
    # BQ25185 load/current allocator or a permission to draw USB current.
    remaining_w = max(0, 5 * source_ma / 1000 * conversion_eff - system_w)
    available_ma = min(battery_charge_ma, remaining_w / 4.2 * 1000)
    return {"optimistic_charge_current_ma": available_ma,
            "optimistic_charge_hours": capacity_mah / available_ma * taper_factor if available_ma > 0 else None,
            "minimum_source_ma_energy_only": (system_w + 4.2 * battery_charge_ma / 1000) / conversion_eff / 5 * 1000,
            "linear_charger_heat_w_at_3v7": (5 - 3.7) * battery_charge_ma / 1000,
            "safe_charge_configuration_established": False}


def report():
    # All load currents are engineering allowances, except the vendor display
    # maximum; mode reductions are CONDITIONAL on verified gating/dimming.
    profiles = {
        "idle_awake_bt_off_assumed": route(mcu_ma=80, sd_ma=2),
        "speaker_typical_bt_off_assumed": route(speaker_w=.25),
        "speaker_typical_bt_scan": route(speaker_w=.25, bt_ma=50),
        "speaker_worst_practical_screening": route(mcu_ma=300, sd_ma=100, speaker_w=1, bt_ma=100, main_eff=.8, amp_eff=.75, bt_eff=.8),
        "wired_typical_bt_off_assumed": route(headphone_w=.02),
        "bluetooth_typical_local_muted": route(bt_ma=50),
        "optimized_speaker_conditional": route(display_ma=20, analog_ma=0, speaker_w=.25),
        "transient_capacity_screen_not_runtime": route(mcu_ma=500, sd_ma=200, speaker_w=2, bt_ma=100, main_eff=.8, amp_eff=.75, bt_eff=.8),
    }
    for name, value in profiles.items():
        if not name.startswith("transient"):
            value["eight_hour_requirement"] = energy(value["cell_equivalent_w"])
            value["capacity_sensitivity_hours"] = {str(c): energy(value["cell_equivalent_w"], c)["estimated_runtime_hours"] for c in (2500, 4000, 5000, 6600, 8000)}
        value["low_cell_input_ma_at_3v2"] = value["cell_equivalent_w"] / 3.2 * 1000
    typical = profiles["speaker_typical_bt_off_assumed"]["cell_equivalent_w"]
    return {"basis": "ESTIMATED_INPUTS_CALCULATED_OUTPUTS_NOT_MEASURED", "updated": "2026-10-03",
            "hardware": ["ESP32-S3-WROOM-1-N16R8", "BM83SM1-00TA", "Waveshare 24382", "TCA9535PWR"],
            "battery_locked": False, "measured_runtime_hours": None, "profiles": profiles,
            "charge_screen_6600mah_1a_requested": {str(s): charge_screen(6600, 1000, s, typical) for s in (100, 500, 1500, 3000)},
            "limits": "Not a pack/charger/regulator selection. Source permission, linear charger allocation, safety timer, rail limits, peak output headroom, NTC, temperature, actual load/dimming/gating and acoustic volume require reviewed circuits and measurements."}


if __name__ == "__main__":
    print(json.dumps(report(), indent=2, allow_nan=False))
