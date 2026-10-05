"""Add functional assembly legends without claiming fabrication approval."""
from repair_nightwave_route import *

LABELS=[
    ('NIGHTWAVE R0',(63,42),1.8),
    ('AI REVIEW - NOT FOR FABRICATION',(63,45),.8),
    ('PREV',(25,92),.8),('PLAY',(39,92),.8),('NEXT',(53,92),.8),
    ('VOL-',(67,92),.8),('VOL+',(81,92),.8),
    ('BATTERY 1S',(89,50.5),.9),('+',(88,41.2),.8),('-', (90,41.2),.8),
    ('PACK NTC',(89,61.5),.9),('BTL SPEAKER',(108,84.5),.8),
    ('TFT HARNESS',(18.5,45.5),.8),('POWER',(12,77.2),.8),
]


def main():
    board=pcbnew.LoadBoard(str(PCB))
    existing={d.GetText():d for d in board.GetDrawings() if isinstance(d,pcbnew.PCB_TEXT)}
    for value,xy,size in LABELS:
        if value in existing:
            existing[value].SetPosition(point(xy));continue
        text=pcbnew.PCB_TEXT(board);text.SetText(value)
        text.SetPosition(point(xy));text.SetLayer(pcbnew.F_SilkS)
        text.SetTextSize(point((size,size)));text.SetTextThickness(pcbnew.FromMM(.12))
        board.Add(text);text.thisown=False
    pcbnew.SaveBoard(str(PCB),board)
    print('Added board identification, button, connector and polarity legends')


if __name__=='__main__':main()
