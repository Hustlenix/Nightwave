#!/usr/bin/env python3
"""Fast repository consistency checks used locally and in CI."""

from __future__ import annotations

import csv
import json
import re
from decimal import Decimal
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read_csv(relative_path: str) -> list[dict[str, str]]:
    with (ROOT / relative_path).open(newline="", encoding="utf-8-sig") as source:
        rows = list(csv.DictReader(source))
    if not rows:
        raise AssertionError(f"{relative_path} is empty")
    return rows


def validate_subtotal(
    relative_path: str,
    part_column: str,
    summary_part: str,
    extended_column: str,
) -> None:
    rows = read_csv(relative_path)
    summary = [row for row in rows if row[part_column] == summary_part]
    if len(summary) != 1:
        raise AssertionError(f"{relative_path}: expected one {summary_part} row")
    captured = sum(
        Decimal(row[extended_column])
        for row in rows
        if row[part_column] != summary_part and row[extended_column]
    )
    declared = Decimal(summary[0][extended_column])
    if captured != declared:
        raise AssertionError(
            f"{relative_path}: captured subtotal {captured} does not match {declared}"
        )


def validate_wiring_matches_firmware() -> None:
    wiring = read_csv("hardware/prototype-wiring.csv")
    header = (ROOT / "firmware/components/app_state/include/nightwave/hardware_config.h").read_text(
        encoding="utf-8"
    )
    constants = {
        name: int(value)
        for name, value in re.findall(
            r"inline constexpr std::int8_t (k[A-Za-z0-9]+) = (\d+);", header
        )
    }
    expected = {
        "SD_CLK": "kSdClk",
        "SD_CMD": "kSdCmd",
        "SD_D0": "kSdD0",
        "I2S_BCLK_DAC": "kI2sBitClock",
        "I2S_LRCLK_DAC": "kI2sWordSelect",
        "I2S_DOUT_DAC": "kI2sDataOut",
        "I2C_SDA": "kI2cSda",
        "I2C_SCL": "kI2cScl",
        "BTN_PREV": "kButtonPrevious",
        "BTN_PLAY": "kButtonPlayPause",
        "BTN_NEXT": "kButtonNext",
        "BTN_VOL_DOWN": "kButtonVolumeDown",
        "BTN_VOL_UP": "kButtonVolumeUp",
        "SPK_ENABLE": "kSpeakerEnable",
        "DAC_MUTE": "kHeadphoneEnable",
    }
    rows = {row["Signal"]: row for row in wiring}
    for signal, constant in expected.items():
        actual_pin = rows[signal]["DevKit_Pin"]
        expected_pin = f"GPIO{constants[constant]}"
        if actual_pin != expected_pin:
            raise AssertionError(
                f"{signal}: wiring has {actual_pin}, firmware {constant} has {expected_pin}"
            )


def validate_required_files() -> None:
    required = [
        "docs/prototype-wiring.md",
        "docs/prototype-bringup.md",
        "docs/test-tracks.md",
        "docs/headphone-prototype-options.md",
        "docs/audio-buffer-analysis.md",
        "docs/mp3-integration-plan.md",
        "docs/phase3-firmware-bringup.md",
        "hardware/prototype-BOM.csv",
        "hardware/prototype-wiring.csv",
        "measurements/audio-prototype.csv",
        "measurements/current-draw.csv",
        "tools/generate_test_media.py",
        "docs/builder-decisions.md",
        "docs/builder-tasks.md",
        "docs/schematic-requirements.md",
        "docs/power-options.md",
        "docs/layout-mechanical-requirements.md",
        "docs/component-research-followup.md",
        "tools/engineering_calculations.py",
        "docs/bluetooth-architecture.md",
        "docs/display-selection.md",
        "docs/engineering-budgets.md",
        "docs/expanded-software-checkpoint.md",
        "docs/expanded-bench-gate.md",
        "docs/library-index.md",
        "tools/analyze_diagnostics.py",
        "hardware/product-config.json",
        "docs/mp3-seek-index.md",
        "docs/current-power-model.md",
        "tools/product_power_budget.py",
        "tests/test_product_power_budget.py",
        "docs/input-expansion.md",
        "tools/check_trial_readiness.py",
        "tests/test_trial_readiness.py",
        "measurements/trial-evidence.json",
    ]
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        raise AssertionError(f"Missing required Phase 2 files: {missing}")


