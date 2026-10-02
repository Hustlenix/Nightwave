#!/usr/bin/env python3
"""Inventory funding artifacts; never certify authorship or physical success."""
from __future__ import annotations

import argparse
import csv
import io
import json
from decimal import Decimal, InvalidOperation, localcontext
from pathlib import Path

MAX_BYTES = 1024 * 1024
MAX_ROWS = 2000
FINAL_STATUSES = {"BUILDER_SELECTED", "BUILDER_SELECTED_BD14", "APPROVED", "LOCKED"}
TOTAL_IDS = {"TOTAL", "TOTAL_COST", "GRAND_TOTAL"}


def read_text(path: Path) -> str:
    if not path.is_file() or path.stat().st_size > MAX_BYTES:
        raise ValueError("missing file or file exceeds 1 MiB")
    return path.read_text(encoding="utf-8-sig")


def number(value: str, field: str) -> Decimal:
    try:
        parsed = Decimal(value)
    except InvalidOperation as error:
        raise ValueError(f"invalid {field}") from error
    if not parsed.is_finite() or parsed < 0:
        raise ValueError(f"invalid {field}")
    # Bound arithmetic and report size, including deliberately extreme exponents.
    if len(parsed.as_tuple().digits) > 24 or abs(parsed.adjusted()) > 12:
        raise ValueError(f"{field} exceeds numeric input limits")
    return parsed


def audit_bom(root: Path, findings: list[str]) -> dict:
    result = {"component_rows": 0, "priced_rows": 0,
              "priced_subtotal_usd": "0.00", "final_total_usd": None}
    try:
        reader = csv.DictReader(io.StringIO(read_text(root / "hardware/BOM.csv")))
        required = {"Manufacturer_Part_Number", "Qty", "Unit_Price_USD",
                    "Extended_Price_USD", "Supplier_URL", "Decision_Status", "Section"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("missing required CSV columns")
        rows = []
        for row in reader:
            if len(rows) == MAX_ROWS:
                raise ValueError("BOM exceeds 2000 rows")
            if None in row or any(value is None for value in row.values()):
                raise ValueError("CSV row has the wrong column count")
            rows.append(row)
        if not rows:
            raise ValueError("BOM has no rows")
    except (OSError, UnicodeError, csv.Error, ValueError) as error:
        findings.append(f"BOM: {error}")
        return result

    subtotal = Decimal(0)
    totals = []
    identities = set()
    for row in rows:
        mpn = row["Manufacturer_Part_Number"].strip()
        if row["Section"].strip().casefold() == "summary":
            if mpn.upper() in TOTAL_IDS:
                totals.append(row)
            continue  # A priced-core subtotal is not a final cost.
        result["component_rows"] += 1
        if not mpn or mpn in identities:
            findings.append(f"BOM: missing or duplicate part identity {mpn!r}")
        identities.add(mpn)
        if row["Decision_Status"].strip().upper() not in FINAL_STATUSES or "PENDING" in mpn.upper():
            findings.append(f"BOM: {mpn or 'unnamed row'} selection is unresolved")
        if not row["Supplier_URL"].strip().lower().startswith(("https://", "http://")):
            findings.append(f"BOM: {mpn} has no supplier link")
        try:
            qty = number(row["Qty"], "quantity")
            unit = number(row["Unit_Price_USD"], "unit price")
            extended = number(row["Extended_Price_USD"], "extended price")
            if qty <= 0 or qty != qty.to_integral_value():
                raise ValueError("quantity must be a positive integer")
            with localcontext() as context:
                context.prec = 64
                if unit * qty != extended:
                    raise ValueError("quantity times unit price differs from extended price")
                subtotal += extended
            result["priced_rows"] += 1
        except ValueError as error:
            findings.append(f"BOM: {mpn} {error}")
    result["priced_subtotal_usd"] = str(subtotal)
    if not result["component_rows"]:
        findings.append("BOM: no component rows")
    if len(totals) != 1:
        findings.append("BOM: exactly one final TOTAL_COST summary is required; a core subtotal is insufficient")
    else:
        try:
            total = number(totals[0]["Extended_Price_USD"], "final total")
            result["final_total_usd"] = str(total)
            if total != subtotal or result["priced_rows"] != result["component_rows"]:
                findings.append("BOM: final total is incomplete or does not reconcile")
        except ValueError as error:
            findings.append(f"BOM: {error}")
    return result


def source_files(root: Path, directory: str, extensions: set[str]) -> list[str]:
    base = root / directory
    found = []
    if base.is_dir():
        for path in base.rglob("*"):
            if path.suffix.lower() not in extensions or not path.is_file():
                continue
            # Do not let an external symlink stand in for repository sources.
            if not path.resolve().is_relative_to(root.resolve()) or path.stat().st_size == 0:
                continue
            found.append(path.relative_to(root).as_posix())
    return sorted(found)


def audit(root: Path) -> dict:
    findings = []
    bom = audit_bom(root, findings)
    try:
        config = json.loads(read_text(root / "hardware/product-config.json"))
        if not isinstance(config, dict):
            raise ValueError("configuration must be an object")
        display, battery = config.get("display"), config.get("battery")
        if not isinstance(display, dict) or display.get("status") != "builder_selected":
            findings.append("Design: final display BD-15 is not selected")
        if not isinstance(battery, dict) or battery.get("status") != "reviewed_power_model" or battery.get("locked") is not True:
            findings.append("Design: battery and updated power model are not locked")
        if config.get("schematic_entry_ready") is not True:
            findings.append("Design: combined architecture/pin/power prerequisites are not resolved")
    except (OSError, UnicodeError, ValueError) as error:
        findings.append(f"Design configuration: {error}")
    files = {
        "schematic": source_files(root, "hardware/kicad", {".kicad_sch"}),
        "pcb": source_files(root, "hardware/kicad", {".kicad_pcb"}),
        "editable_enclosure": source_files(root, "enclosure", {".fcstd", ".f3d", ".blend"}),
        "assembly_step": source_files(root, "enclosure", {".step", ".stp"}),
    }
    for kind, paths in files.items():
        if not paths:
            findings.append(f"Artifact: missing nonempty {kind} source")
    return {
        "status": "BLOCKED" if findings else "ARTIFACTS_PRESENT_REVIEW_REQUIRED",
        "stage": "design_funding_inventory", "bom": bom, "files": files,
        "findings": findings, "human_review_required": True,
        "physical_acceptance": "NOT_ESTABLISHED", "tier": "NOT_ASSESSED",
        "limits": "Inventory only. File presence does not prove complete CAD, correct circuits, authorship, review, valid quotes, physical performance or Pixl approval. Onshape-only designs need manual source review.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = audit(args.root)
    text = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    print(text, end="")
    return 2 if report["findings"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
