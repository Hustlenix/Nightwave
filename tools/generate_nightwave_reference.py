"""Generate the Nightwave AI-authored KiCad reference package.

This script intentionally produces a reviewable engineering reference, not a
claim of fabrication approval.  It uses KiCad's installed symbols/footprints,
keeps schematic and PCB UUID paths identical, and emits a machine-readable
design manifest alongside the native KiCad sources.

Run with KiCad's bundled Python:
  C:\\Program Files\\KiCad\\10.0\\bin\\python.exe tools\\generate_nightwave_reference.py
"""

from __future__ import annotations

import csv
import json
import re
import shutil
import uuid
from dataclasses import dataclass, field
from pathlib import Path

import pcbnew


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "hardware" / "kicad" / "nightwave-reference"
KICAD = Path(r"C:\Program Files\KiCad\10.0\share\kicad")
SYMBOLS = KICAD / "symbols"
FOOTPRINTS = KICAD / "footprints"
PROJECT = "Nightwave-Reference"
ROOT_UUID = str(uuid.uuid5(uuid.NAMESPACE_URL, "nightwave/reference/root/v1"))


def uid(key: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "nightwave/reference/v1/" + key))


def q(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def sexpr_block(text: str, marker: str) -> str:
    start = text.index(marker)
    depth = 0
    quoted = escaped = False
    for index in range(start, len(text)):
        char = text[index]
        if escaped:
            escaped = False
        elif quoted and char == "\\":
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif not quoted:
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    return text[start : index + 1]
    raise ValueError(f"Unbalanced s-expression: {marker}")


def load_symbol(lib: str, name: str) -> tuple[str, list[dict[str, object]]]:
    path = SYMBOLS / f"{lib}.kicad_sym"
    text = path.read_text(encoding="utf-8")
    marker = f'(symbol "{name}"'
    if marker not in text:
        raise ValueError(f"Symbol not found: {lib}:{name}")
    block = sexpr_block(text, marker)
    embedded = block.replace(marker, f'(symbol "{lib}:{name}"', 1)
    unit_matches = list(re.finditer(r'\(symbol\s+"[^"]+_1_1"', block))
    pin_source = block
    if unit_matches:
        unit_marker = unit_matches[0].group(0)
        pin_source = sexpr_block(block, unit_marker)
    pins: list[dict[str, object]] = []
    seen: set[str] = set()
    cursor = 0
    while True:
        match = re.search(r"\(pin\s+(?:input|output|bidirectional|tri_state|passive|power_in|power_out|open_collector|open_emitter|no_connect|unspecified)\s+", pin_source[cursor:])
        if not match:
            break
        start = cursor + match.start()
        # Extract locally because several pin blocks share the same prefix.
        depth = 0
        quoted = escaped = False
        end = start
        for end in range(start, len(pin_source)):
            ch = pin_source[end]
            if escaped:
                escaped = False
            elif quoted and ch == "\\":
                escaped = True
            elif ch == '"':
                quoted = not quoted
            elif not quoted:
                if ch == "(": depth += 1
                elif ch == ")":
                    depth -= 1
                    if depth == 0:
                        end += 1
                        break
        pin_block = pin_source[start:end]
        cursor = end
        at = re.search(r"\(at\s+(-?[0-9.]+)\s+(-?[0-9.]+)\s+(-?[0-9.]+)\)", pin_block)
        number = re.search(r'\(number\s+"([^"]+)"', pin_block)
        pname = re.search(r'\(name\s+"([^"]*)"', pin_block)
        if not at or not number or number.group(1) in seen:
            continue
        seen.add(number.group(1))
        pins.append({
            "number": number.group(1),
            "name": pname.group(1) if pname else "",
            "x": float(at.group(1)),
            "y": float(at.group(2)),
            "angle": float(at.group(3)),
        })
    if not pins:
        raise ValueError(f"No pins parsed from {lib}:{name}")
    return embedded, pins


@dataclass
class Component:
    ref: str
    value: str
    sheet: str
    lib: str
    symbol: str
    footprint: str
    x: float
    y: float
    board_x: float
    board_y: float
    rotation: float = 0.0
    nets: dict[str, str] = field(default_factory=dict)
    name_nets: dict[str, str] = field(default_factory=dict)
    datasheet: str = ""
    description: str = ""
    mpn: str = ""
    manufacturer: str = ""
    unit_price: str = ""
    supplier_url: str = ""
    symbol_uuid: str = ""
    sheet_uuid: str = ""
    parsed_pins: list[dict[str, object]] = field(default_factory=list)


SHEETS = [
    ("01_power_charging", "Power, charging and battery safety"),
    ("02_mcu_storage", "MCU, USB and microSD storage"),
    ("03_local_audio", "I2S DAC, headphone and speaker audio"),
    ("04_display_controls", "TFT display, input expander and buttons"),
    ("05_bluetooth", "BM83 Bluetooth source module"),
    ("06_connectors_test", "Service connectors, power switch and test points"),
]
SHEET_UUIDS = {name: uid("sheet/" + name) for name, _ in SHEETS}


def C(ref, value, sheet, lib, symbol, fp, x, y, bx, by, **kwargs):
    return Component(ref, value, sheet, lib, symbol, fp, x, y, bx, by, **kwargs)


components: list[Component] = [
    C("J1", "USB4105-GF-A", "01_power_charging", "Connector", "USB_C_Receptacle_USB2.0_16P",
      "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal", 45, 65, 9, 42,
      name_nets={"VBUS":"VBUS5", "GND":"GND", "SHIELD":"CHASSIS", "CC1":"USB_CC1", "CC2":"USB_CC2", "D+":"USB_DP", "D-":"USB_DM"},
      datasheet="https://gct.co/files/specs/usb4105-spec.pdf", manufacturer="GCT", mpn="USB4105-GF-A"),
    C("U2", "TUSB320LAIRWBR", "01_power_charging", "Connector_Generic", "Conn_01x12",
      "Package_DFN_QFN:Texas_X2QFN-12_1.6x1.6mm_P0.4mm", 110, 65, 25, 43,
      nets={"1":"USB_CC1","2":"USB_CC2","3":"GND","4":"VBUS5","5":"GND","6":"USB_INT_N","7":"I2C_SDA","8":"I2C_SCL","9":"GND","10":"GND","11":"+3V3","12":"+3V3"},
      datasheet="https://www.ti.com/lit/ds/symlink/tusb320.pdf", manufacturer="Texas Instruments", mpn="TUSB320LAIRWBR"),
    C("U3", "BQ25628ERYKR", "01_power_charging", "Connector_Generic", "Conn_01x25",
      "Package_DFN_QFN:WQFN-24-1EP_4x4mm_P0.5mm_EP2.6x2.6mm_ThermalVias", 175, 72, 40, 42,
      nets={"1":"VBUS5", "2":"PMID", "3":"SW_CHG", "4":"SW_CHG", "5":"GND", "6":"BAT", "7":"BAT", "8":"SYS", "9":"SYS", "10":"TS", "11":"ILIM", "12":"I2C_SDA", "13":"I2C_SCL", "14":"CHG_INT_N", "15":"CHG_STAT", "16":"REGN", "17":"BTST", "18":"PGND", "19":"PGND", "20":"VAC_OVP", "21":"DPLUS", "22":"DMINUS", "23":"QON", "24":"CE_N", "25":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/bq25628e.pdf", manufacturer="Texas Instruments", mpn="BQ25628ERYKR"),
    C("U4", "TPS22965DSGR", "01_power_charging", "Connector_Generic", "Conn_01x09",
      "Package_SON:Texas_DSG0008A_WSON-8-1EP_2x2mm_P0.5mm_EP0.9x1.6mm_ThermalVias", 240, 55, 53, 42,
      nets={"1":"SYS", "2":"SYS", "3":"PWR_SWITCH_ON", "4":"GND", "5":"VSYS_SW", "6":"VSYS_SW", "7":"VSYS_SW", "8":"VSYS_SW", "9":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps22965.pdf", manufacturer="Texas Instruments", mpn="TPS22965DSGR"),
    C("U5", "TPS63802DLAR", "01_power_charging", "Connector_Generic", "Conn_01x11",
      "Package_DFN_QFN:Texas_DSQ0010A_WSON-10-1EP_2x2mm_P0.4mm_EP0.9x1.5mm_ThermalVias", 295, 55, 65, 42,
      nets={"1":"VSYS_SW", "2":"SW1_3V3", "3":"SW2_3V3", "4":"GND", "5":"FB_3V3", "6":"+3V3", "7":"+3V3", "8":"PWR_GOOD_3V3", "9":"VSYS_SW", "10":"+3V3_EN", "11":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps63802.pdf", manufacturer="Texas Instruments", mpn="TPS63802DLAR"),
    C("U6", "TPS63802DLAR", "01_power_charging", "Connector_Generic", "Conn_01x11",
      "Package_DFN_QFN:Texas_DSQ0010A_WSON-10-1EP_2x2mm_P0.4mm_EP0.9x1.5mm_ThermalVias", 350, 55, 76, 42,
      nets={"1":"VSYS_SW", "2":"SW1_BT", "3":"SW2_BT", "4":"GND", "5":"FB_BT", "6":"+3V6_BT", "7":"+3V6_BT", "8":"PWR_GOOD_BT", "9":"VSYS_SW", "10":"+3V6_BT_EN", "11":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps63802.pdf", manufacturer="Texas Instruments", mpn="TPS63802DLAR"),
    C("U7", "MAX17048G+T10", "01_power_charging", "Connector_Generic", "Conn_01x09",
      "Package_DFN_QFN:TDFN-8-1EP_2x2mm_P0.5mm_EP0.8x1.2mm", 240, 125, 64, 56,
      nets={"1":"BAT", "2":"GND", "3":"I2C_SCL", "4":"I2C_SDA", "5":"FUEL_ALERT_N", "6":"BAT", "7":"GND", "8":"GND", "9":"GND"},
      datasheet="https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf", manufacturer="Analog Devices", mpn="MAX17048G+T10"),
    C("J2", "BATTERY_1S_10050mAh", "01_power_charging", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 320, 125, 88, 43,
      nets={"1":"BAT", "2":"GND"}, manufacturer="Adafruit", mpn="5035", unit_price="29.95", supplier_url="https://www.adafruit.com/product/5035"),
    C("J3", "EXTERNAL_10K_NTC", "01_power_charging", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 365, 125, 88, 54,
      nets={"1":"TS", "2":"GND"}, description="10 kOhm NTC physically retained against battery pack"),

    C("U1", "ESP32-S3-WROOM-1-N16R8", "02_mcu_storage", "Connector_Generic", "Conn_01x41",
      "RF_Module:ESP32-S3-WROOM-1", 130, 115, 32, 18,
      nets={"1":"GND","2":"+3V3","3":"ESP_EN","4":"BT_MFB","5":"I2S_BCLK","6":"I2S_LRCLK","7":"I2S_DOUT","8":"BT_RST_N","9":"HP_DETECT","10":"SPK_EN","11":"DAC_XSMT","12":"I2C_SDA","17":"I2C_SCL","18":"BT_WAKE","19":"SD_CMD","20":"SD_CLK","21":"SD_D0","22":"TCA_INT_N","23":"BT_MCLK_OPTION","24":"TFT_MOSI","25":"TFT_SCLK","27":"BOOT_N","31":"TFT_BL_PWM","32":"HP_EN","33":"TFT_RST","34":"TFT_DC","35":"TFT_CS","38":"BT_UART_RX","39":"BT_UART_TX","40":"GND","41":"GND"},
      datasheet="https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf", manufacturer="Espressif Systems", mpn="ESP32-S3-WROOM-1-N16R8", unit_price="6.76"),
    C("J4", "Molex 104031-0811", "02_mcu_storage", "Connector", "Micro_SD_Card",
      "Connector_Card:microSD_HC_Molex_104031-0811", 265, 95, 62, 18,
      nets={"1":"SD_D2_NC","2":"+3V3","3":"SD_CMD","4":"+3V3","5":"SD_CLK","6":"GND","7":"SD_D0","8":"SD_D1_NC","SH":"GND"},
      datasheet="https://www.molex.com/en-us/products/part-detail/1040310811", manufacturer="Molex", mpn="104031-0811", unit_price="2.21"),

    C("U8", "PCM5102APWR", "03_local_audio", "Connector_Generic", "Conn_01x20",
      "Package_SO:Texas_PW0020A_TSSOP-20_4.4x6.5mm_P0.65mm", 95, 95, 62, 68,
      nets={"1":"+3V3_A","3":"GND","6":"DAC_L","7":"DAC_R","8":"+3V3_A","9":"GND","10":"GND","11":"GND","12":"GND","13":"I2S_BCLK_DAC","14":"I2S_DOUT_DAC","15":"I2S_LRCLK_DAC","16":"GND","17":"DAC_XSMT","19":"GND","20":"+3V3"},
      datasheet="https://www.ti.com/lit/ds/symlink/pcm5102a.pdf", manufacturer="Texas Instruments", mpn="PCM5102APWR", unit_price="4.04"),
    C("U9", "TPA6132A2RTER", "03_local_audio", "Connector_Generic", "Conn_01x17",
      "Package_DFN_QFN:VQFN-16-1EP_3x3mm_P0.5mm_EP1.6x1.6mm_ThermalVias", 205, 95, 78, 68,
      nets={"1":"DAC_L","2":"DAC_L","3":"DAC_R","4":"DAC_R","5":"HP_R","8":"GND","10":"GND","13":"HP_EN","14":"+3V3_A","15":"GND","16":"HP_L","17":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tpa6132a2.pdf", manufacturer="Texas Instruments", mpn="TPA6132A2RTER", unit_price="1.10"),
    C("U10", "MAX98357AETE+T", "03_local_audio", "Audio", "MAX98357A",
      "Package_CSP:WLCSP-9_1.21x1.22mm_Layout3x3_P0.4mm", 300, 95, 92, 68,
      name_nets={"VDD":"VSYS_SW", "GND":"GND", "BCLK":"I2S_BCLK_SPK", "LRCLK":"I2S_LRCLK_SPK", "DIN":"I2S_DOUT_SPK", "SD_MODE":"SPK_EN", "GAIN_SLOT":"GND", "OUTP":"SPK_P", "OUTN":"SPK_N"},
      datasheet="https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf", manufacturer="Analog Devices", mpn="MAX98357AETE+T"),
    C("J5", "SJ-3523-SMT-TR", "03_local_audio", "Connector_Audio", "AudioJack3_Ground_Switch",
      "Connector_Audio:Jack_3.5mm_CUI_SJ-3523-SMT_Horizontal", 100, 175, 107, 66,
      nets={"T":"HP_L", "R":"HP_R", "S":"GND", "TN":"HP_DETECT", "RN":"HP_DETECT_BIAS", "G":"GND", "SN":"GND"},
      datasheet="https://www.sameskydevices.com/product/resource/sj-3523-smt-tr.pdf", manufacturer="Same Sky", mpn="SJ-3523-SMT-TR", unit_price="1.38"),
    C("J6", "CMS-28528N-L152", "03_local_audio", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 260, 175, 107, 77,
      nets={"1":"SPK_P", "2":"SPK_N"}, datasheet="https://www.sameskydevices.com/product/resource/cms-28528n-l152.pdf", manufacturer="Same Sky", mpn="CMS-28528N-L152", unit_price="4.76"),

    C("U11", "TCA9535PWR", "04_display_controls", "Connector_Generic", "Conn_01x24",
      "Package_SO:TSSOP-24_4.4x7.8mm_P0.65mm", 115, 100, 42, 74,
      nets={"1":"TCA_INT_N","2":"GND","3":"GND","4":"BTN_PREV_N","5":"BTN_PLAY_N","6":"BTN_NEXT_N","7":"BTN_VOL_DOWN_N","8":"BTN_VOL_UP_N","9":"SD_DETECT_N","10":"CHG_STAT","11":"FUEL_ALERT_N","12":"GND","13":"CHG_INT_N","14":"USB_INT_N","15":"PWR_GOOD_3V3","16":"PWR_GOOD_BT","17":"EXP_IN14","18":"EXP_IN15","19":"EXP_IN16","20":"EXP_IN17","21":"GND","22":"I2C_SCL","23":"I2C_SDA","24":"+3V3"},
      datasheet="https://www.ti.com/lit/ds/symlink/tca9535.pdf", manufacturer="Texas Instruments", mpn="TCA9535PWR"),
    C("J7", "Waveshare 24382 TFT", "04_display_controls", "Connector_Generic", "Conn_01x08",
      "Connector_JST:JST_GH_SM08B-GHS-TB_1x08-1MP_P1.25mm_Horizontal", 260, 85, 12, 32,
      nets={"1":"+3V3", "2":"GND", "3":"TFT_MOSI", "4":"TFT_SCLK", "5":"TFT_CS", "6":"TFT_DC", "7":"TFT_RST", "8":"TFT_BL_PWM"},
      datasheet="https://www.waveshare.com/wiki/1.69inch_LCD_Module", manufacturer="Waveshare", mpn="24382", unit_price="9.49"),
]

button_x = [95, 145, 195, 245, 295]
for index, (label, net) in enumerate([
    ("PREV", "BTN_PREV_N"), ("PLAY_PAUSE", "BTN_PLAY_N"), ("NEXT", "BTN_NEXT_N"),
    ("VOL_DOWN", "BTN_VOL_DOWN_N"), ("VOL_UP", "BTN_VOL_UP_N")
]):
    components.append(C(f"SW{index+1}", label, "04_display_controls", "Switch", "SW_Push",
                        "Button_Switch_SMD:SW_SPST_TL3342", button_x[index], 190, 25 + index*14, 88,
                        nets={"1": net, "2": "GND"}, description="Physical front-panel button"))

components += [
    C("U12", "BM83SM1-00TA", "05_bluetooth", "RF_Bluetooth", "Microchip_BM83",
      "RF_Module:Microchip_BM83", 165, 115, 99, 20,
      nets={"1":"I2S_DOUT_BT","2":"I2S_LRCLK_BT","3":"I2S_BCLK_BT","5":"BT_MCLK_OPTION","16":"GND","22":"+3V6_BT","23":"+3V6_BT","25":"+3V3","26":"BT_MFB","29":"BT_UART_TX","30":"BT_UART_RX","31":"BT_SERVICE_P34","32":"BT_LED","34":"BT_STATE","43":"BT_RST_N","46":"I2C_SCL","47":"I2C_SDA","49":"BT_WAKE","50":"GND","56":"GND","57":"GND"},
      datasheet="https://ww1.microchip.com/downloads/en/DeviceDoc/BM83-Bluetooth-Stereo-Audio-Module-Data-Sheet-DS70005402D.pdf", manufacturer="Microchip Technology", mpn="BM83SM1-00TA", unit_price="12.20"),
    C("SW6", "MAIN_POWER", "06_connectors_test", "Switch", "SW_SPDT",
      "Button_Switch_SMD:SW_SPDT_PCM12", 75, 70, 10, 70,
      nets={"1":"GND", "2":"PWR_SWITCH_ON", "3":"SYS"}, description="Low-current hardware load-switch control"),
    C("J8", "UART0_SERVICE", "06_connectors_test", "Connector_Generic", "Conn_01x06",
      "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical", 155, 70, 15, 57,
      nets={"1":"+3V3", "2":"GND", "3":"UART0_TX", "4":"UART0_RX", "5":"ESP_EN", "6":"BOOT_N"}, description="Bring-up header; not populated in sealed build"),
]

for index, net in enumerate(["VBUS5", "BAT", "SYS", "VSYS_SW", "+3V3", "+3V6_BT", "I2C_SDA", "I2C_SCL", "I2S_BCLK", "I2S_LRCLK", "I2S_DOUT", "GND"]):
    components.append(C(f"TP{index+1}", net, "06_connectors_test", "Connector", "TestPoint",
                        "TestPoint:TestPoint_Pad_D1.0mm", 70 + (index % 6)*48, 135 + (index // 6)*45,
                        25 + (index % 6)*16, 94 + (index // 6)*6, nets={"1":net}, description="Accessible engineering test point"))


# Source series resistors and essential bus pulls are real schematic/PCB parts.
passives = [
    ("R1", "5.1k", "01_power_charging", "USB_CC1", "GND", 25, 50),
    ("R2", "5.1k", "01_power_charging", "USB_CC2", "GND", 28, 50),
    ("R3", "4.7k", "01_power_charging", "I2C_SDA", "+3V3", 31, 50),
    ("R4", "4.7k", "01_power_charging", "I2C_SCL", "+3V3", 34, 50),
    ("R5", "22", "02_mcu_storage", "USB_DP", "USB_DP_MCU", 52, 28),
    ("R6", "22", "02_mcu_storage", "USB_DM", "USB_DM_MCU", 55, 28),
    ("R7", "33", "03_local_audio", "I2S_BCLK", "I2S_BCLK_DAC", 54, 75),
    ("R8", "33", "03_local_audio", "I2S_LRCLK", "I2S_LRCLK_DAC", 57, 75),
    ("R9", "33", "03_local_audio", "I2S_DOUT", "I2S_DOUT_DAC", 60, 75),
    ("R10", "33", "03_local_audio", "I2S_BCLK", "I2S_BCLK_SPK", 63, 75),
    ("R11", "33", "03_local_audio", "I2S_LRCLK", "I2S_LRCLK_SPK", 66, 75),
    ("R12", "33", "03_local_audio", "I2S_DOUT", "I2S_DOUT_SPK", 69, 75),
    ("R13", "33", "05_bluetooth", "I2S_BCLK", "I2S_BCLK_BT", 91, 54),
    ("R14", "33", "05_bluetooth", "I2S_LRCLK", "I2S_LRCLK_BT", 94, 54),
    ("R15", "33", "05_bluetooth", "I2S_DOUT", "I2S_DOUT_BT", 97, 54),
]
for idx, (ref, value, sheet, n1, n2, bx, by) in enumerate(passives):
    components.append(C(ref, value, sheet, "Device", "R", "Resistor_SMD:R_0603_1608Metric",
                        55 + (idx % 5)*55, 225 + (idx // 5)*18, bx, by, nets={"1":n1,"2":n2}, description="Reviewed-value placeholder; verify against final datasheet implementation"))

for idx, (net, value) in enumerate([("VBUS5","10u"),("BAT","10u"),("SYS","22u"),("VSYS_SW","22u"),("+3V3","22u"),("+3V6_BT","22u"),("+3V3_A","10u"),("REGN","1u")]):
    components.append(C(f"C{idx+1}", value, "01_power_charging", "Device", "C", "Capacitor_SMD:C_0603_1608Metric",
                        65 + (idx % 4)*65, 185 + (idx // 4)*22, 38 + idx*5.5, 50, nets={"1":net,"2":"GND"}, description="MLCC; voltage rating and effective capacitance require review"))


def normalized(name: str) -> str:
    return re.sub(r"[^A-Z0-9]+", "", name.upper())


def net_for_pin(comp: Component, pin: dict[str, object]) -> str | None:
    num = str(pin["number"])
    name = str(pin["name"])
    if num in comp.nets:
        return comp.nets[num]
    for key, net in comp.name_nets.items():
        if key.strip().upper() == name.strip().upper():
            return net
    candidates = [name, normalized(name)]
    for key, net in comp.name_nets.items():
        if normalized(key) in [normalized(c) for c in candidates]:
            return net
    n = normalized(name)
    if any(token in n for token in ("GND", "VSS", "PGND", "AGND", "DGND", "EP")):
        return "GND"
    if n in {"VCC", "VDD", "DVDD", "VDDIO", "3V3"}:
        return "+3V3"
    return None


def component_symbol(comp: Component, symbol_block: str, pins: list[dict[str, object]]) -> str:
    comp.symbol_uuid = uid(f"symbol/{comp.ref}")
    comp.sheet_uuid = SHEET_UUIDS[comp.sheet]
    path = f"/{ROOT_UUID}/{comp.sheet_uuid}"
    pin_rows = []
    for pin in pins:
        number = str(pin["number"])
        pin_rows.append(f'        (pin "{q(number)}" (uuid "{uid("pin/" + comp.ref + "/" + number)}"))')
    pin_lines = "\n".join(pin_rows)
    return f'''(symbol (lib_id "{comp.lib}:{comp.symbol}") (at {comp.x:.3f} {comp.y:.3f} {comp.rotation:.0f}) (unit 1) (in_bom yes) (on_board yes) (dnp no)
        (uuid "{comp.symbol_uuid}")
        (property "Reference" "{q(comp.ref)}" (at {comp.x:.3f} {comp.y-15:.3f} 0) (effects (font (size 1.27 1.27))))
        (property "Value" "{q(comp.value)}" (at {comp.x:.3f} {comp.y+15:.3f} 0) (effects (font (size 1.27 1.27))))
        (property "Footprint" "{q(comp.footprint)}" (at {comp.x:.3f} {comp.y:.3f} 0) (effects (font (size 1.27 1.27)) (hide yes)))
        (property "Datasheet" "{q(comp.datasheet)}" (at {comp.x:.3f} {comp.y:.3f} 0) (effects (font (size 1.27 1.27)) (hide yes)))
{pin_lines}
        (instances (project "{PROJECT}" (path "{path}" (reference "{comp.ref}") (unit 1)))))'''


def label_or_nc(comp: Component, pin: dict[str, object]) -> str:
    # All selected symbols use zero-degree placement.  Pin 'at' is the electrical endpoint.
    x = comp.x + float(pin["x"])
    y = comp.y + float(pin["y"])
    net = net_for_pin(comp, pin)
    number = str(pin["number"])
    if net:
        local_x, local_y = float(pin["x"]), float(pin["y"])
        if abs(local_x) >= abs(local_y):
            dx, dy = (-2.54 if local_x <= 0 else 2.54), 0.0
        else:
            dx, dy = 0.0, (-2.54 if local_y <= 0 else 2.54)
        lx, ly = x + dx, y + dy
        angle = 0 if dx else 90
        return (f'(wire (pts (xy {x:.3f} {y:.3f}) (xy {lx:.3f} {ly:.3f})) '
                f'(stroke (width 0) (type default)) (uuid "{uid("wire/" + comp.ref + "/" + number)}")) '
                f'(global_label "{q(net)}" (shape passive) (at {lx:.3f} {ly:.3f} {angle}) '
                f'(effects (font (size 0.9 0.9)) (justify left)) '
                f'(uuid "{uid("label/" + comp.ref + "/" + number)}"))')
    return f'(no_connect (at {x:.3f} {y:.3f}) (uuid "{uid("nc/" + comp.ref + "/" + number)}"))'


def build_schematics() -> None:
    blocks: dict[tuple[str, str], tuple[str, list[dict[str, object]]]] = {}
    for comp in components:
        key = (comp.lib, comp.symbol)
        if key not in blocks:
            blocks[key] = load_symbol(*key)
        comp.parsed_pins = blocks[key][1]

    sheet_entries: list[str] = []
    for page, (sheet, title) in enumerate(SHEETS, start=2):
        sid = SHEET_UUIDS[sheet]
        sheet_entries.append(f'''(sheet (at 35 {25 + (page-2)*34}) (size 95 24)
          (stroke (width 0) (type default)) (fill (color 0 0 0 0))
          (uuid "{sid}")
          (property "Sheetname" "{q(title)}" (at 35 {24 + (page-2)*34} 0) (effects (font (size 1.27 1.27)) (justify left bottom)))
          (property "Sheetfile" "{sheet}.kicad_sch" (at 35 {50 + (page-2)*34} 0) (effects (font (size 1.27 1.27)) (justify left top)))
          (instances (project "{PROJECT}" (path "/{ROOT_UUID}" (page "{page}")))))''')

        comps = [c for c in components if c.sheet == sheet]
        unique = []
        seen = set()
        for comp in comps:
            key = (comp.lib, comp.symbol)
            if key not in seen:
                seen.add(key)
                unique.append(blocks[key][0])
        symbols = [component_symbol(c, blocks[(c.lib,c.symbol)][0], c.parsed_pins) for c in comps]
        labels = [label_or_nc(c, p) for c in comps for p in c.parsed_pins]
        note = "AI-authored engineering reference. Verify pinout, support values, footprint orientation, lithium safety, RF keepout and assembly before fabrication."
        child = f'''(kicad_sch (version 20260306) (generator "nightwave_reference_generator")
          (uuid "{uid('file/'+sheet)}") (paper "A3")
          (lib_symbols {' '.join(unique)})
          (text "{q(title)}" (exclude_from_sim no) (at 25 18 0) (effects (font (size 2.5 2.5) (bold yes)) (justify left bottom)))
          (text "{q(note)}" (exclude_from_sim no) (at 25 25 0) (effects (font (size 1.0 1.0)) (justify left bottom)))
          {' '.join(symbols)}
          {' '.join(labels)}
          (embedded_fonts no))'''
        (OUT / f"{sheet}.kicad_sch").write_text(child, encoding="utf-8")

    root = f'''(kicad_sch (version 20260306) (generator "nightwave_reference_generator")
      (uuid "{ROOT_UUID}") (paper "A4") (lib_symbols)
      (text "NIGHTWAVE AI-AUTHORED DIGITAL REFERENCE" (exclude_from_sim no) (at 35 15 0) (effects (font (size 2.5 2.5) (bold yes)) (justify left bottom)))
      (text "Complete source package for independent engineering review; unbuilt and not approved for fabrication or lithium connection." (exclude_from_sim no) (at 35 21 0) (effects (font (size 1.1 1.1)) (justify left bottom)))
      {' '.join(sheet_entries)}
      (sheet_instances (path "/" (page "1"))) (embedded_fonts no))'''
    (OUT / f"{PROJECT}.kicad_sch").write_text(root, encoding="utf-8")


def footprint_parts(fp_id: str) -> tuple[Path, str]:
    lib, name = fp_id.split(":", 1)
    return FOOTPRINTS / f"{lib}.pretty", name


def add_edge(board: pcbnew.BOARD, a: tuple[float,float], b: tuple[float,float], layer=pcbnew.Edge_Cuts, width=0.05):
    shape = pcbnew.PCB_SHAPE(board)
    shape.SetShape(pcbnew.SHAPE_T_SEGMENT)
    shape.SetLayer(layer)
    shape.SetWidth(pcbnew.FromMM(width))
    shape.SetStart(pcbnew.VECTOR2I(pcbnew.FromMM(a[0]), pcbnew.FromMM(a[1])))
    shape.SetEnd(pcbnew.VECTOR2I(pcbnew.FromMM(b[0]), pcbnew.FromMM(b[1])))
    board.Add(shape)


def build_board() -> None:
    board = pcbnew.BOARD()
    board.SetCopperLayerCount(4)
    net_objects: dict[str, pcbnew.NETINFO_ITEM] = {}
    def get_net(name: str) -> pcbnew.NETINFO_ITEM:
        if name not in net_objects:
            obj = pcbnew.NETINFO_ITEM(board, name)
            board.Add(obj)
            net_objects[name] = obj
        return net_objects[name]

    board.SetTitleBlock(pcbnew.TITLE_BLOCK())
    tb = board.GetTitleBlock()
    tb.SetTitle("Nightwave AI-authored digital reference")
    tb.SetComment(0, "UNBUILT - INDEPENDENT ENGINEERING REVIEW REQUIRED BEFORE FABRICATION")

    for a,b in [((5,5),(115,5)),((115,5),(115,105)),((115,105),(5,105)),((5,105),(5,5))]:
        add_edge(board,a,b)

    # Antenna keepout outlines on documentation layer; copper-zone keepouts must be rechecked in GUI.
    for a,b in [((17,5),(47,5)),((47,5),(47,14)),((47,14),(17,14)),((17,14),(17,5)),
                ((95,5),(115,5)),((115,5),(115,25)),((115,25),(95,25)),((95,25),(95,5))]:
        add_edge(board,a,b,pcbnew.Dwgs_User,0.15)

    for comp in components:
        lib_path, fp_name = footprint_parts(comp.footprint)
        fp = pcbnew.FootprintLoad(str(lib_path), fp_name)
        if fp is None:
            raise FileNotFoundError(f"Cannot load footprint {comp.footprint}")
        fp.SetReference(comp.ref)
        fp.SetValue(comp.value)
        fp.SetFPID(pcbnew.LIB_ID(*comp.footprint.split(":",1)))
        fp.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(comp.board_x), pcbnew.FromMM(comp.board_y)))
        fp.SetOrientationDegrees(comp.rotation)
        path = pcbnew.KIID_PATH()
        for item_uuid in (ROOT_UUID, comp.sheet_uuid, comp.symbol_uuid):
            path.push_back(pcbnew.KIID(item_uuid))
        fp.SetPath(path)
        board.Add(fp)
        pin_by_num = {str(p["number"]): p for p in comp.parsed_pins}
        for pad in fp.Pads():
            num = pad.GetNumber()
            pin = pin_by_num.get(num)
            net_name = net_for_pin(comp, pin) if pin else None
            if not net_name:
                net_name = f"unconnected-({comp.ref}-Pad{num})"
            pad.SetNet(get_net(net_name))

    pcbnew.SaveBoard(str(OUT / f"{PROJECT}.kicad_pcb"), board)


def write_support_files() -> None:
    pro = {
        "board": {}, "boards": [], "cvpcb": {}, "erc": {}, "libraries": {},
        "meta": {"filename": f"{PROJECT}.kicad_pro", "version": 1},
        "net_settings": {"classes": [], "meta": {"version": 3}},
        "pcbnew": {}, "schematic": {}, "text_variables": {
            "DESIGN_STATUS": "AI-authored, unbuilt, independent review required"
        }
    }
    (OUT / f"{PROJECT}.kicad_pro").write_text(json.dumps(pro, indent=2), encoding="utf-8")
    (OUT / ".gitignore").write_text("*.kicad_prl\n*.lck\n~*.tmp\n.history/\n", encoding="utf-8")

    manifest = {
        "schema": 1,
        "status": "AI-authored digital reference; unbuilt; not approved for fabrication or lithium connection",
        "board": {"layers": 4, "outline_mm": [110,100], "stackup": "final fab stackup pending independent review"},
        "selected_architecture": {
            "mcu": "ESP32-S3-WROOM-1-N16R8", "bluetooth": "BM83SM1-00TA",
            "display": "Waveshare 24382", "input_expander": "TCA9535PWR",
            "battery_reference": "Adafruit 5035 protected 1S 10050 mAh plus external 10k NTC",
            "charger": "BQ25628ERYKR", "logic_regulator": "TPS63802DLAR",
            "bluetooth_regulator": "TPS63802DLAR", "load_switch": "TPS22965DSGR"
        },
        "components": [
            {"reference": c.ref, "value": c.value, "manufacturer": c.manufacturer, "mpn": c.mpn or c.value,
             "sheet": c.sheet, "footprint": c.footprint, "datasheet": c.datasheet,
             "pin_nets": {str(p["number"]): {"name": str(p["name"]), "net": (net_for_pin(c,p) or "NC")} for p in c.parsed_pins}}
            for c in components
        ],
        "review_gates": [
            "Independent electrical review of USB-C, BQ25628E, NTC, battery polarity and charge limits",
            "Manufacturer-footprint and pin-one review for every IC/module",
            "BM83 AT source firmware, I2S clock direction/rate and antenna keepout validation",
            "ESP32-S3 and BM83 RF keepout/layout review",
            "Display connector/cable pinout and backlight-current validation",
            "Complete manual PCB routing/return-path/SI review and clean final DRC",
            "Mechanical fit check against editable enclosure CAD before fabrication"
        ]
    }
    (OUT / "design-manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    with (OUT / "reference-bom.csv").open("w", newline="", encoding="utf-8") as handle:
        fields = ["Reference","Qty","Manufacturer","MPN","Value","Footprint","Datasheet","Unit_Price_USD","Supplier_URL","Status"]
        writer = csv.DictWriter(handle, fieldnames=fields); writer.writeheader()
        for c in components:
            writer.writerow({"Reference":c.ref,"Qty":1,"Manufacturer":c.manufacturer,"MPN":c.mpn or c.value,"Value":c.value,
                             "Footprint":c.footprint,"Datasheet":c.datasheet,"Unit_Price_USD":c.unit_price,"Supplier_URL":c.supplier_url,
                             "Status":"REFERENCE_REVIEW_REQUIRED"})

    readme = """# Nightwave KiCad reference design

This directory is a **complete AI-authored digital reference package for independent engineering review**. It is unbuilt, unmeasured, and **not approved for fabrication or lithium-cell connection**.

## What is here

- native KiCad 10 hierarchical schematic and four-layer PCB source;
- selected ESP32-S3, BM83, Waveshare TFT, TCA9535, local DAC/headphone/speaker, USB-C, charger, dual regulated rails, protected battery and service/test interfaces;
- exact logical net/GPIO assignments generated from the same source as the PCB;
- BOM and machine-readable design manifest;
- `manufacturing/` review exports generated by KiCad CLI and prominently marked **DO NOT FABRICATE**;
- `reports/` ERC/DRC/netlist and rendered-review evidence.

## Critical stop

Do not order this board yet. An independent reviewer must close every item in `REVIEW_CHECKLIST.md`, complete placement/routing, then the reviewed layout must produce a clean ERC/DRC with no waived safety or RF faults. The board contains footprints and the full ratsnest so tomorrow's KiCad work starts from a real source package, not a blank canvas.

## Opening tomorrow

Open `Nightwave-Reference.kicad_pro`. Start with the root schematic, then inspect the PCB. The existing `nightwave-starter` project is intentionally separate and unchanged.
"""
    (OUT / "README.md").write_text(readme, encoding="utf-8")

    checklist = """# Independent review checklist

- [ ] Confirm exact USB4105 suffix, mating/cutout dimensions, shield strategy and CC implementation.
- [ ] Confirm BQ25628E pinout, input-current detection, ILIM, charge current/voltage, safety timer, TS network and thermal limits from the current datasheet.
- [ ] Confirm Adafruit 5035 connector polarity, 3 A limits, mechanical retention, replaceability and external 10 kOhm NTC attachment.
- [ ] Confirm TPS63802 inductors, feedback dividers, effective capacitance, peak currents and thermal copper for both rails.
- [ ] Confirm ESP32-S3 N16R8 GPIO availability, straps, USB, PSRAM exclusions and antenna keepout.
- [ ] Confirm BM83SM1-00TA 50-pad land pattern, supply sequencing, AT source firmware, UART/MFB/service pins, I2S clock direction, sample rates and antenna keepout.
- [ ] Confirm Waveshare 24382 cable connector, pin order, 3.3 V levels, backlight current/PWM and enclosure orientation.
- [ ] Confirm TCA9535 address 0x20, external pulls for every input, interrupt polarity and unused-input treatment.
- [ ] Confirm PCM5102A/TPA6132A2/MAX98357A reference circuits, gain, pop/click behavior, SJ-3523 switch-based headphone detection and BTL speaker isolation.
- [ ] Re-place and manually route the board for switch-mode current loops, controlled USB pair, uninterrupted RF keepouts and continuous return paths.
- [ ] Re-run ERC, DRC, schematic parity, fabrication viewer, assembly drawing and enclosure interference checks.
- [ ] Obtain named independent reviewer sign-off before ordering or connecting a cell.
"""
    (OUT / "REVIEW_CHECKLIST.md").write_text(checklist, encoding="utf-8")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.iterdir():
        if old.is_file():
            old.unlink()
        elif old.name in {"manufacturing", "reports"}:
            shutil.rmtree(old, ignore_errors=True)
    for comp in components:
        comp.x = round(comp.x / 1.27) * 1.27
        comp.y = round(comp.y / 1.27) * 1.27
    build_schematics()
    build_board()
    write_support_files()
    print(f"Generated {len(components)} components in {OUT}")


if __name__ == "__main__":
    main()
