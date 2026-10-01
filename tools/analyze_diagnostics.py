"""Read Nightwave JSONL logs without turning missing/host data into bench passes."""
from __future__ import annotations
import argparse
import json
from pathlib import Path

TYPES = {"nightwave_boot", "nightwave_stream", "nightwave_performance", "nightwave_selftest"}


def analyze(lines):
    records = []
    for number, line in enumerate(lines, 1):
        if len(line) > 16384:
            raise ValueError(f"line {number}: exceeds 16 KiB")
        # Serial prompts/log prefixes are outside the JSON record.
        opening = line.find("{")
        if opening < 0:
            continue
        try:
            record = json.loads(line[opening:])
        except json.JSONDecodeError:
            continue
        if not isinstance(record, dict) or record.get("type") not in TYPES:
            continue
        if record.get("schema") != 1:
            raise ValueError(f"line {number}: unsupported schema")
        for key, value in record.items():
            if key.endswith(("_bytes", "_us", "_ms", "_frames")) or key in {"errors", "underruns", "sd_reads", "decode_calls"}:
                if value is not None and (type(value) is not int or value < 0):
                    raise ValueError(f"line {number}: invalid {key}")
        records.append(record)
        if len(records) > 100000:
            raise ValueError("record limit exceeded")
    performance = [r for r in records if r["type"] == "nightwave_performance"]
    summary = {"records": len(records), "physical_acceptance": "NOT_ESTABLISHED", "averages": []}
    for r in performance:
        sd_time = r.get("sd_total_us", 0)
        calls = r.get("decode_calls", 0)
        reads = r.get("sd_reads", 0)
        summary["averages"].append({
            "sd_average_us": sd_time / reads if reads else None,
            "sd_bytes_per_second": r.get("sd_bytes", 0) * 1_000_000 / sd_time if sd_time else None,
            "instrumented_decode_average_us": r.get("decode_total_us", 0) / calls if calls else None,
            "counters_saturated": any(r.get(k) == 0xFFFFFFFF for k in ("sd_total_us", "sd_bytes", "decode_total_us")),
        })
    return summary


def self_test():
    assert analyze(["not JSON"]) == {"records": 0, "physical_acceptance": "NOT_ESTABLISHED", "averages": []}
    sample = {"type": "nightwave_performance", "schema": 1, "sd_reads": 2,
              "sd_bytes": 8192, "sd_total_us": 4000, "decode_calls": 2, "decode_total_us": 1000}
    result = analyze([json.dumps(sample)])
    assert result["averages"][0]["sd_average_us"] == 2000
    assert result["averages"][0]["sd_bytes_per_second"] == 2048000
    for invalid in (-1, True, "100"):
        sample["sd_reads"] = invalid
        try:
            analyze([json.dumps(sample)])
        except ValueError:
            pass
        else:
            raise AssertionError("invalid numeric counter accepted")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("Diagnostic parser tests passed (synthetic records, not board evidence)")
    elif args.log:
        with args.log.open(encoding="utf-8", errors="replace") as source:
            print(json.dumps(analyze(source), indent=2))
    else:
        parser.error("provide a log or --self-test")


if __name__ == "__main__":
    main()
