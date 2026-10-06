import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from reference_power_budget import report

class ReferencePowerBudgetTests(unittest.TestCase):
    def test_no_false_completion(self):
        r=report()
        self.assertFalse(r["fabrication_approved"])
        self.assertFalse(r["battery_locked"])
        self.assertIsNone(r["measured_runtime_hours"])
        self.assertGreaterEqual(len(r["blocking_findings"]),6)

    def test_correct_reference_and_energy(self):
        r=report(); p=r["profiles"]["speaker_typical_bt_retained"]
        self.assertAlmostEqual(p["bt_output_w"],.18)
        self.assertAlmostEqual(p["cell_equivalent_w"],3.3*.363/.85+.25/.8+.18/.85+.025)
        self.assertGreater(p["typical_capacity_10050mah"]["estimated_runtime_hours"],
                           p["minimum_capacity_9500mah"]["estimated_runtime_hours"])

    def test_charge_and_peak_failures_remain_visible(self):
        r=report()
        timer=r["charger_reset"]
        self.assertEqual(timer["fast_charge_timer_hours"],14.5)
        self.assertEqual(timer["fast_charge_timer_min_hours"],10.5)
        self.assertEqual(timer["fast_charge_timer_max_hours"],15.5)
        self.assertGreater(10050/timer["charge_ma"],timer["fast_charge_timer_max_hours"])
        self.assertGreater(r["profiles"]["transient_not_runtime"]["cell_current_at_3v2_ma"],2000)
        self.assertGreater(r["charging"]["system_off_allowance"]["optimistic_charge_hours"],timer["fast_charge_timer_max_hours"])
        self.assertLess(r["charging"]["speaker_playback"]["optimistic_charge_current_ma"],320)
        self.assertNotIn("linear_charger_heat_w_at_3v7",r["charging"]["speaker_playback"])

if __name__=="__main__": unittest.main()
