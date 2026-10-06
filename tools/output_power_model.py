"""Exclusive-output design estimates, not measurements or an approved circuit."""
import json
from product_power_budget import route, energy, charge_screen


def profile(mcu, sd, display, analog, speaker=0, headphone=0, bt=0, efficiency=.85):
    # The generic helper's BT rail is historical; calculate 3.6 V separately.
    p = route(mcu_ma=mcu, sd_ma=sd, display_ma=display, analog_ma=analog,
              speaker_w=speaker, headphone_w=headphone, bt_ma=0,
              main_eff=efficiency, amp_eff=.8 if efficiency == .85 else .75)
    bt_output = 3.6 * bt / 1000
    p["bt_output_w"] = bt_output
    p["regulator_loss_w"] += bt_output * (1 / efficiency - 1)
    p["cell_equivalent_w"] += bt_output / efficiency
    p["inputs_ESTIMATED"] = dict(mcu_ma=mcu, sd_ma=sd, display_ma=display,
                                  analog_ma=analog, speaker_output_w=speaker,
                                  headphone_output_w=headphone, bt_ma=bt,
                                  rail_efficiency=efficiency)
    p["battery_current_ma_CALCULATED"] = {
        str(v): 1000 * p["cell_equivalent_w"] / v for v in (3.2, 3.7, 4.2)}
    p["runtime_hours_CALCULATED"] = {
        str(c): energy(p["cell_equivalent_w"], c)["estimated_runtime_hours"]
        for c in (3000, 4000, 5000, 6000)}
    return p


def report():
    modes = {
        "speaker": {"typical": profile(200, 40, 90, 28, speaker=.25),
                    "conservative": profile(300, 100, 90, 28, speaker=1, efficiency=.8)},
        "wired": {"typical": profile(200, 40, 90, 28, headphone=.02),
                  "conservative": profile(300, 100, 90, 28, headphone=.1, efficiency=.8)},
        "bluetooth": {"typical": profile(200, 40, 90, 28, bt=50),
                      "conservative": profile(300, 100, 90, 28, bt=100, efficiency=.8)},
        "idle_paused": {"typical": profile(80, 2, 20, 28),
                        "conservative": profile(200, 40, 90, 28, efficiency=.8)},
        "scan_heavy_ui": {"typical": profile(250, 80, 90, 28),
                          "conservative": profile(500, 200, 90, 28, efficiency=.8)},
    }
    charging = {}
    for mode in ("speaker", "wired", "bluetooth"):
        charging[mode] = {}
        for level, p in modes[mode].items():
            charging[mode][level] = {}
            for source in (100, 500, 1500):
                c = charge_screen(5000, 960, source, p["cell_equivalent_w"])
                c.pop("linear_charger_heat_w_at_3v7")
                available_w = 5 * source / 1000 * .85
                c["battery_discharge_deficit_w"] = max(0, p["cell_equivalent_w"] - available_w)
                c["input_conversion_heat_w_at_full_source"] = 5 * source / 1000 * .15
                charging[mode][level][str(source)] = c
    return {
        "classification": {"load_currents_and_efficiencies": "ESTIMATED",
                           "power_current_runtime_charge_outputs": "CALCULATED",
                           "measured": None},
        "battery_selected": None, "hardware_approved": False,
        "runtime_assumptions": {"nominal_v": 3.7, "usable": .8, "aging": .8, "load_margin": 1.2},
        "prerequisites": [
            "Exactly one audio output; no simultaneous speaker and Bluetooth streaming",
            "Non-Bluetooth modes require verified BM83 shutdown/isolation; not implemented by muting alone",
            "28 mA analog quiescent allowance retained even in Bluetooth mode; no fictional DAC power gating",
            "20 mA paused display input requires validated dimming; otherwise use conservative case",
            "Scan mode is paused, not a simultaneous playback stress profile",
            "5 V source currents are conditional permissions, never inferred from connector shape",
            "960 mA requested charge is a proposed limit, not a programmed or pack-approved value",
        ],
        "modes": modes, "charging_while_playing_CALCULATED": charging,
        "thermal_ESTIMATED": {
            "speaker": "Class-D dissipation and charger heat near cell; verify acoustic level and thermal separation",
            "wired": "DAC/headphone quiescent plus charge-pump and regulator heat; qualify load and listening level",
            "bluetooth": "Second regulator/radio heat and antenna separation; measure actual AT source current",
            "idle_paused": "Not deep sleep; retained rails contribute; do not claim standby runtime",
            "scan_heavy_ui": "SD bursts and MCU peaks; rail current margin and brownout tests required",
            "charging_while_playing": "Charge reduces first on weak source; battery may still discharge. No runtime claim while externally powered",
        },
    }


if __name__ == "__main__":
    print(json.dumps(report(), indent=2, allow_nan=False))
