import math
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"tools"))
from independent_thermal_window import screen


class IndependentWindowTests(unittest.TestCase):
    def test_estimated_corners_have_positive_margins(self):
        result = screen()
        self.assertTrue(result["sensitivity_passes"])
        self.assertFalse(result["hardware_qualified"])
        self.assertTrue(all(v > 0 for v in result["normalized_margins"].values()))

    def test_large_uncertainty_is_not_hidden(self):
        self.assertFalse(screen(rt_error=.30)["sensitivity_passes"])
        self.assertFalse(screen(error_v=.3)["sensitivity_passes"])

    def test_worse_supply_reduces_margin(self):
        low, high = screen(supply_v=3), screen(supply_v=3.6)
        for key in low["normalized_margins"]:
            self.assertLess(low["normalized_margins"][key],
                            high["normalized_margins"][key])

    def test_invalid_assumptions(self):
        for kwargs in ({"rt_error": -1}, {"rt_error": 1},
                       {"resistor_error": 1}, {"error_v": -1},
                       {"supply_v": 0}, {"supply_v": math.nan},
                       {"error_v": math.inf}):
            with self.assertRaises(ValueError):
                screen(**kwargs)


if __name__ == "__main__":
    unittest.main()
