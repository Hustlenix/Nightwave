#!/usr/bin/env python3
"""Inventory finished-device evidence, never certify a physical result or Pixl approval."""
from __future__ import annotations

import argparse
import csv
from datetime import datetime
from decimal import Decimal, InvalidOperation
import io
import json
from pathlib import Path
import re
from urllib.parse import urlsplit

MAX_JSON_BYTES = 65536
MAX_TEXT_BYTES = 1024 * 1024
MAX_ARTIFACT_BYTES = 512 * 1024 * 1024
MAX_PATH = 256
MAX_NODES = 1000
RECORD = {".md", ".txt", ".log", ".json", ".csv", ".pdf"}
VISUAL = {".png", ".jpg", ".jpeg", ".mp4", ".webm", ".mov"}
VIDEO = {".mp4", ".webm", ".mov"}
CAD = {".fcstd", ".f3d", ".blend"}


def parsed_number(raw: str) -> Decimal:
    if len(raw) > 32:
        raise ValueError("numeric input is too long")
    try:
        value = Decimal(raw)
    except InvalidOperation as error:
        raise ValueError("invalid numeric input") from error
    if not value.is_finite() or len(value.as_tuple().digits) > 16 or abs(value.adjusted()) > 9:
        raise ValueError("numeric input exceeds finite precision/magnitude limits")
    return value


