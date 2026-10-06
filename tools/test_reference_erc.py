"""Native ERC negative controls in isolated copies; never edits design sources.

Requires KiCad 10. A clean report is meaningful only if the checker rejects a
known bad driver connection and a missing power declaration. Not sign-off.
"""
from pathlib import Path
import argparse
import json
import re
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'hardware/kicad/nightwave-reference'


def violations(cli, folder, output):
    subprocess.run(['rtk','proxy',cli,'sch','erc','--format','json','-o',str(output),
                    str(folder/'Nightwave-Reference.kicad_sch')],check=True)
    report=json.loads(output.read_text())
    return [v for sheet in report['sheets'] for v in sheet['violations']]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--cli',default=r'C:\Program Files\KiCad\10.0\bin\kicad-cli.exe')
    args=parser.parse_args()
    scratch=ROOT/'.generated'
    scratch.mkdir(exist_ok=True)
    # Keep evidence rather than recursively deleting generated directories.
    folder=Path(tempfile.mkdtemp(prefix='erc-negative-',dir=scratch))
    for source in SOURCE.glob('*.kicad_*'):
        shutil.copy2(source,folder/source.name)
    shutil.copy2(SOURCE/'sym-lib-table',folder/'sym-lib-table')
    shutil.copy2(SOURCE/'fp-lib-table',folder/'fp-lib-table')
    shutil.copytree(SOURCE/'Nightwave.pretty',folder/'Nightwave.pretty')
    assert not violations(args.cli,folder,folder/'baseline.json'), 'Baseline ERC is not clean'
    path=folder/'02_mcu_storage.kicad_sch'
    text=path.read_text()
    # U1 is the first embedded symbol. GPIO21 is connected to BM83 MCLK output.
    changed,count=re.subn(r'\(pin input line([^\n]*\n\s*\(name "GPIO21")',
                          r'(pin output line\1',text,count=1)
    assert count==1, 'Expected GPIO21 electrical model not found'
    path.write_text(changed)
    bad=violations(args.cli,folder,folder/'driver-conflict.json')
    assert any(v['type']=='pin_to_pin' for v in bad), 'ERC failed to catch two output drivers'
    path.write_text(text)
    root=folder/'Nightwave-Reference.kicad_sch'
    text=root.read_text()
    assert text.count('(global_label "BAT"')==1
    root.write_text(text.replace('(global_label "BAT"','(global_label "UNUSED_NEGATIVE_CONTROL"'))
    bad=violations(args.cli,folder,folder/'missing-supply.json')
    assert any(v['type']=='power_pin_not_driven' for v in bad), 'ERC failed to catch missing supply'
    print(f'Native ERC negative controls passed; isolated evidence in {folder}')


if __name__=='__main__': main()
