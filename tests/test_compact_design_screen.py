import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from compact_design_screen import report, supported_average_w
from product_power_budget import energy


class CompactScreenTests(unittest.TestCase):
    def test_constraints_not_approval(self):
        r = report()
        self.assertEqual(r["pcb_target_mm"], [90, 60])
        self.assertEqual(r["device_target_mm"], [110, 70, 25])
        self.assertEqual(r["budget_target_inr"], 15000)
        self.assertEqual(r["final_cad_author"], "builder")
        self.assertIsNone(r["battery_selected"])
        self.assertIsNone(r["measured_runtime_hours"])
        self.assertFalse(r["fabrication_approved"])

    def test_inverse_energy_calculation(self):
        for capacity in (3000, 4000, 5000, 6000, 8000):
            power = supported_average_w(capacity)
            self.assertAlmostEqual(energy(power, capacity)["estimated_runtime_hours"], 8)
            self.assertAlmostEqual(energy(power, capacity)["required_capacity_mah"], capacity)

    def test_invalid_inputs(self):
        for value in (True, float("nan"), float("inf"), -1, 0, "5000"):
            with self.assertRaises(ValueError):
                supported_average_w(value)
        with self.assertRaises(ValueError):
            supported_average_w(5000, hours=0)

    def test_thickness_and_transient_not_runtime(self):
        r = report()
        self.assertAlmostEqual(r["illustrative_thickness_budget_mm"]["remaining_if_old_pack_stacked_over_pcb"], 1.1)
        self.assertNotIn("transient_not_runtime", r["reference_load_requirements_not_final_design"])
        self.assertGreater(r["reference_load_requirements_not_final_design"]["speaker_typical_bt_retained"]["required_capacity_mah"], 7900)


if __name__ == "__main__":
    unittest.main()
