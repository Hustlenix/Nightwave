"""Bounded geometric closure of reported reference routing findings.

Uses native DRC as the authority after changes. This is not SI/current approval.
"""
from pathlib import Path
import json
import math
import heapq
import pcbnew

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'hardware/kicad/nightwave-reference'
PCB=BASE/'Nightwave-Reference.kicad_pcb'
GRID=.05


def mm(point):
    return (pcbnew.ToMM(point.x),pcbnew.ToMM(point.y))


def point(xy):
    return pcbnew.VECTOR2I(pcbnew.FromMM(xy[0]),pcbnew.FromMM(xy[1]))


def track(board,net,a,b,layer,width=.2):
    if math.dist(a,b)<1e-6:
        return
    t=pcbnew.PCB_TRACK(board);t.SetNet(net)
    t.SetStart(point(a));t.SetEnd(point(b));t.SetWidth(pcbnew.FromMM(width));t.SetLayer(layer)
    board.Add(t)
    t.thisown=False


def via(board,net,xy):
    v=pcbnew.PCB_VIA(board);v.SetNet(net);v.SetPosition(point(xy))
    v.SetWidth(pcbnew.FromMM(.6));v.SetDrill(pcbnew.FromMM(.3))
    v.SetViaType(pcbnew.VIATYPE_THROUGH);v.SetLayerPair(pcbnew.F_Cu,pcbnew.B_Cu)
    board.Add(v)
    v.thisown=False


def occupancy(board,net,layer,half_width=.075):
    blocked=set()
    def rect(x0,y0,x1,y1):
        for x in range(math.floor(x0/GRID),math.ceil(x1/GRID)+1):
            for y in range(math.floor(y0/GRID),math.ceil(y1/GRID)+1):
                blocked.add((x,y))
    def line(a,b,r):
        x0,x1=math.floor((min(a[0],b[0])-r)/GRID),math.ceil((max(a[0],b[0])+r)/GRID)
        y0,y1=math.floor((min(a[1],b[1])-r)/GRID),math.ceil((max(a[1],b[1])+r)/GRID)
        dx,dy=b[0]-a[0],b[1]-a[1]
        length=dx*dx+dy*dy
        for x in range(x0,x1+1):
            for y in range(y0,y1+1):
                px,py=x*GRID,y*GRID
                u=max(0,min(1,((px-a[0])*dx+(py-a[1])*dy)/length)) if length else 0
                if math.hypot(px-a[0]-u*dx,py-a[1]-u*dy)<r:
                    blocked.add((x,y))
    for t in board.GetTracks():
        if t.GetNetname()==net.GetNetname():
            continue
        if t.GetClass()=='PCB_VIA' or t.GetLayer()==layer:
            width=t.GetWidth(layer) if t.GetClass()=='PCB_VIA' else t.GetWidth()
            line(mm(t.GetStart()),mm(t.GetEnd()),pcbnew.ToMM(width)/2+.15+half_width+.025)
    for fp in list(board.GetFootprints()):
        for pad in fp.Pads():
            if pad.GetNetname()==net.GetNetname():
                continue
            if pad.FlashLayer(layer) or pad.GetAttribute()==pcbnew.PAD_ATTRIB_NPTH:
                box=pad.GetBoundingBox()
                margin=.15+half_width+.025
                rect(box.GetLeft()/1e6-margin,box.GetTop()/1e6-margin,box.GetRight()/1e6+margin,box.GetBottom()/1e6+margin)
    for z in board.Zones():
        if z.GetIsRuleArea() and z.GetDoNotAllowTracks():
            box=z.GetBoundingBox()
            rect(box.GetLeft()/1e6-.2,box.GetTop()/1e6-.2,box.GetRight()/1e6+.2,box.GetBottom()/1e6+.2)
    return blocked


def astar(blocked,a,b):
    start=tuple(round(v/GRID) for v in a);end=tuple(round(v/GRID) for v in b)
    blocked.discard(start);blocked.discard(end)
    queue=[(math.dist(start,end),0,start)];cost={start:0};parent={}
    while queue:
        _,g,p=heapq.heappop(queue)
        if p==end:
            result=[p]
            while p in parent:
                p=parent[p];result.append(p)
            return [tuple(v*GRID for v in q) for q in reversed(result)]
        if g>cost[p]:
            continue
        for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]:
            q=(p[0]+dx,p[1]+dy)
            if not (5.6/GRID<=q[0]<=114.4/GRID and 5.6/GRID<=q[1]<=104.4/GRID) or q in blocked:
                continue
            if dx and dy and ((p[0]+dx,p[1]) in blocked or (p[0],p[1]+dy) in blocked):
                continue
            value=g+math.hypot(dx,dy)
            if value<cost.get(q,math.inf):
                cost[q]=value;parent[q]=p
                heapq.heappush(queue,(value+math.dist(q,end),value,q))
    raise RuntimeError('No legal raster escape path; native/manual review required')


