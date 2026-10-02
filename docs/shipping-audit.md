# Shipping artifact inventory

Run `python3 tools/check_shipping_readiness.py` before preparing a funding
submission. Use `--output .generated/shipping-inventory.json` to save the report.
Exit 2 means missing or unresolved inputs; exit 0 only means the checked artifacts
are present and still need human review. It never awards T4, certifies authorship,
establishes physical acceptance or approves a Pixl submission.

Unlike the development consistency validator, this tool rejects pending/candidate
BOM selections, missing prices, nonfinite/negative amounts, bad quantity-price
arithmetic, duplicate identities and a core subtotal masquerading as a final cost.
It also checks display/power prerequisites and nonempty schematic/board/editable
enclosure/STEP files. Empty directories and `.gitkeep` files do not count.
Numeric input is limited to 24 significant digits and a base-10 magnitude from
1e-12 through values below 1e13 (plus zero), so extreme exponents cannot overflow
arithmetic or create oversized reports. Arithmetic uses 64-digit precision.

File contents still require expert review: dummy files can satisfy an inventory,
and file presence does not prove correct circuits or a full electronics assembly.
Onshape sources and supplier carts require manual verification. Real shipping,
tax and fabrication costs, actual builder design work, independent review and
the later physical audio/runtime tests are separate requirements.

The test suite uses explicitly synthetic fixtures to check incomplete-price,
placeholder, invalid-input and subtotal handling. These are software tests, not
human build sessions or hardware test results.
