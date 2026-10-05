"""Export and truthfully package a headless KiCad review checkpoint.

Never marks a design fabrication approved or supplies physical build evidence.
Run after routing/zone filling, using KiCad Python.
"""
from pathlib import Path
import subprocess
import json
import hashlib
import zipfile
import argparse
from datetime import datetime,timezone
import pcbnew

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'hardware/kicad/nightwave-reference'
NAME='Nightwave-Reference'
CLI=r'C:\Program Files\KiCad\10.0\bin\kicad-cli.exe'


def run(*args):
    subprocess.run(['rtk','proxy',CLI,*map(str,args)],check=True,cwd=ROOT)


def source_hashes():
    paths=[*BASE.glob('*.kicad_sch'),*BASE.glob('*.kicad_pcb'),*BASE.glob('*.kicad_pro'),*BASE.glob('*.kicad_sym'),*(BASE/'Nightwave.pretty').glob('*.kicad_mod')]
    return {str(p.relative_to(BASE)).replace('\\','/'):hashlib.sha256(p.read_bytes().replace(b'\r\n',b'\n')).hexdigest() for p in sorted(paths)}


def archive_package():
    archive=ROOT/'hardware/kicad/Nightwave-AI-Reference-2026-10-05.zip'
    validation=json.loads((BASE/'reports/validation.json').read_text(encoding='utf-8'))
    if validation.get('source_hashes')!=source_hashes():
        raise RuntimeError('Source files changed after export; regenerate checks and exports')
    if not validation['connectivity_geometry_clean']:
        raise RuntimeError('Cannot release archive with native DRC/parity/connectivity findings')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for path in sorted(BASE.rglob('*')):
            if path.is_file() and path.suffix not in {'.lck','.kicad_prl'} and path.name not in {'filled-drc.json','placement-drc.rpt','detail.png'}:
                z.write(path,path.relative_to(BASE.parent))
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:raise RuntimeError('Corrupt archive')
    print(json.dumps({'archive':str(archive),'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'fabrication_approved':False}))


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--archive-only',action='store_true')
    args=parser.parse_args()
    if args.archive_only:
        archive_package();return
    reports=BASE/'reports'
    manufacturing=BASE/'manufacturing'
    gerbers=manufacturing/'gerbers'
    for directory in [reports,gerbers]:
        directory.mkdir(parents=True,exist_ok=True)
    pcb=BASE/f'{NAME}.kicad_pcb'
    sch=BASE/f'{NAME}.kicad_sch'
    run('sch','erc','-o',reports/'erc.rpt',sch)
    run('sch','export','netlist','-o',reports/f'{NAME}.xml',sch)
    run('sch','export','pdf','-o',reports/f'{NAME}-Schematic-Review.pdf',sch)
    run('pcb','drc','--schematic-parity','--refill-zones','--save-board','--format','json','-o',reports/'drc.json',pcb)
    run('pcb','drc','--schematic-parity','-o',reports/'drc.rpt',pcb)
    checked=json.loads((reports/'drc.json').read_text(encoding='utf-8'))
    if any(checked[k] for k in ['violations','unconnected_items','schematic_parity']):
        raise RuntimeError('Native checks failed; fix findings before rebuilding manufacturing exports')
    run('pcb','export','gerbers','-l','F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts','-o',str(gerbers)+'/',pcb)
    run('pcb','export','drill','--excellon-separate-th','--generate-report','--report-path',reports/'drill-report.rpt','-o',str(manufacturing)+'/',pcb)
    run('pcb','export','pos','--format','csv','-o',manufacturing/f'{NAME}-all-pos.csv',pcb)
    run('pcb','export','step','--force','--subst-models','-o',manufacturing/f'{NAME}-PCB-Review.step',pcb)
    for side in ['top','bottom']:
        run('pcb','render','--width','1600','--height','1400','--side',side,'--background','opaque','-o',reports/f'board-{side}.png',pcb)
    board=pcbnew.LoadBoard(str(pcb))
    drc=json.loads((reports/'drc.json').read_text(encoding='utf-8'))
    counts={k:len(drc.get(k,[])) for k in ['violations','unconnected_items','schematic_parity']}
    manifest=json.loads((BASE/'design-manifest.json').read_text(encoding='utf-8'))
    power={'VBUS5','BAT','SYS','VSYS_SW','+3V3','+3V3_A','+3V6_BT','PMID','SW_CHG','SW1_3V3','SW2_3V3','SW1_BT','SW2_BT','SPK_P','SPK_N'}
    narrow=[]
    for t in board.GetTracks():
        if t.GetClass()=='PCB_TRACK' and t.GetNetname() in power and pcbnew.ToMM(t.GetWidth())<.6:
            narrow.append({'uuid':t.m_Uuid.AsString(),'net':t.GetNetname(),'layer':t.GetLayerName(),'start_mm':[pcbnew.ToMM(t.GetStart().x),pcbnew.ToMM(t.GetStart().y)],'width_mm':pcbnew.ToMM(t.GetWidth()),'length_mm':round(pcbnew.ToMM(t.GetLength()),4)})
    status={
        'generated_utc':datetime.now(timezone.utc).isoformat(),
        'pcb_sha256':hashlib.sha256(pcb.read_bytes()).hexdigest(),
        'source_hashes':source_hashes(),
        'source_hash_normalization':'CRLF to LF',
        'component_count':len(list(board.GetFootprints())),
        'track_and_via_items':len(list(board.GetTracks())),
        'drc_counts':counts,
        'connectivity_geometry_clean':all(v==0 for v in counts.values()),
        'erc_scope':'passive block-symbol connectivity only; no power/driver-type certification',
        'narrow_power_segments':narrow,
        'independent_electrical_review_approved':False,
        'fabrication_approved':False,
        'physical_build_verified':False,
        'measured_runtime_hours':None,
        'pixl_grant_acceptance_verified':False,
    }
    (reports/'validation.json').write_text(json.dumps(status,indent=2),encoding='utf-8')
    (BASE/'VALIDATION_STATUS.md').write_text(
        '# Validation status\n\n'
        f'Generated UTC: {status["generated_utc"]}\n\n'
        f'- Components: {status["component_count"]}; track/via items: {status["track_and_via_items"]}.\n'
        f'- Native KiCad DRC violations: {counts["violations"]}.\n'
        f'- Unconnected items: {counts["unconnected_items"]}.\n'
        f'- Schematic/PCB parity findings: {counts["schematic_parity"]}.\n'
        '- ERC checks connectivity using passive block-symbol pins. Zero ERC is **not** a driver/power-type review.\n'
        f'- {len(narrow)} power-net segments below 0.6 mm require explicit current-density/voltage-drop review; see `reports/validation.json`. Autorouter necking is not a current-rating approval.\n'
        '- Gerbers/drills/positions and PCB STEP are review exports only. Missing custom-part 3D bodies are not fit evidence. Board STEP is not enclosure CAD.\n'
        '- Independent electrical, USB source-current/ESD/impedance, RF, power-loop and mechanical review are pending.\n'
        '- Exact passive/connector MPNs and complete pricing remain pending; reference BOM is not a complete procurement BOM.\n'
        '- No physical device, measurements, runtime, assembly photos, demo video or Pixl acceptance is claimed.\n\n'
        '**DO NOT FABRICATE OR CONNECT A LITHIUM CELL before closing `REVIEW_CHECKLIST.md`.**\n',encoding='utf-8')
    (manufacturing/'DO_NOT_FABRICATE.txt').write_text(
        'UNBUILT AI-AUTHORED REVIEW EXPORTS - NOT FABRICATION APPROVED\n'
        'See ../VALIDATION_STATUS.md and ../REVIEW_CHECKLIST.md.\n'
        'A clean DRC is not charging, thermal, RF, USB or mechanical qualification.\n',encoding='utf-8')
    u1=next(c for c in manifest['components'] if c['reference']=='U1')
    u11=next(c for c in manifest['components'] if c['reference']=='U11')
    lines=['# Reference GPIO map','',
           'This map matches the reference schematic, not the historical OLED/direct-button firmware configuration. Live adapters remain unqualified.','',
           '| Module pad | GPIO/function | Net |','| --- | --- | --- |']
    for pin,row in u1['pin_nets'].items():
        lines.append(f'| {pin} | {row["name"]} | {row["net"]} |')
    lines.extend(['','## TCA9535 input map, address 0x20','',
                  '| IC pad | Port/function | Net |','| --- | --- | --- |'])
    for pin,row in u11['pin_nets'].items():
        lines.append(f'| {pin} | {row["name"]} | {row["net"]} |')
    (BASE/'FINAL_GPIO_MAP.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    archive_package()


if __name__=='__main__':
    main()