def main():
    report=json.loads((BASE/'reports/filled-drc.json').read_text(encoding='utf-8'))
    board=pcbnew.LoadBoard(str(PCB))
    # Keep typed wrappers alive while detaching SWIG-owned items. Re-querying
    # during mutation can expose untyped pointers in KiCad 10's bindings.
    footprints=list(board.GetFootprints())
    tracks=list(board.GetTracks())
    ids={item['uuid'] for v in report['violations'] if v['type'] in ['via_dangling','track_dangling'] for item in v['items']}
    for t in tracks:
        if t.m_Uuid.AsString() in ids:
            board.Remove(t)
    unresolved=' '.join(i['description'] for v in report['unconnected_items'] for i in v['items'])
    for fp in footprints:
        if fp.GetReference()=='U12':
            for pad in fp.Pads():
                if pad.GetNumber() in ['56','57'] and f'Pad {pad.GetNumber()} [GND]' in unresolved:
                    xy=mm(pad.GetPosition())
                    if not any(t.GetClass()=='PCB_VIA' and math.dist(mm(t.GetPosition()),xy)<.01 for t in board.GetTracks()):
                        via(board,board.FindNet('GND'),xy)
    if '[CHG_INT_N]' in unresolved:
        net=board.FindNet('CHG_INT_N')
        pad=next(p for fp in footprints if fp.GetReference()=='U3' for p in fp.Pads() if p.GetNumber()=='11')
        a=mm(pad.GetPosition())
        # The autorouter's SDA jog closes the interrupt-pad escape corridor.
        # Lift that local jog by 0.3508 mm; preserve its existing bus via and
        # verify all actual clearances with native DRC afterwards.
        sda=board.FindNet('I2C_SDA')
        for t in tracks:
            if t.GetClass()=='PCB_TRACK' and t.GetNetname()=='I2C_SDA' and t.GetLayer()==pcbnew.F_Cu:
                p,q=mm(t.GetStart()),mm(t.GetEnd())
                if 41.39<=min(p[0],q[0]) and max(p[0],q[0])<=43.24 and 55.0<=min(p[1],q[1]) and max(p[1],q[1])<=55.36:
                    board.Remove(t)
        track(board,sda,(41.4,55.007),(43.2356,55.007),pcbnew.F_Cu,.15)
        track(board,sda,(43.2356,55.007),(43.2356,55.3578),pcbnew.F_Cu,.15)
        targets=[mm(t.GetPosition()) for t in board.GetTracks() if t.GetClass()=='PCB_VIA' and t.GetNetname()=='CHG_INT_N']
        path=None
        front=occupancy(board,net,pcbnew.F_Cu)
        via_blocked=occupancy(board,net,pcbnew.F_Cu,.3)
        front_path=None
        for layer in [pcbnew.In2_Cu,pcbnew.B_Cu,pcbnew.In1_Cu]:
            blocked=occupancy(board,net,layer)
            candidates=[(42.5,55.55)]+[(a[0]+dx,a[1]+dy) for dx in [.8,1.2,1.6,2,2.5,3] for dy in [0,.6,-.6,1,-1,1.5,-1.5,2,-2]]
            for escape in candidates:
                cell=tuple(round(v/GRID) for v in escape)
                if (cell in via_blocked and escape!=(42.5,55.55)) or cell in blocked:
                    continue
                target=min(targets,key=lambda p:math.dist(p,escape))
                try:
                    front_path=[a,(42.357,55.407),escape] if escape==(42.5,55.55) else astar(set(front),a,escape)
                    path=astar(set(blocked),escape,target)
                    break
                except RuntimeError:
                    continue
            if path:
                break
        if not path:
            raise RuntimeError('No escape candidate passed bounded routing')
        points=[escape,*path,target]
        # Retain only bends, with exact pad/via endpoints.
        simple=[points[0]]
        for i in range(1,len(points)-1):
            previous,current,nxt=simple[-1],points[i],points[i+1]
            cross=(current[0]-previous[0])*(nxt[1]-current[1])-(current[1]-previous[1])*(nxt[0]-current[0])
            if abs(cross)>1e-8:
                simple.append(current)
        simple.append(target)
        for p,q in zip([a,*front_path], [*front_path,escape]):
            track(board,net,p,q,pcbnew.F_Cu,.15)
        via(board,net,escape)
        for a,b in zip(simple,simple[1:]):
            track(board,net,a,b,layer,.15)
        print(f'Closed charger interrupt with {len(simple)-1} inner-layer segments')
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(str(PCB),board)
    print(f'Removed {len(ids)} reported dangling items; added ground stitching as needed')


if __name__=='__main__':
    main()
