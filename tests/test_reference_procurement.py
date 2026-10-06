import csv
import json
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
from build_reference_procurement import build

class ProcurementTests(unittest.TestCase):
    def setUp(self):
        self.choices=json.loads((ROOT/"hardware/reference-resistor-selections.json").read_text())
        with (ROOT/"hardware/kicad/nightwave-reference/reference-bom.csv").open(newline="",encoding="utf-8") as f:
            self.rows=list(csv.DictReader(f))
    def test_value_matching_and_no_invented_quotes(self):
        result=build(self.rows,self.choices)
        self.assertEqual(len(result),len(self.rows))
        self.assertEqual(sum(r["Status"]=="MPN_SELECTED_APPLICATION_REVIEW_REQUIRED" for r in result),49)
        for old,new in zip(self.rows,result):
            self.assertEqual(old["Unit_Price_USD"],new["Unit_Price_USD"])
            self.assertEqual(old["Value"],new["Value"])
            self.assertEqual(old["Footprint"],new["Footprint"])
        self.assertTrue(all(r["MPN"]=="PENDING_SELECTION" for r in result if r["Reference"] in ("R23","R30")))
    def test_footprint_mismatch_fails(self):
        row=next(r for r in self.rows if r["Reference"]=="R1")
        row["Footprint"]="Resistor_SMD:R_0402_1005Metric"
        with self.assertRaises(ValueError): build(self.rows,self.choices)
    def test_existing_selection_not_overwritten(self):
        row=next(r for r in self.rows if r["Reference"]=="R1")
        row["MPN"]="OTHER_SELECTED_PART"
        with self.assertRaises(ValueError): build(self.rows,self.choices)
    def test_committed_inventory_current(self):
        with (ROOT/"hardware/reference-procurement.csv").open(newline="",encoding="utf-8") as f:
            self.assertEqual(list(csv.DictReader(f)),build(self.rows,self.choices))

if __name__=="__main__": unittest.main()