def validate_current_bom() -> None:
    rows = read_csv("hardware/BOM.csv")
    required = ("Package", "Datasheet_URL", "Footprint_Information", "Power_Role", "Interface_Role", "Decision_Status")
    for row in rows:
        for field in required:
            if not row.get(field):
                raise AssertionError(f"BOM {row['Manufacturer_Part_Number']}: missing {field}")
    parts = {row["Manufacturer_Part_Number"] for row in rows}
    if "SP12864-13" in parts or "353" in parts or "BM83SM1-00AA" in parts:
        raise AssertionError("Obsolete final core hardware remains in current BOM")
    config = json.loads((ROOT / "hardware/product-config.json").read_text(encoding="utf-8"))
    if config["bluetooth"]["mpn"] not in parts or config["bluetooth"]["role"] != "A2DP_source":
        raise AssertionError("Selected Bluetooth source missing from BOM")
    if config["display"]["status"] == "pending_builder_selection" and "PENDING_DISPLAY_BD15" not in parts:
        raise AssertionError("Missing explicit pending display BOM row")
    if config["display"]["status"] == "builder_selected" and config["display"].get("mpn") not in parts:
        raise AssertionError("Selected display missing from BOM")
    if config["input_expansion"]["status"] == "builder_selected" and config["input_expansion"].get("mpn") not in parts:
        raise AssertionError("Selected input expander missing from BOM")
    header = (ROOT / "firmware/components/app_state/include/nightwave/hardware_config.h").read_text(encoding="utf-8")
    if config["display"].get("mpn") != "24382" or "kFinalDisplaySku = 24382;" not in header:
        raise AssertionError("Selected display JSON/firmware identity mismatch")
    if config["input_expansion"].get("mpn") != "TCA9535PWR" or "kTca9535InputExpansionSelected = true;" not in header:
        raise AssertionError("Selected input expander JSON/firmware identity mismatch")
    for relative in (config["power_model"]["tool"], config["power_model"]["document"]):
        if not (ROOT / relative).is_file():
            raise AssertionError("Missing selected-product power screening")
    unresolved_display = config["display"]["status"] != "builder_selected"
    unresolved_power = config["battery"]["status"] != "reviewed_power_model"
    if not config["combined_pins_reviewed"] and config["schematic_entry_ready"]:
        raise AssertionError("Unreviewed combined pin map cannot start schematic")
    if (unresolved_display or unresolved_power) and (config["battery"]["locked"] or config["schematic_entry_ready"]):
        raise AssertionError("Current pending display/power review cannot lock battery or start schematic")


def validate_builder_task_packet() -> None:
    """Check handoff structure, not authorship or engineering correctness."""
    packet = (ROOT / "docs/builder-tasks.md").read_text(encoding="utf-8")
    tasks = re.split(r"^## BUILDER TASK .*?$", packet, flags=re.MULTILINE)[1:]
    if len(tasks) < 17:
        raise AssertionError("Missing electrical/PCB/CAD builder tasks")
    headings = (
        "Objective", "What I need to create", "Datasheets I need",
        "Engineering decisions I must make", "Constraints", "Checklist",
        "What files/screenshots to return",
    )
    for index, task in enumerate(tasks, 1):
        for heading in headings:
            if f"### {heading}\n" not in task:
                raise AssertionError(f"Builder task {index}: missing {heading}")


def main() -> None:
    validate_subtotal(
        "hardware/BOM.csv",
        "Manufacturer_Part_Number",
        "PRICED_CORE_SUBTOTAL",
        "Extended_Price_USD",
    )
    validate_subtotal(
        "hardware/prototype-BOM.csv",
        "Manufacturer_Part_Number",
        "KNOWN_PRICED_SUBTOTAL",
        "Extended_Price_USD",
    )
    validate_wiring_matches_firmware()
    validate_current_bom()
    validate_required_files()
    validate_builder_task_packet()
    from engineering_calculations import self_test
    self_test()
    from analyze_diagnostics import self_test as diagnostics_self_test
    diagnostics_self_test()
    print("Nightwave project checks passed")


if __name__ == "__main__":
    main()
