"""Semantic regressions for the AI reference; not electrical/physical sign-off."""
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'hardware/kicad/nightwave-reference'


def validate(manifest=None):
    if manifest is None:
        manifest=json.loads((BASE/'design-manifest.json').read_text(encoding='utf-8'))
    parts={c['reference']:c for c in manifest['components']}
    assert len(parts)==len(manifest['components']), 'Duplicate references'
    def net(ref,pin):
        return parts[ref]['pin_nets'][str(pin)]['net']
    assert manifest['board']['layers']==4
    assert manifest['board']['thickness_mm']==1.2
    assert net('U12',23)=='+3V6_BT'
    assert net('U12',22)=='NC', 'BM83 ADAP_IN must not be fed from BAT_IN rail'
    assert net('U12',25)=='NC', 'BM83 internal VDD_IO must not be driven externally'
    assert net('U10',17)=='GND', 'Speaker amplifier exposed pad needs ground'
    assert parts['U3']['footprint']=='Nightwave:TI_RYK0018A'
    assert parts['U5']['footprint']==parts['U6']['footprint']=='Nightwave:TI_DLA0010A'
    assert parts['U12']['footprint']=='Nightwave:Microchip_BM83_GroundLands'
    for ref in ['J2','J3','J6']:
        assert parts[ref]['mpn']=='S2B-PH-K-S', 'PCB headers must not use off-board load MPNs'
    assert parts['J7']['mpn']=='SM08B-GHS-TB(LF)(SN)'
    assert net('U12',56)==net('U12',57)=='GND'
    assert net('U1',13)=='USB_DM_MCU' and net('U1',14)=='USB_DP_MCU'
    for pin in [15,16,26,28,29,30]:
        assert net('U1',pin)=='NC', 'Straps and PSRAM must remain isolated'
    assert net('J4',2)=='SD_DAT3'
    for ref,signal in [('R48','SD_CMD'),('R49','SD_D0'),('R50','SD_DAT3')]:
        assert set([net(ref,1),net(ref,2)])=={signal,'+3V3'}
    assert net('U10',4)=='SPK_SD_MODE'
    assert parts['R51']['value']=='47k'
    assert set([net('R51',1),net('R51',2)])=={'SPK_EN','SPK_SD_MODE'}
    assert net('R27',1)=='SPK_SD_MODE' and net('R27',2)=='GND'
    assert net('J5',6)=='HP_DETECT' and net('J5',5)=='GND'
    assert net('J5',2)=='HP_L' and net('J5',3)=='HP_R'
    assert net('J6',1)=='SPK_P' and net('J6',2)=='SPK_N'
    assert all(parts['U11']['pin_nets'][str(i)]['net']=='GND' for i in [2,3,21])
    return len(parts)


if __name__=='__main__':
    print(f'Nightwave reference semantic checks passed ({validate()} components); no physical approval implied')
