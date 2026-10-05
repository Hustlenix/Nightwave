"""Reject stale or failing saved CAD evidence. Does not replace KiCad DRC."""
from pathlib import Path
import hashlib
import json

BASE=Path(__file__).resolve().parents[1]/'hardware/kicad/nightwave-reference'


def validate():
    status=json.loads((BASE/'reports/validation.json').read_text(encoding='utf-8'))
    report=json.loads((BASE/'reports/drc.json').read_text(encoding='utf-8'))
    for key in ['violations','unconnected_items','schematic_parity']:
        assert report[key]==[], f'Native {key} findings are unresolved'
        assert status['drc_counts'][key]==0
    for name,digest in status['source_hashes'].items():
        path=(BASE/name).resolve()
        assert path.is_relative_to(BASE.resolve()), 'Invalid evidence source path'
        assert hashlib.sha256(path.read_bytes().replace(b'\r\n',b'\n')).hexdigest()==digest, f'Stale native evidence: {name}'
    required=[
        'reports/Nightwave-Reference-Schematic-Review.pdf','reports/Nightwave-Reference.xml',
        'manufacturing/Nightwave-Reference-PTH.drl',
        'manufacturing/Nightwave-Reference-NPTH.drl',
        'manufacturing/Nightwave-Reference-all-pos.csv',
        'manufacturing/Nightwave-Reference-PCB-Review.step',
        'manufacturing/DO_NOT_FABRICATE.txt',
    ]
    for name in required:assert (BASE/name).stat().st_size>0, f'Missing export: {name}'
    assert len(list((BASE/'manufacturing/gerbers').glob('*')))==12, 'Expected 11 plots and one Gerber job'
    assert status['connectivity_geometry_clean'] is True
    assert status['fabrication_approved'] is False
    assert status['physical_build_verified'] is False
    assert status['measured_runtime_hours'] is None
    print('Reference source hashes, saved clean native reports and export inventory passed; no electrical or physical approval implied')


if __name__=='__main__':validate()
