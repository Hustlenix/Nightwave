"""Add three checked, non-plated M2 mounting holes to the review PCB."""
from repair_nightwave_route import *

MOUNTS=[('H1',(71,9)),('H2',(9,101)),('H3',(111,101))]


def main():
    board=pcbnew.LoadBoard(str(PCB));fps=list(board.GetFootprints())
    existing={f.GetReference() for f in fps}
    for z in board.Zones():
        if z.GetIsRuleArea() and any(math.dist(mm(z.GetBoundingBox().GetCenter()),xy)<.01 for _,xy in MOUNTS):
            z.SetDoNotAllowPads(False)
            z.SetZoneName('MOUNT_CLEARANCE')
    for ref,xy in MOUNTS:
        if ref in existing:continue
        fp=pcbnew.FootprintLoad(r'C:\Program Files\KiCad\10.0\share\kicad\footprints\MountingHole.pretty','MountingHole_2.2mm_M2')
        fp.SetReference(ref);fp.SetValue('M2_NPTH_NYLON_HARDWARE')
        fp.SetFPID(pcbnew.LIB_ID('MountingHole','MountingHole_2.2mm_M2'))
        fp.SetPosition(point(xy))
        fp.SetAttributes(fp.GetAttributes()|pcbnew.FP_BOARD_ONLY|pcbnew.FP_EXCLUDE_FROM_BOM|pcbnew.FP_EXCLUDE_FROM_POS_FILES)
        fp.Reference().SetLayer(pcbnew.F_Fab);fp.Value().SetVisible(False)
        board.Add(fp);fp.thisown=False
        z=pcbnew.ZONE(board);z.SetIsRuleArea(True)
        z.SetLayerSet(pcbnew.LSET.AllCuMask(4))
        z.SetDoNotAllowPads(False);z.SetZoneName('MOUNT_CLEARANCE')
        z.SetDoNotAllowZoneFills(True);z.SetDoNotAllowTracks(True);z.SetDoNotAllowVias(True)
        outline=z.Outline();outline.NewOutline()
        for i in range(64):
            theta=2*math.pi*i/64
            outline.Append(pcbnew.FromMM(xy[0]+2.5*math.cos(theta)),pcbnew.FromMM(xy[1]+2.5*math.sin(theta)))
        board.Add(z);z.thisown=False
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(str(PCB),board)
    print('Three 2.2 mm NPTH mounting holes with 5 mm copper-free regions')


if __name__=='__main__':main()
