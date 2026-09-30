#!/usr/bin/env python3
"""Fast repository consistency checks used locally and in CI."""

from __future__ import annotations

import csv
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
        "hardware/prototype-BOM.csv",
        "hardware/prototype-wiring.csv",
        "tools/generate_test_media.py",
    ]
    missing = [path for path in required if not (ROOT / path).is_file()]
    if missing:
        raise AssertionError(f"Missing required Phase 2 files: {missing}")


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
    validate_required_files()
    print("Nightwave project checks passed")


if __name__ == "__main__":
    main()
