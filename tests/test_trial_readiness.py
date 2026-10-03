"""Synthetic inventory fixtures; none establish physical construction or runtime."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("trial_audit", REPO / "tools/check_trial_readiness.py")
trial = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trial)


class TrialReadinessTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.manifest = json.loads((REPO / "measurements/trial-evidence.json").read_text())

    def file(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content if isinstance(content, bytes) else content.encode())
        return name

    def write(self):
        self.file("measurements/trial-evidence.json", json.dumps(self.manifest))

    def complete(self):
        m = self.manifest
        record = self.file("evidence/synthetic.txt", "SYNTHETIC RECORD FOR SOFTWARE TESTS; NEVER A REAL MEASUREMENT")
        video = self.file("evidence/synthetic.mp4", b"\x00\x00\x00\x18ftypisomSYNTHETIC NOT REAL FOOTAGE")
        m["device_identity"] = {"firmware_commit": "a" * 40, "board_revision": "SYNTHETIC BOARD"}
        m["own_files"] = {"builder_reported_played": True, "storage": "microSD", "media_inventory": record, "playback_log": record}
        m["speaker"] = {"builder_reported_audible": True, "without_phone_app_streaming": True, "evidence": video}
        m["controls"] = {"play_pause": True, "skip": True, "volume": True, "without_console_commands": True, "evidence": video}
        m["runtime"] = {
            "builder_reported_measured": True, "output": "speaker", "duration_hours": 8,
            "start_utc": "2026-10-01T00:00:00Z", "end_utc": "2026-10-01T08:00:00Z",
            "raw_log": record, "external_evidence": record,
            "conditions": {"battery_mpn": "SYNTHETIC", "battery_capacity_mah": 6600,
                "full_charge_method": "SYNTHETIC", "speaker_mpn": "SYNTHETIC", "speaker_load_ohms": 8,
                "volume_percent": 20, "display_backlight_profile": "SYNTHETIC",
                "ambient_c": 25, "logger_setup": "SYNTHETIC", "cutoff_reason": "SYNTHETIC",
                "usb_power_or_charging_absent": True, "continuous_playback": True}}
        m["artifacts"] = {
            "schematic": self.file("hardware/kicad/synthetic.kicad_sch", "(kicad_sch SYNTHETIC NOT A REAL DESIGN)"),
            "editable_enclosure": self.file("enclosure/synthetic.blend", "BLENDER SYNTHETIC NOT A MODEL"),
            "assembly_step": self.file("enclosure/synthetic.step", "ISO-10303-21; SYNTHETIC NOT CAD"),
            "build_photos": [self.file("evidence/progress.png", b"\x89PNG\r\n\x1a\nSYNTHETIC"),
                             self.file("evidence/finished.jpg", b"\xff\xd8\xffSYNTHETIC")],
            "demo": {"path": video, "url": None, "builder_reported_real_device": True},
            "readme": self.file("README.md", "# SYNTHETIC\n## Parts list\n" + "Synthetic description. " * 4 + "\n## Build steps\n" + "Synthetic instructions. " * 4),
            "parts_list": self.file("hardware/BOM.csv", "Manufacturer_Part_Number,Qty,Supplier_URL\nSYNTHETIC,1,https://example.invalid/part\n")}
        m["human_review"] = {"builder_reported_completed": True, "reviewer": "SYNTHETIC REVIEWER", "record": record}
        m["pixl_help"] = {"builder_reported_confirmed": True, "record": record}
        self.write()

    def assert_blocked(self):
        self.write()
        result = trial.audit(self.root)
        self.assertEqual(result["status"], "BLOCKED")
        self.assertTrue(result["findings"])
        self.assertFalse(result["physical_pass"])
        return result

    def test_repository_manifest_is_blocked(self):
        result = trial.audit(REPO)
        self.assertEqual(result["status"], "BLOCKED")
        self.assertEqual(result["physical_acceptance"], "NOT_ESTABLISHED")

    def test_synthetic_complete_always_requires_human_review(self):
        self.complete()
        result = trial.audit(self.root)
        self.assertEqual(result["findings"], [])
        self.assertEqual(result["status"], "EVIDENCE_PRESENT_HUMAN_REVIEW_REQUIRED")
        self.assertFalse(result["physical_pass"])
        self.assertTrue(result["human_review_required"])
        self.assertEqual(result["attestation_status"], "BUILDER_REPORTED_NOT_VERIFIED")
        self.assertEqual(result["pixl_acceptance"], "NOT_ESTABLISHED")

    def test_core_false_or_truthy_string_never_counts(self):
        self.complete()
        baseline = copy.deepcopy(self.manifest)
        for section, field in (("own_files", "builder_reported_played"), ("speaker", "without_phone_app_streaming"),
                               ("controls", "volume"), ("human_review", "builder_reported_completed"), ("pixl_help", "builder_reported_confirmed")):
            for value in (False, "true", 1, None):
                with self.subTest(section=section, value=value):
                    self.manifest = copy.deepcopy(baseline)
                    self.manifest[section][field] = value
                    self.assert_blocked()

    def test_insufficient_or_inconsistent_runtime(self):
        self.complete()
        for value in (7.99, "8", True, -1, 169):
            with self.subTest(value=value):
                self.manifest["runtime"]["duration_hours"] = value
                self.assert_blocked()
        self.manifest["runtime"]["duration_hours"] = 8
        self.manifest["runtime"]["end_utc"] = "2026-10-01T07:00:00Z"
        self.assert_blocked()

    def test_invalid_numeric_json(self):
        self.complete()
        for raw in ("NaN", "Infinity", "1e99999", "1e-99999", "0e99999", "123456789012345678901234567890123456789"):
            self.file("measurements/trial-evidence.json", '{"schema":1,"bad":' + raw + '}')
            self.assertEqual(trial.audit(self.root)["status"], "BLOCKED")

    def test_unsafe_paths_and_type_signatures(self):
        self.complete()
        for path in ("../outside.mp4", "/tmp/test.mp4", "C:/test.mp4", "evidence\\synthetic.mp4", "evidence/./synthetic.mp4", ".git/config", "evidence/synthetic.txt"):
            with self.subTest(path=path):
                self.manifest["speaker"]["evidence"] = path
                self.assert_blocked()
        self.manifest["speaker"]["evidence"] = self.file("evidence/fake.mp4", "not a video")
        self.assert_blocked()

    def test_symlinks_do_not_count(self):
        self.complete()
        link = self.root / "evidence/link.mp4"
        try:
            link.symlink_to(self.root / "evidence/synthetic.mp4")
        except (OSError, NotImplementedError):
            self.skipTest("host cannot create symlinks; exercised on Linux CI")
        self.manifest["speaker"]["evidence"] = "evidence/link.mp4"
        self.assert_blocked()

    def test_demo_url_validation_and_no_claim_of_fetch(self):
        self.complete()
        demo = self.manifest["artifacts"]["demo"]
        demo["path"] = None
        for url in ("javascript:alert(1)", "http://example.com/video", "https://user:secret@example.com/v", "https://example.com:bad/v", "https://localhost/v", "https://127.0.0.1/v", "https://exa mple.com/v", "https://example.com\\evil/v", "https://example.invalid/v", "https://device.local/v"):
            with self.subTest(url=url):
                demo["url"] = url
                self.assert_blocked()
        demo["url"] = "https://example.com/synthetic-video"
        self.write()
        self.assertEqual(trial.audit(self.root)["status"], "EVIDENCE_PRESENT_HUMAN_REVIEW_REQUIRED")

    def test_missing_artifact_and_readme_sections(self):
        self.complete()
        (self.root / "enclosure/synthetic.step").write_text("")
        self.assert_blocked()
        self.file("enclosure/synthetic.step", "ISO-10303-21; SYNTHETIC")
        self.file("README.md", "# Synthetic concept only")
        self.assert_blocked()

    def test_corrupt_huge_deep_duplicate_and_array_json(self):
        for text in ("{", "[]", " " * (trial.MAX_JSON_BYTES + 1), '{"schema":1,"schema":1}',
                     '{"schema":1,"bad":' + "[" * 20 + "0" + "]" * 20 + "}"):
            self.file("measurements/trial-evidence.json", text)
            self.assertEqual(trial.audit(self.root)["status"], "BLOCKED")

    def test_bad_typed_fields_and_late_readme_encoding_do_not_crash(self):
        self.complete()
        self.manifest["own_files"]["storage"] = []
        self.assert_blocked()
        self.manifest["own_files"]["storage"] = "microSD"
        self.file("README.md", b"A" * 5000 + b"\xff")
        self.assert_blocked()

    def test_directory_symlinks_and_manifest_escapes_do_not_count(self):
        self.complete()
        self.assertEqual(trial.audit(self.root, "../trial-evidence.json")["status"], "BLOCKED")
        try:
            (self.root / "linked").symlink_to(self.root / "evidence", target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("host cannot create symlinks; exercised on Linux CI")
        self.manifest["speaker"]["evidence"] = "linked/synthetic.mp4"
        self.assert_blocked()


if __name__ == "__main__":
    unittest.main()
