"""Close specific routing defects while preserving antenna exclusions.

Native DRC must be rerun; this script itself is not a pass certificate.
"""
from repair_nightwave_route import *
from generate_nightwave_reference import bm83_ground_windows,build_custom_footprints


def simplified(points):
    result=[points[0]]
    for i in range(1,len(points)-1):
        a,b,c=result[-1],points[i],points[i+1]
        if abs((b[0]-a[0])*(c[1]-b[1])-(b[1]-a[1])*(c[0]-b[0]))>1e-8:
            result.append(b)
    return result+[points[-1]]


def main():
    board=pcbnew.LoadBoard(str(PCB))
    fps=list(board.GetFootprints());ts=list(board.GetTracks())
    build_custom_footprints()
    fp=next(f for f in fps if f.GetReference()=='U12')
    bm83_ground_windows(fp)
    fp.SetFPID(pcbnew.LIB_ID('Nightwave','Microchip_BM83_GroundLands'))
    for t in ts:
        if t.GetClass()=='PCB_TRACK' and t.GetNetname()=='I2C_SDA' and t.GetLayer()==pcbnew.F_Cu:
            for getter,setter in [(t.GetStart,t.SetStart),(t.GetEnd,t.SetEnd)]:
                p=mm(getter())
                if math.dist(p,(41.6,55.007))<.001:setter(point((41.8,55.007)))
                if math.dist(p,(41.7,55.107))<.001:setter(point((41.9,55.107)))
        if t.m_Uuid.AsString()=='e9016650-a9a9-4e5b-b8de-3afec3a2d4e2':
            t.SetPosition(point((104.32,28.08)))
    # Lower only the SDA escape, staying between the SCL via and IRQ trace.
    ids={'279dc6e8-1539-4bc6-b253-7537e8ee2a22','3a11d69b-9f67-4318-b775-e1af7d1addde'}
    removed=False
    for t in ts:
        if t.m_Uuid.AsString() in ids:
            board.Remove(t);removed=True
    if removed:
        points=[(41.4,55.007),(41.8,55.007),(41.9,55.107),(43.2356,55.107),(43.2356,55.3578)]
        for a,b in zip(points,points[1:]):
            track(board,board.FindNet('I2C_SDA'),a,b,pcbnew.F_Cu,.15)
    # Re-route the slow power-switch control around the IRQ through-via.
    for t in ts:
        if t.m_Uuid.AsString()=='c37a03a8-dd4d-4a0d-b0d5-d9c8a392bf2f':
            a,b=mm(t.GetStart()),mm(t.GetEnd());net=t.GetNet();layer=t.GetLayer()
            board.Remove(t)
            path=simplified([a,*astar(occupancy(board,net,layer,.1),a,b),b])
            for p,q in zip(path,path[1:]):
                track(board,net,p,q,layer,.2)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
    pcbnew.SaveBoard(str(PCB),board)
    print('Applied local DRC repairs; antenna keepouts preserved')


if __name__=='__main__':main()
