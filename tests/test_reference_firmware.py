import importlib.util
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('board_validator',ROOT/'tools/validate_reference_firmware.py')
validator=importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)


class FirmwareMapTests(unittest.TestCase):
    def setUp(self):
        self.header=(ROOT/'firmware/components/app_state/include/nightwave/reference_board.h').read_text()
        self.manifest=json.loads((ROOT/'hardware/kicad/nightwave-reference/design-manifest.json').read_text())

    def test_profile_matches_schematic(self):
        self.assertEqual(validator.validate(self.header,self.manifest),25)

    def test_legacy_headphone_pin_rejected(self):
        with self.assertRaises(AssertionError):
            validator.validate(self.header.replace('kHeadphoneEnable = 39','kHeadphoneEnable = 18'),self.manifest)

    def test_swapped_uart_rejected(self):
        with self.assertRaises(AssertionError):
            validator.validate(self.header.replace('kBtRx = 2','kBtRx = 40'),self.manifest)

    def test_wrong_schematic_net_rejected(self):
        next(c for c in self.manifest['components'] if c['reference']=='U1')['pin_nets']['11']['net']='HP_EN'
        with self.assertRaises(AssertionError):
            validator.validate(self.header,self.manifest)

    def test_wrong_button_map_rejected(self):
        with self.assertRaises(AssertionError):
            validator.validate(self.header.replace('kButtonBits{0, 1, 2, 3, 4}','kButtonBits{4, 3, 2, 1, 0}'),self.manifest)
