import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from ntc_window_screen import feasibility


class NtcWindowTests(unittest.TestCase):
    def test_reset_window_not_qualified(self):
        r=feasibility()
        self.assertFalse(r["necessary_condition_passes"])
        self.assertFalse(r["hardware_qualified"])
        self.assertGreater(r["minimum_pullup_ohm_to_block_hot_without_shunt"],
                           r["maximum_pullup_ohm_to_block_cold_without_shunt"])

    def test_nominal_is_not_tolerance_proof(self):
        self.assertTrue(feasibility(envelope=0)["necessary_condition_passes"])
        self.assertFalse(feasibility()["necessary_condition_passes"])


if __name__=="__main__": unittest.main()
