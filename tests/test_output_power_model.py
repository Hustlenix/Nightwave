import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from output_power_model import report


class OutputPowerTests(unittest.TestCase):
    def test_exclusive_routes(self):
        r = report()
        for mode, levels in r["modes"].items():
            for p in levels.values():
                i = p["inputs_ESTIMATED"]
                self.assertLessEqual(sum(x > 0 for x in (i["speaker_output_w"], i["headphone_output_w"], i["bt_ma"])), 1)
                self.assertGreater(p["regulator_loss_w"], 0)
                if mode != "bluetooth": self.assertEqual(i["bt_ma"], 0)

    def test_runtime_and_currents(self):
        for levels in report()["modes"].values():
            self.assertGreater(levels["conservative"]["cell_equivalent_w"], levels["typical"]["cell_equivalent_w"])
            for p in levels.values():
                self.assertAlmostEqual(p["runtime_hours_CALCULATED"]["6000"], 2*p["runtime_hours_CALCULATED"]["3000"])
                self.assertAlmostEqual(p["battery_current_ma_CALCULATED"]["3.2"], p["cell_equivalent_w"]*1000/3.2)

    def test_charging_and_no_false_claims(self):
        r=report()
        self.assertIsNone(r["battery_selected"])
        self.assertIsNone(r["classification"]["measured"])
        self.assertFalse(r["hardware_approved"])
        for levels in r["charging_while_playing_CALCULATED"].values():
            for sources in levels.values():
                self.assertIsNone(sources["100"]["optimistic_charge_hours"])
                self.assertGreater(sources["100"]["battery_discharge_deficit_w"], 0)
                self.assertLessEqual(sources["1500"]["optimistic_charge_current_ma"], 960)
                self.assertFalse(sources["1500"]["safe_charge_configuration_established"])


if __name__ == "__main__": unittest.main()
