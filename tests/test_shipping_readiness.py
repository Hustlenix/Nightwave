"""Synthetic fixtures test the inventory tool, never hardware acceptance."""
import csv
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "shipping_audit", Path(__file__).resolve().parents[1] / "tools/check_shipping_readiness.py")
shipping = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shipping)


class ShippingReadinessTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "hardware").mkdir()
        self.rows = [
            ["PCB", "TEST_PART", "2", "1.25", "2.50", "https://example.invalid/part", "LOCKED"],
            ["Summary", "TOTAL_COST", "1", "", "2.50", "", ""],
        ]

    def write_bom(self):
        with (self.root / "hardware/BOM.csv").open("w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(["Section", "Manufacturer_Part_Number", "Qty", "Unit_Price_USD",
                             "Extended_Price_USD", "Supplier_URL", "Decision_Status"])
            writer.writerows(self.rows)

    def complete_inventory(self):
        self.write_bom()
        (self.root / "hardware/product-config.json").write_text(json.dumps({
            "display": {"status": "builder_selected"},
            "battery": {"status": "reviewed_power_model", "locked": True},
            "schematic_entry_ready": True,
        }))
        for name in ["hardware/kicad/test.kicad_sch", "hardware/kicad/test.kicad_pcb",
                     "enclosure/test.FCStd", "enclosure/test.step"]:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("SYNTHETIC INVENTORY FIXTURE; NOT A REAL DESIGN")

    def test_missing_repository_blocks(self):
        report = shipping.audit(self.root)
        self.assertEqual(report["status"], "BLOCKED")
        self.assertIsNone(report["bom"]["final_total_usd"])

    def test_present_files_never_claim_approval(self):
        self.complete_inventory()
        report = shipping.audit(self.root)
        self.assertEqual(report["findings"], [])
        self.assertEqual(report["status"], "ARTIFACTS_PRESENT_REVIEW_REQUIRED")
        self.assertTrue(report["human_review_required"])
        self.assertEqual(report["physical_acceptance"], "NOT_ESTABLISHED")
        self.assertEqual(report["tier"], "NOT_ASSESSED")

    def test_priced_subtotal_is_not_final_cost(self):
        self.rows[1][1] = "PRICED_CORE_SUBTOTAL"
        self.write_bom()
        issues = []
        result = shipping.audit_bom(self.root, issues)
        self.assertEqual(result["priced_subtotal_usd"], "2.50")
        self.assertIsNone(result["final_total_usd"])
        self.assertTrue(issues)

    def test_missing_price_not_zero(self):
        self.rows[0][3:5] = ["", ""]
        self.rows[1][4] = "0"
        self.write_bom()
        issues = []
        result = shipping.audit_bom(self.root, issues)
        self.assertEqual(result["priced_rows"], 0)
        self.assertTrue(any("incomplete" in issue for issue in issues))

    def test_bad_costs_and_duplicate_totals(self):
        for bad in ["NaN", "Infinity", "-1", "n.a.", "1e999999999", "1e-999999999", "0e999999999"]:
            with self.subTest(bad=bad):
                self.rows[0][3] = bad
                self.write_bom()
                issues = []
                shipping.audit_bom(self.root, issues)
                self.assertTrue(issues)
        self.rows[0][3] = "1.25"
        self.rows.append(self.rows[1][:])
        self.write_bom()
        issues = []
        shipping.audit_bom(self.root, issues)
        self.assertTrue(any("exactly one" in issue for issue in issues))

    def test_precise_arithmetic_does_not_round_away_mismatch(self):
        self.rows[0][2:5] = ["999999999999", "999999999999.999999999999", "1"]
        self.write_bom()
        issues = []
        shipping.audit_bom(self.root, issues)
        self.assertTrue(any("quantity times" in issue for issue in issues))

    def test_unresolved_selection_and_bad_arithmetic(self):
        self.rows[0][1] = "PENDING_SUPPORT_PARTS"
        self.rows[0][4] = "1.25"
        self.write_bom()
        issues = []
        shipping.audit_bom(self.root, issues)
        self.assertTrue(any("unresolved" in issue for issue in issues))
        self.assertTrue(any("quantity times" in issue for issue in issues))

    def test_empty_sources_and_template_directories_do_not_count(self):
        self.complete_inventory()
        (self.root / "hardware/kicad/test.kicad_pcb").write_text("")
        (self.root / "hardware/kicad/.gitkeep").write_text("placeholder")
        self.assertTrue(any("missing nonempty pcb" in issue for issue in shipping.audit(self.root)["findings"]))

    def test_malformed_and_oversized_input(self):
        self.complete_inventory()
        config = self.root / "hardware/product-config.json"
        config.write_text("[]")
        self.assertTrue(any("configuration" in issue for issue in shipping.audit(self.root)["findings"]))
        config.write_text(" " * (shipping.MAX_BYTES + 1))
        self.assertTrue(any("exceeds" in issue for issue in shipping.audit(self.root)["findings"]))
        (self.root / "hardware/BOM.csv").write_text("wrong,columns\n1,2\n")
        self.assertTrue(any("CSV columns" in issue for issue in shipping.audit(self.root)["findings"]))


if __name__ == "__main__":
    unittest.main()
