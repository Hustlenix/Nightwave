# Reproducing this checkpoint

Open `Nightwave-Reference.kicad_pro` in KiCad 10. The shipped native PCB is the
authoritative routed artifact. Do not regenerate it as a substitute for editing
the layout. `tools/generate_nightwave_reference.py` now refuses to overwrite an
existing board unless `--rebuild-board` is explicitly supplied; that option
**discards routing**. `--schematic-only` preserves the PCB but changes sources
and invalidates previous evidence until checks and exports are regenerated.

Run with KiCad's Python (which provides `pcbnew`):

```text
rtk proxy "C:\Program Files\KiCad\10.0\bin\python.exe" tools/package_nightwave_reference.py
```

This regenerates native ERC, DRC, schematic parity, the review schematic PDF,
Gerbers, separate plated/non-plated drills, placement CSV, board STEP and two
renders. Archive creation rejects non-zero DRC/parity/connectivity findings and
records hashes of the checked native sources. Text-source hashes normalize
CRLF to LF so Windows and Linux Git checkouts verify consistently.

The final PDF is `reports/Nightwave-Reference-Schematic-Review.pdf`. Older
exports are historical; the current native reports are `reports/drc.json`,
`reports/drc.rpt`, `reports/erc.rpt` and `reports/validation.json`.

Portable checks, also in CI:

```text
python tools/validate_project.py
python tools/validate_reference_design.py
python tools/validate_reference_package.py
python -m unittest discover -s tests -p "test_*.py"
```

The portable package check verifies saved native evidence and source freshness;
it does not run KiCad itself. The semantic checks catch selected known pin-map
regressions, not arbitrary circuit errors. Passive block-symbol ERC cannot
certify power/driver conflicts. Read `VALIDATION_STATUS.md`,
`PROCUREMENT_GAPS.md` and `REVIEW_CHECKLIST.md` before using any export.
