import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('reference_validator',ROOT/'tools/validate_reference_design.py')
validator=importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)


class ReferenceRegressionTests(unittest.TestCase):
    def setUp(self):
        self.design=json.loads((validator.BASE/'design-manifest.json').read_text(encoding='utf-8'))

    def test_published_reference(self):
        self.assertGreaterEqual(validator.validate(self.design),132)

    def test_reject_internal_supply_drive(self):
        design=copy.deepcopy(self.design)
        next(c for c in design['components'] if c['reference']=='U12')['pin_nets']['25']['net']='+3V3'
        with self.assertRaises(AssertionError):
            validator.validate(design)

    def test_reject_floating_amplifier_ep(self):
        design=copy.deepcopy(self.design)
        next(c for c in design['components'] if c['reference']=='U10')['pin_nets']['17']['net']='NC'
        with self.assertRaises(AssertionError):
            validator.validate(design)

    def test_reject_psram_pin_use(self):
        design=copy.deepcopy(self.design)
        next(c for c in design['components'] if c['reference']=='U1')['pin_nets']['28']['net']='TFT_CS'
        with self.assertRaises(AssertionError):
            validator.validate(design)

    def test_reject_offboard_part_as_connector(self):
        design=copy.deepcopy(self.design)
        next(c for c in design['components'] if c['reference']=='J2')['mpn']='5035'
        with self.assertRaises(AssertionError):
            validator.validate(design)

    def test_reject_missing_bm83_ground_land(self):
        design=copy.deepcopy(self.design)
        next(c for c in design['components'] if c['reference']=='U12')['pin_nets']['56']['net']='NC'
        with self.assertRaises(AssertionError):
            validator.validate(design)
