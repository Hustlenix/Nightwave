"""Cross-check the compiled reference profile's source map against schematic nets."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAPPING = {
    'kSdClk': ('20','SD_CLK'), 'kSdCmd': ('19','SD_CMD'), 'kSdD0': ('21','SD_D0'),
    'kI2sBitClock': ('5','I2S_BCLK'), 'kI2sWordSelect': ('6','I2S_LRCLK'),
    'kI2sDataOut': ('7','I2S_DOUT'), 'kI2cSda': ('12','I2C_SDA'), 'kI2cScl': ('17','I2C_SCL'),
    'kHeadphoneDetect': ('9','HP_DETECT'), 'kSpeakerEnable': ('10','SPK_EN'),
    'kDacMute': ('11','DAC_XSMT'), 'kHeadphoneEnable': ('32','HP_EN'),
    'kTcaInterrupt': ('22','TCA_INT_N'), 'kBtMfb': ('4','BT_MFB'),
    'kBtReset': ('8','BT_RST_N'), 'kBtWake': ('18','BT_WAKE'),
    'kBtMclk': ('23','BT_MCLK_OPTION'), 'kBtRx': ('38','BT_UART_RX'), 'kBtTx': ('39','BT_UART_TX'),
    'kDisplayMosi': ('24','TFT_MOSI'), 'kDisplayClock': ('25','TFT_SCLK'),
    'kDisplayBacklight': ('31','TFT_BL_PWM'), 'kDisplayReset': ('33','TFT_RST'),
    'kDisplayDc': ('34','TFT_DC'), 'kDisplayCs': ('35','TFT_CS'),
}


def validate(header=None, manifest=None):
    if header is None:
        header=(ROOT/'firmware/components/app_state/include/nightwave/reference_board.h').read_text()
    if manifest is None:
        manifest=json.loads((ROOT/'hardware/kicad/nightwave-reference/design-manifest.json').read_text())
    parts={c['reference']:c for c in manifest['components']}
    pins=parts['U1']['pin_nets']
    constants={key:int(value) for key,value in re.findall(r'std::int8_t (k\w+) = (\d+);', header)}
    assert set(constants)==set(MAPPING), 'Missing or unvalidated reference-board GPIO'
    assert len(set(constants.values()))==len(constants), 'Duplicate reference-board GPIO'
    for key,(pad,net) in MAPPING.items():
        assert pins[pad]['name']==f'GPIO{constants[key]}', f'{key} GPIO/module-pad mismatch'
        assert pins[pad]['net']==net, f'{key} net mismatch'
    for i, net in enumerate(['BTN_PREV_N','BTN_PLAY_N','BTN_NEXT_N','BTN_VOL_DOWN_N','BTN_VOL_UP_N','SD_DETECT_N']):
        assert parts['U11']['pin_nets'][str(4+i)]['net']==net
    assert 'kTcaAddress = 0x20;' in header
    assert 'kButtonBits{0, 1, 2, 3, 4};' in header and 'kSdDetectBit = 5;' in header
    for pad in ['2','3','21']:
        assert parts['U11']['pin_nets'][pad]['net']=='GND'
    return len(constants)


if __name__=='__main__':
    print(f'Reference firmware/schematic GPIO checks passed ({validate()} signals); not bench qualification')