def pairs(items: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in items:
        if key in result:
            raise ValueError("duplicate JSON key")
        result[key] = value
    return result


def bounded_json(raw: str) -> dict:
    value = json.loads(raw, parse_float=parsed_number, parse_int=parsed_number,
                       parse_constant=lambda _: (_ for _ in ()).throw(ValueError("nonfinite JSON number")),
                       object_pairs_hook=pairs)
    count = 0

    def visit(item: object, depth: int) -> None:
        nonlocal count
        count += 1
        if count > MAX_NODES or depth > 8:
            raise ValueError("JSON exceeds node/depth limits")
        if isinstance(item, dict):
            for key, child in item.items():
                if len(key) > 128:
                    raise ValueError("JSON key is too long")
                visit(child, depth + 1)
        elif isinstance(item, list):
            if len(item) > 64:
                raise ValueError("JSON array exceeds 64 entries")
            for child in item:
                visit(child, depth + 1)
        elif isinstance(item, str) and (len(item) > 2048 or any(ord(c) < 32 for c in item)):
            raise ValueError("JSON string exceeds length/control-character limits")

    visit(value, 0)
    if not isinstance(value, dict) or value.get("schema") != Decimal(1) or isinstance(value.get("schema"), bool):
        raise ValueError("manifest must be a schema 1 object")
    return value


def linked_path(path: Path) -> bool:
    return path.is_symlink() or getattr(path, "is_junction", lambda: False)()


def local_file(root: Path, supplied: object, extensions: set[str], maximum: int = MAX_ARTIFACT_BYTES) -> Path:
    if not isinstance(supplied, str) or not supplied or len(supplied) > MAX_PATH:
        raise ValueError("missing or overlong repository-relative path")
    if "\\" in supplied or ":" in supplied or supplied.startswith("/"):
        raise ValueError("absolute paths, backslashes and drive names are not allowed")
    parts = supplied.split("/")
    if len(parts) > 16 or any(p.startswith(".") or not re.fullmatch(r"[A-Za-z0-9_ -][A-Za-z0-9_. -]{0,127}", p)
                             or p.endswith((" ", ".")) for p in parts):
        raise ValueError("invalid path segment or path escape")
    target = root
    if linked_path(root):
        raise ValueError("repository root cannot be a symlink/junction")
    for part in parts:
        target /= part
        if linked_path(target):
            raise ValueError("symlink/junction artifacts are not allowed")
    if not target.resolve().is_relative_to(root.resolve()):
        raise ValueError("artifact escapes repository")
    if target.suffix.lower() not in extensions or not target.is_file():
        raise ValueError("missing artifact or unsupported file type")
    if not 0 < target.stat().st_size <= maximum:
        raise ValueError("empty or oversized artifact")
    return target


def check_signature(path: Path) -> None:
    with path.open("rb") as source:
        head = source.read(4096)
    suffix = path.suffix.lower()
    signatures = {
        ".png": head.startswith(b"\x89PNG\r\n\x1a\n"),
        ".jpg": head.startswith(b"\xff\xd8\xff"), ".jpeg": head.startswith(b"\xff\xd8\xff"),
        ".pdf": head.startswith(b"%PDF-"),
        ".blend": head.startswith(b"BLENDER"),
        ".fcstd": head.startswith(b"PK\x03\x04"), ".f3d": head.startswith(b"PK\x03\x04"),
        ".step": head.lstrip().startswith(b"ISO-10303-21;"),
        ".stp": head.lstrip().startswith(b"ISO-10303-21;"),
        ".kicad_sch": head.lstrip().startswith(b"(kicad_sch"),
        ".mp4": len(head) >= 12 and head[4:8] == b"ftyp",
        ".mov": len(head) >= 12 and head[4:8] in {b"ftyp", b"moov", b"mdat", b"wide"},
        ".webm": head.startswith(b"\x1a\x45\xdf\xa3"),
    }
    if suffix in signatures and not signatures[suffix]:
        raise ValueError("artifact signature does not match file type")
    if suffix in RECORD - {".pdf"}:
        if b"\x00" in head or not head.strip():
            raise ValueError("record is empty or is not a text record")
        # A prefix can end inside a UTF-8 character. All text is reviewed by a human.
        head.decode("utf-8-sig", errors="strict") if len(head) < 4096 else head[:-4].decode("utf-8-sig", errors="replace")


def valid_demo_url(value: object) -> bool:
    if not isinstance(value, str) or not value or len(value) > 2048 or any(c.isspace() or ord(c) < 32 for c in value) or "\\" in value:
        return False
    try:
        parsed = urlsplit(value)
        hostname = parsed.hostname or ""
        return (parsed.scheme == "https" and parsed.username is None and parsed.password is None
                and parsed.port in {None, 443} and len(hostname) <= 253 and "." in hostname
                and hostname.rsplit(".", 1)[-1].lower() not in {"invalid", "test", "example", "localhost", "local", "localdomain"}
                and all(re.fullmatch(r"[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?", label)
                        for label in hostname.split("."))
                and not hostname.replace(".", "").isdigit())
    except ValueError:
        return False


def audit(root: Path, manifest_path: str = "measurements/trial-evidence.json") -> dict:
    findings: list[str] = []
    files: dict[str, str | list[str]] = {}
    reported_hours = None
    try:
        manifest_file = local_file(root, manifest_path, {".json"}, MAX_JSON_BYTES)
        manifest = bounded_json(manifest_file.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, ValueError, RecursionError) as error:
        findings.append(f"Manifest: {error}")
        manifest = {}

    def section(name: str, parent: dict | None = None) -> dict:
        item = (manifest if parent is None else parent).get(name)
        if not isinstance(item, dict):
            findings.append(f"{name}: missing object")
            return {}
        return item

    def require_true(data: dict, field: str, label: str) -> None:
        if data.get(field) is not True:
            findings.append(f"{label}: builder-reported {field} is not confirmed")

    def require_text(data: dict, field: str, label: str) -> None:
        if not isinstance(data.get(field), str) or not data[field].strip() or len(data[field]) > 512:
            findings.append(f"{label}: missing bounded {field}")

    def numeric(data: dict, field: str, low: Decimal, high: Decimal, label: str) -> Decimal | None:
        value = data.get(field)
        if not isinstance(value, Decimal) or not low <= value <= high:
            findings.append(f"{label}: {field} must be a bounded JSON number ({low}..{high})")
            return None
        return value

    def artifact(data: dict, field: str, kinds: set[str], label: str, maximum: int = MAX_ARTIFACT_BYTES) -> Path | None:
        try:
            path = local_file(root, data.get(field), kinds, maximum)
            check_signature(path)
            files[label] = path.relative_to(root).as_posix()
            return path
        except (OSError, UnicodeError, ValueError) as error:
            findings.append(f"{label}: {error}")
            return None

    identity = section("device_identity")
    commit = identity.get("firmware_commit")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        findings.append("Device: exact 40-character firmware commit is required")
    require_text(identity, "board_revision", "Device")
    own = section("own_files")
    require_true(own, "builder_reported_played", "Own files")
    if not isinstance(own.get("storage"), str) or own["storage"] not in {"microSD", "flash"}:
        findings.append("Own files: storage must be microSD or flash")
    artifact(own, "media_inventory", RECORD, "Own-file inventory", MAX_TEXT_BYTES)
    artifact(own, "playback_log", RECORD, "Own-file playback log", MAX_TEXT_BYTES)
    speaker = section("speaker")
    require_true(speaker, "builder_reported_audible", "Speaker")
    require_true(speaker, "without_phone_app_streaming", "Speaker")
    artifact(speaker, "evidence", VIDEO, "Standalone speaker footage")
    controls = section("controls")
    for field in ("play_pause", "skip", "volume", "without_console_commands"):
        require_true(controls, field, "Physical buttons")
    artifact(controls, "evidence", VIDEO, "Physical button footage")

    runtime = section("runtime")
    require_true(runtime, "builder_reported_measured", "Runtime")
    if runtime.get("output") != "speaker":
        findings.append("Runtime: Nightwave full-night profile requires speaker output")
    hours = numeric(runtime, "duration_hours", Decimal(8), Decimal(168), "Runtime")
    if hours is not None:
        reported_hours = str(hours)
    try:
        timestamps = []
        for field in ("start_utc", "end_utc"):
            value = runtime.get(field)
            if not isinstance(value, str) or not re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z", value):
                raise ValueError("start/end must be actual UTC timestamps YYYY-MM-DDTHH:MM:SSZ")
            timestamps.append(datetime.strptime(value, "%Y-%m-%dT%H:%M:%SZ"))
        elapsed = Decimal(str((timestamps[1] - timestamps[0]).total_seconds())) / Decimal(3600)
        if elapsed <= 0 or hours is None or abs(elapsed - hours) > Decimal("0.001"):
            raise ValueError("runtime duration disagrees with actual start/end timestamps")
    except ValueError as error:
        findings.append(f"Runtime: {error}")
    artifact(runtime, "raw_log", RECORD - {".pdf"}, "Raw runtime log", MAX_TEXT_BYTES)
    artifact(runtime, "external_evidence", RECORD | VISUAL, "External runtime observations")
    conditions = section("conditions", runtime)
    for field in ("battery_mpn", "full_charge_method", "speaker_mpn", "display_backlight_profile", "logger_setup", "cutoff_reason"):
        require_text(conditions, field, "Runtime conditions")
    for field, low, high in (("battery_capacity_mah", 100, 30000), ("speaker_load_ohms", 1, 32),
                             ("volume_percent", 1, 100), ("ambient_c", -10, 60)):
        numeric(conditions, field, Decimal(low), Decimal(high), "Runtime conditions")
    require_true(conditions, "usb_power_or_charging_absent", "Runtime conditions")
    require_true(conditions, "continuous_playback", "Runtime conditions")

    design = section("artifacts")
    artifact(design, "schematic", {".kicad_sch", ".pdf"}, "Final schematic")
    artifact(design, "editable_enclosure", CAD, "Editable enclosure")
    artifact(design, "assembly_step", {".step", ".stp"}, "Assembly STEP")
    photos = design.get("build_photos")
    if not isinstance(photos, list) or not 2 <= len(photos) <= 32:
        findings.append("Build photos: require 2..32 progress/finished-build images")
    else:
        photo_paths = []
        for index, value in enumerate(photos):
            path = artifact({"path": value}, "path", {".png", ".jpg", ".jpeg"}, f"Build photo {index + 1}")
            if path is not None:
                photo_paths.append(path.relative_to(root).as_posix())
        if len(set(photo_paths)) != len(photo_paths):
            findings.append("Build photos: duplicate images do not establish separate progress/finished evidence")
        files["build_photos"] = photo_paths
    demo = section("demo", design)
    if demo.get("path"):
        artifact(demo, "path", VIDEO, "Real demo video")
    elif not valid_demo_url(demo.get("url")):
        findings.append("Demo: require a nonempty local video or valid public HTTPS video URL")
    else:
        files["demo_url"] = demo["url"]  # Presence only; accessibility/content are NOT verified.
    require_true(demo, "builder_reported_real_device", "Demo")
    if design.get("readme") != "README.md":
        findings.append("README: final repository README.md must be referenced")
    readme = artifact(design, "readme", {".md"}, "README", MAX_TEXT_BYTES)
    if readme is not None:
        try:
            text = readme.read_text(encoding="utf-8-sig")
            sections = re.split(r"(?m)^#{1,6}\s+", text)[1:]
            for label, pattern in (("parts list", r"\b(parts|bom)\b"), ("build steps", r"\b(build|assembly)\b")):
                if not any(s.splitlines() and re.search(pattern, s.splitlines()[0], re.I)
                           and len("".join(s.splitlines()[1:]).strip()) >= 40 for s in sections):
                    findings.append(f"README: missing substantive {label} section")
        except (OSError, UnicodeError) as error:
            findings.append(f"README: {error}")
    bom = artifact(design, "parts_list", {".csv"}, "Parts list", MAX_TEXT_BYTES)
    if bom is not None:
        try:
            reader = csv.DictReader(io.StringIO(bom.read_text(encoding="utf-8-sig")))
            if not {"Manufacturer_Part_Number", "Qty", "Supplier_URL"}.issubset(reader.fieldnames or []):
                raise ValueError("parts list must include MPN, quantity and supplier links")
            count = 0
            for row in reader:
                count += 1
                if count > 2000 or None in row or any(v is None for v in row.values()):
                    raise ValueError("parts list exceeds bounded valid row structure")
            if not count:
                raise ValueError("parts list has no component rows")
        except (OSError, UnicodeError, csv.Error, ValueError) as error:
            findings.append(f"Parts list: {error}")
    review = section("human_review")
    require_true(review, "builder_reported_completed", "Independent human review")
    require_text(review, "reviewer", "Independent human review")
    artifact(review, "record", RECORD, "Human review record", MAX_TEXT_BYTES)
    help_record = section("pixl_help")
    require_true(help_record, "builder_reported_confirmed", "Pixl-help clarification")
    artifact(help_record, "record", RECORD, "Pixl-help confirmation record", MAX_TEXT_BYTES)
    return {
        "status": "BLOCKED" if findings else "EVIDENCE_PRESENT_HUMAN_REVIEW_REQUIRED",
        "stage": "finished_device_trial_evidence_inventory", "findings": findings, "files": files,
        "builder_reported_runtime_hours": reported_hours, "physical_pass": False,
        "physical_acceptance": "NOT_ESTABLISHED", "human_review_required": True,
        "attestation_status": "BUILDER_REPORTED_NOT_VERIFIED", "authorship": "NOT_VERIFIED",
        "pixl_acceptance": "NOT_ESTABLISHED", "tier": "NOT_ASSESSED",
        "limits": "Inventory and basic format checks only. Attestations, timestamps, logs, video, CAD contents and public URL accessibility are not independently verified. Dummy files can pass signatures. Human review and actual physical truth remain separate; funding inventory is a different stage.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--manifest", default="measurements/trial-evidence.json")
    parser.add_argument("--output", type=Path, help="Save the JSON inventory report; not physical acceptance")
    args = parser.parse_args()
    report = audit(args.root, args.manifest)
    print(json.dumps(report, indent=2))
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 2 if report["findings"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
