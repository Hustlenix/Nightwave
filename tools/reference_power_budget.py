"""Reference-circuit screening, not measurements or battery/USB authorization."""
from __future__ import annotations
import json
from pathlib import Path
from product_power_budget import route, energy, charge_screen

ROOT = Path(__file__).resolve().parents[1]

def report():
    # Retain auditable load assumptions from the product screening model, but
    # use the actual reference BT rail and reference pack, not legacy 6.6 Ah.
    options = {
        "idle_lit_bt_retained": dict(mcu_ma=80,sd_ma=2,bt_ma=50),
        "speaker_typical_bt_retained": dict(speaker_w=.25,bt_ma=50),
        "wired_typical_bt_retained": dict(headphone_w=.02,bt_ma=50),
        "bluetooth_typical_local_muted": dict(bt_ma=50),
        "worst_practical": dict(mcu_ma=300,sd_ma=100,speaker_w=1,bt_ma=100,main_eff=.8,amp_eff=.75,bt_eff=.8),
        "transient_not_runtime": dict(mcu_ma=500,sd_ma=200,speaker_w=2,bt_ma=100,main_eff=.8,amp_eff=.75,bt_eff=.8),
    }
    profiles={}
    for name, args in options.items():
        bt_ma=args.pop("bt_ma")
        item=route(**args)
        bt_w=3.6*bt_ma/1000
        loss=bt_w*(1/args.get("bt_eff",.85)-1)
        item["bt_output_w"]=bt_w
        item["regulator_loss_w"]+=loss
        item["cell_equivalent_w"]+=bt_w+loss
        item["cell_current_at_3v2_ma"]=item["cell_equivalent_w"]/3.2*1000
        item["cell_current_with_margin_ma"]=item["cell_current_at_3v2_ma"]*1.2
        if name!="transient_not_runtime":
            item["typical_capacity_10050mah"]=energy(item["cell_equivalent_w"],10050)
            item["minimum_capacity_9500mah"]=energy(item["cell_equivalent_w"],9500)
        profiles[name]=item
    # Typical ILIM relation is not a guaranteed low-current tolerance bound.
    ilim_typical=2500/5490*1000
    charging={}
    for label, load in (("system_off_allowance",.03),
                         ("speaker_playback",profiles["speaker_typical_bt_retained"]["cell_equivalent_w"])):
        screen=charge_screen(10050,320,ilim_typical,load)
        screen.pop("linear_charger_heat_w_at_3v7") # Not a linear charger.
        charging[label]=screen
    return {
        "basis":"ESTIMATED_INPUTS_CALCULATED_OUTPUTS_NOT_MEASURED",
        "hardware":{"charger":"BQ25628ERYKR","bt_rail_v":3.6,"pack_reference":"Adafruit 5035",
                    "pack_typical_mah":10050,"pack_minimum_mah":9500,"pack_harness_limit_ma":2000},
        "battery_locked":False,"fabrication_approved":False,"measured_runtime_hours":None,
        "profiles":profiles,
        "charger_reset":{"charge_ma":320,"voltage_mv":4200,"fast_charge_timer_hours":12,
                         "input_register_ma":1600,"ilim_typical_ma":ilim_typical},
        "charging":charging,
        "blocking_findings":[
            "No source detection/enumeration/suspend policy; 455 mA typical ILIM is not USB authorization",
            "10050 mAh / 320 mA = 31.4 h ideal CC, longer than 12 h default fast-charge timer; not a complete charging design",
            "Transient load screen exceeds the pack harness 2 A limit; current/load limiting or an approved alternative is required",
            "Pack specifies charging 0..45 C; reference NTC and JEITA defaults require an explicit compatible-temperature review",
            "Charger CE tied low and no qualified charging-control backend; telemetry cannot enforce safe reset behavior",
            "Power loop geometry, effective capacitor bias, thermal rise, USB protection and actual currents remain unqualified",
        ],
        "sources":["https://www.ti.com/lit/ds/symlink/bq25628e.pdf",
                   "https://www.adafruit.com/product/5035"],
    }

if __name__ == "__main__":
    import argparse
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    content=json.dumps(report(),indent=2,allow_nan=False)+"\n"
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(content,encoding="utf-8")
    else:
        print(content,end="")
