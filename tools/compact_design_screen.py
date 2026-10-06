"""Builder requirements screening; never produces PCB/CAD or selects a battery."""
import json
from product_power_budget import bounded
from reference_power_budget import report as reference_report


def supported_average_w(capacity_mah, hours=8, usable=.8, aging=.8, margin=1.2):
    capacity_mah = bounded(capacity_mah, "capacity", 100, 30000)
    hours = bounded(hours, "hours", .01, 168)
    usable = bounded(usable, "usable", .1, 1)
    aging = bounded(aging, "aging", .1, 1)
    margin = bounded(margin, "margin", 1, 3)
    return capacity_mah / 1000 * 3.7 * usable * aging / hours / margin


def report():
    reference = reference_report()
    profiles = {
        name: {"estimated_cell_w": p["cell_equivalent_w"],
               "required_capacity_mah": p["minimum_capacity_9500mah"]["required_capacity_mah"]}
        for name, p in reference["profiles"].items()
        if name != "transient_not_runtime"
    }
    return {
        "basis": "ESTIMATED_INPUTS_CALCULATED_OUTPUTS_NOT_MEASURED",
        "pcb_target_mm": [90, 60], "device_target_mm": [110, 70, 25],
        "budget_target_inr": 15000, "reusable_tools_included": False,
        "battery_selected": None, "final_cad_author": "builder",
        "runtime_hours_target": 8, "measured_runtime_hours": None,
        "assumptions": {"nominal_cell_v": 3.7, "usable": .8, "aging": .8, "load_margin": 1.2},
        "reference_load_requirements_not_final_design": profiles,
        "capacity_sensitivity_not_pack_selection": {
            str(c): {"maximum_average_cell_w": supported_average_w(c)}
            for c in (3000, 4000, 5000, 6000, 8000)
        },
        "illustrative_thickness_budget_mm": {
            "outside": 25, "walls_total": 4, "pcb": 1.2,
            "remaining_for_everything_else": 25 - 4 - 1.2,
            "old_5035_pack_thickness": 18.7,
            "remaining_if_old_pack_stacked_over_pcb": 25 - 4 - 1.2 - 18.7,
        },
        "thickness_warning": "2 mm walls and 1.2 mm PCB are assumptions, not chosen CAD. Remaining space must include components, display, insulation, battery allowance, retention and tolerances; not a fit proof.",
        "budget_status": "Unknown landed total; no price or affordability approval",
        "fabrication_approved": False,
    }


if __name__ == "__main__":
    print(json.dumps(report(), indent=2, allow_nan=False))
