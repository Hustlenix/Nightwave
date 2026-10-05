"""Increase power-trace widths using KiCad's actual copper-shape collision API.

This preserves topology and never reduces existing clearance rules. Native
DRC still must pass after repouring. Widths alone do not qualify power loops.
"""
from repair_nightwave_route import *

POWER={'VBUS5','BAT','SYS','VSYS_SW','+3V3','+3V3_A','+3V6_BT',
       'SW_CHG','SW1_3V3','SW2_3V3','SW1_BT','SW2_BT','SPK_P','SPK_N'}


def main():
    board=pcbnew.LoadBoard(str(PCB))
    fps=list(board.GetFootprints());ts=list(board.GetTracks())
    pads=[p for f in fps for p in f.Pads()]
    # A fine-pitch endpoint need not force its entire multi-mm feed to the
    # endpoint's narrow width. Split only existing power-neck candidates.
    for t in ts:
        if t.GetClass()!='PCB_TRACK' or t.GetNetname() not in POWER or t.GetWidth()>=600000 or t.GetLength()<=1000000:
            continue
        a,b=mm(t.GetStart()),mm(t.GetEnd());n=math.ceil(pcbnew.ToMM(t.GetLength())/.5)
        points=[(a[0]+(b[0]-a[0])*i/n,a[1]+(b[1]-a[1])*i/n) for i in range(n+1)]
        for p,q in zip(points,points[1:]):track(board,t.GetNet(),p,q,t.GetLayer(),pcbnew.ToMM(t.GetWidth()))
        board.Remove(t)
    ts=list(board.GetTracks())
    changed=[]
    for t in ts:
        if t.GetClass()!='PCB_TRACK' or t.GetNetname() not in POWER or t.GetWidth()>=600000:
            continue
        layer=t.GetLayer();net=t.GetNetCode();old=t.GetWidth()
        # Only objects near the maximum-width candidate need collision tests.
        a,b=t.GetStart(),t.GetEnd()
        left,right=min(a.x,b.x)-800000,max(a.x,b.x)+800000
        top,bottom=min(a.y,b.y)-800000,max(a.y,b.y)+800000
        def near(bb):
            return bb.GetRight()>=left and bb.GetLeft()<=right and bb.GetBottom()>=top and bb.GetTop()<=bottom
        obstacles=[]
        for p in pads:
            if p.GetNetCode()==net or not near(p.GetBoundingBox()):continue
            if p.FlashLayer(layer):obstacles.append(p.GetEffectiveShape(layer))
            elif p.GetAttribute()==pcbnew.PAD_ATTRIB_NPTH:obstacles.append(p.GetEffectiveHoleShape())
        for other in ts:
            if other.GetNetCode()==net or not near(other.GetBoundingBox()):continue
            isvia=other.GetClass()=='PCB_VIA'
            if not isvia and other.GetLayer()!=layer:continue
            width=other.GetWidth(layer) if isvia else other.GetWidth()
            obstacles.append(pcbnew.SHAPE_SEGMENT(other.GetStart(),other.GetEnd(),width))
        for width in range(600000,old,-25000):
            candidate=pcbnew.SHAPE_SEGMENT(a,b,width)
            if not any(candidate.Collide(o,151000) for o in obstacles):
                t.SetWidth(width)
                changed.append({'uuid':t.m_Uuid.AsString(),'net':t.GetNetname(),'length_mm':pcbnew.ToMM(t.GetLength()),'old_mm':old/1e6,'new_mm':width/1e6})
                break
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(str(PCB),board)
    report=BASE/'reports/power-width-improvements.json'
    previous=json.loads(report.read_text(encoding='utf-8')) if report.exists() else []
    report.write_text(json.dumps(previous+changed,indent=2),encoding='utf-8')
    print(f'Widened {len(changed)} power segments; native DRC required')


if __name__=='__main__':main()
