"""Arithmetic tests only, not real power, charge or runtime results."""
import importlib.util
import math
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location("product_power", Path(__file__).resolve().parents[1] / "tools/product_power_budget.py")
power = importlib.util.module_from_spec(spec)
spec.loader.exec_module(power)


class ProductPowerTests(unittest.TestCase):
    def test_selected_display_and_separate_loss(self):
        v = power.route(speaker_w=.25)
        self.assertEqual(v["main_3v3_ma"], 363)
        self.assertAlmostEqual(v["cell_equivalent_w"], 3.3 * .363 / .85 + .25 / .8 + .025)
        self.assertAlmostEqual(power.route(bt_ma=50)["bt_output_w"], .185)

    def test_no_double_conversion_and_exclusive_audio(self):
        v = power.route(main_eff=1, bt_eff=1)
        self.assertEqual(v["regulator_loss_w"], 0)
        with self.assertRaises(ValueError): power.route(speaker_w=.1, headphone_w=.1)

    def test_capacity_and_load_monotonicity(self):
        a = power.energy(2, 4000)
        self.assertAlmostEqual(power.energy(2, 8000)["estimated_runtime_hours"], a["estimated_runtime_hours"] * 2)
        self.assertLess(power.energy(3, 4000)["estimated_runtime_hours"], a["estimated_runtime_hours"])
        self.assertGreater(power.energy(2, hours=10)["required_capacity_mah"], a["required_capacity_mah"])

    def test_invalid_inputs_rejected(self):
        for value in (float("nan"), float("inf"), -1, True, "200", 1e99):
            with self.subTest(value=value), self.assertRaises(ValueError): power.route(mcu_ma=value)
        for value in (0, -.1, 1.01, math.inf):
            with self.assertRaises(ValueError): power.energy(1, usable=value)

    def test_charge_is_conditional_and_starved(self):
        a = power.charge_screen(6600, 1000, 100, 2)
        self.assertIsNone(a["optimistic_charge_hours"])
        self.assertFalse(a["safe_charge_configuration_established"])
        b = power.charge_screen(6600, 1000, 1500, 2)
        self.assertAlmostEqual(b["optimistic_charge_hours"], 8.58)
        self.assertGreater(power.charge_screen(8000, 1000, 1500, 2)["optimistic_charge_hours"], b["optimistic_charge_hours"])
        self.assertAlmostEqual(b["linear_charger_heat_w_at_3v7"], 1.3)

    def test_report_never_locks_or_measures(self):
        r = power.report()
        self.assertFalse(r["battery_locked"])
        self.assertIsNone(r["measured_runtime_hours"])
        self.assertIn("TCA9535PWR", r["hardware"])
        self.assertNotIn("eight_hour_requirement", r["profiles"]["transient_capacity_screen_not_runtime"])
        self.assertGreater(r["profiles"]["speaker_worst_practical_screening"]["cell_equivalent_w"], r["profiles"]["speaker_typical_bt_off_assumed"]["cell_equivalent_w"])


if __name__ == "__main__": unittest.main()
