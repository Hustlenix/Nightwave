"""Prepare/export/import a Nightwave routing candidate with KiCad Python.

The router output is a geometry candidate. It does not certify power loops,
USB impedance, RF performance, or assembly/firmware qualification.
"""
from pathlib import Path
import argparse
import json
import pcbnew

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'hardware/kicad/nightwave-reference'
PCB=BASE/'Nightwave-Reference.kicad_pcb'
SCRATCH=ROOT/'.generated/nightwave-routing'


def prepare(board):
    nc=board.GetDesignSettings().m_NetSettings.GetDefaultNetclass()
    nc.SetClearance(pcbnew.FromMM(0.15))
    nc.SetTrackWidth(pcbnew.FromMM(0.2))
    nc.SetViaDiameter(pcbnew.FromMM(0.6))
    nc.SetViaDrill(pcbnew.FromMM(0.3))
    settings=board.GetDesignSettings().m_NetSettings
    power=pcbnew.NETCLASS('Power')
    power.SetClearance(pcbnew.FromMM(.15))
    power.SetTrackWidth(pcbnew.FromMM(.6))
    power.SetViaDiameter(pcbnew.FromMM(.8))
    power.SetViaDrill(pcbnew.FromMM(.4))
    settings.SetNetclass('Power',power)
    for net in ['VBUS5','BAT','SYS','VSYS_SW','+3V3','+3V3_A','+3V6_BT','SW_CHG','SW1_3V3','SW2_3V3','SW1_BT','SW2_BT','SPK_P','SPK_N']:
        settings.SetNetclassPatternAssignment(net,'Power')
    for zone in list(board.Zones()):
        if zone.GetIsRuleArea() and zone.GetZoneName()!='MOUNT_CLEARANCE':
            board.Remove(zone)
    for fp in board.GetFootprints():
        fp.Reference().SetLayer(pcbnew.F_Fab)
        fp.Reference().SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8),pcbnew.FromMM(0.8)))
        fp.Reference().SetTextThickness(pcbnew.FromMM(0.12))
        fp.Value().SetVisible(False)
        if fp.GetReference() in ('U1','U12'):
            x,y=pcbnew.ToMM(fp.GetPosition().x),pcbnew.ToMM(fp.GetPosition().y)
            # PCB trace antenna and its adjacent air/copper exclusion.
            box=(x-24,max(5.5,y-28),x+24,y-6.75) if fp.GetReference()=='U1' else (x-15,max(5.5,y-31),min(114.5,x+15),y-10.5)
            z=pcbnew.ZONE(board)
            z.SetIsRuleArea(True)
            z.SetLayerSet(pcbnew.LSET.AllCuMask(4))
            z.SetDoNotAllowZoneFills(True)
            z.SetDoNotAllowTracks(True)
            z.SetDoNotAllowVias(True)
            z.SetDoNotAllowPads(True)
            o=z.Outline();o.NewOutline()
            for px,py in [(box[0],box[1]),(box[2],box[1]),(box[2],box[3]),(box[0],box[3])]:
                o.Append(pcbnew.FromMM(px),pcbnew.FromMM(py))
            board.Add(z)


def add_ground_planes(board):
    """Add continuous-return pours; final current/SI review remains required."""
    for layer,net_name in ((pcbnew.In1_Cu,'GND'),(pcbnew.B_Cu,'GND'),(pcbnew.In2_Cu,'+3V3')):
        if any(not z.GetIsRuleArea() and z.GetLayer()==layer for z in board.Zones()):
            continue
        z=pcbnew.ZONE(board)
        z.SetLayer(layer)
        z.SetNetCode(board.FindNet(net_name).GetNetCode())
        z.SetLocalClearance(pcbnew.FromMM(.2))
        z.SetMinThickness(pcbnew.FromMM(.15))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        z.SetMinIslandArea(1_000_000_000_000)
        o=z.Outline();o.NewOutline()
        for x,y in [(5.5,5.5),(114.5,5.5),(114.5,104.5),(5.5,104.5)]:
            o.Append(pcbnew.FromMM(x),pcbnew.FromMM(y))
        board.Add(z)


def sync_manifest(board):
    """Apply explicit final pin changes without regenerating placed/routed PCB.

    Changed nets are ripped up for routing again, never silently reassigned to
    another net. Single-pad NC fanout is removed too.
    """
    manifest=json.loads((BASE/'design-manifest.json').read_text(encoding='utf-8'))
    byref={c['reference']:c for c in manifest['components']}
    changed=set()
    for fp in board.GetFootprints():
        if fp.GetAttributes() & pcbnew.FP_BOARD_ONLY:
            continue
        comp=byref[fp.GetReference()]
        fp.SetValue(comp['value'])
        fp.SetField('Datasheet',comp['datasheet'])
        for pad in fp.Pads():
            number=pad.GetNumber()
            if not number:
                continue
            data=comp['pin_nets'][number]
            name=data['net']
            if name=='NC':
                pname=data['name'].replace('/','{slash}')
                name=f'unconnected-({fp.GetReference()}-{pname}-Pad{number})'
            if name==pad.GetNetname():
                continue
            changed.add(pad.GetNetname())
            net=board.FindNet(name)
            if not net:
                net=pcbnew.NETINFO_ITEM(board,name);board.Add(net)
            pad.SetNet(net)
    for track in list(board.GetTracks()):
        if track.GetNetname() in changed:
            board.Remove(track)
    print('Ripped up changed nets:',sorted(changed))


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('action',choices=['export','import','planes','sync'])
    args=parser.parse_args()
    SCRATCH.mkdir(parents=True,exist_ok=True)
    board=pcbnew.LoadBoard(str(PCB))
    if args.action=='export':
        prepare(board)
        pcbnew.SaveBoard(str(PCB),board)
        if not pcbnew.ExportSpecctraDSN(board,str(SCRATCH/'Nightwave.dsn')):
            raise RuntimeError('DSN export failed')
        print('Exported DSN with antenna copper/track/via keepouts')
    elif args.action=='import':
        if not pcbnew.ImportSpecctraSES(board,str(SCRATCH/'Nightwave.ses')):
            raise RuntimeError('SES import failed')
        pcbnew.SaveBoard(str(PCB),board)
        print(f'Imported {len(list(board.GetTracks()))} track/via items')
    elif args.action=='planes':
        add_ground_planes(board)
        pcbnew.ZONE_FILLER(board).Fill(board.Zones())
        pcbnew.SaveBoard(str(PCB),board)
        print('Filled ground/3.3 V planes with all-layer antenna exclusions')
    else:
        sync_manifest(board)
        pcbnew.SaveBoard(str(PCB),board)

if __name__=='__main__':
    main()
