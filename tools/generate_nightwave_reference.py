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
import argparse
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
FP_CACHE = {}
SYMBOL_CACHE = {}


def load_footprint(fp_id):
    if fp_id not in FP_CACHE:
        lib_path,fp_name=footprint_parts(fp_id)
        fp=pcbnew.FootprintLoad(str(lib_path),fp_name)
        if fp is None:
            raise FileNotFoundError(fp_id)
        FP_CACHE[fp_id]=fp
    return pcbnew.Cast_to_FOOTPRINT(FP_CACHE[fp_id].Duplicate(False))


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
    if (lib,name) in SYMBOL_CACHE:
        return SYMBOL_CACHE[(lib,name)]
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
    SYMBOL_CACHE[(lib,name)]=(embedded,pins)
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
    pin_names: dict[str, str] = field(default_factory=dict)


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
      "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal", 45, 65, 8.675, 58, rotation=270,
      name_nets={"VBUS":"VBUS5", "GND":"GND", "SHIELD":"CHASSIS", "CC1":"USB_CC1", "CC2":"USB_CC2", "D+":"USB_DP", "D-":"USB_DM"},
      datasheet="https://gct.co/files/specs/usb4105-spec.pdf", manufacturer="GCT", mpn="USB4105-GF-A"),
    # A fixed 5 V sink does not require a Type-C port controller.  The two
    # 5.1 kOhm Rd resistors below advertise a USB-C sink and remove an entire
    # failure-prone CC/I2C dependency from the battery charger path.
    C("U3", "BQ25628ERYKR", "01_power_charging", "Connector_Generic", "Conn_01x18",
      "Nightwave:TI_RYK0018A", 175, 72, 40, 42,
      nets={"1":"BTST", "2":"REGN", "3":"PGOOD_CHG", "4":"ILIM", "5":"TS_BIAS", "6":"TS", "8":"BAT", "9":"SYS", "10":"CHG_STAT", "11":"CHG_INT_N", "12":"I2C_SDA", "13":"I2C_SCL", "14":"GND", "15":"GND", "16":"SW_CHG", "17":"PMID", "18":"VBUS5"},
      datasheet="https://www.ti.com/lit/ds/symlink/bq25628e.pdf", manufacturer="Texas Instruments", mpn="BQ25628ERYKR"),
    C("U4", "TPS22965DSGR", "01_power_charging", "Connector_Generic", "Conn_01x09",
      "Package_SON:Texas_DSG0008A_WSON-8-1EP_2x2mm_P0.5mm_EP0.9x1.6mm_ThermalVias", 240, 55, 53, 42,
      nets={"1":"SYS", "2":"SYS", "3":"PWR_SWITCH_ON", "4":"SYS", "5":"GND", "7":"VSYS_SW", "8":"VSYS_SW", "9":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps22965.pdf", manufacturer="Texas Instruments", mpn="TPS22965DSGR"),
    C("U5", "TPS63802DLAR", "01_power_charging", "Connector_Generic", "Conn_01x10",
      "Nightwave:TI_DLA0010A", 295, 55, 65, 42,
      nets={"1":"VSYS_SW", "2":"GND", "3":"GND", "4":"FB_3V3", "5":"PWR_GOOD_3V3", "6":"+3V3", "7":"SW2_3V3", "8":"GND", "9":"SW1_3V3", "10":"VSYS_SW"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps63802.pdf", manufacturer="Texas Instruments", mpn="TPS63802DLAR"),
    C("U6", "TPS63802DLAR", "01_power_charging", "Connector_Generic", "Conn_01x10",
      "Nightwave:TI_DLA0010A", 350, 55, 76, 42,
      nets={"1":"VSYS_SW", "2":"GND", "3":"GND", "4":"FB_BT", "5":"PWR_GOOD_BT", "6":"+3V6_BT", "7":"SW2_BT", "8":"GND", "9":"SW1_BT", "10":"VSYS_SW"},
      datasheet="https://www.ti.com/lit/ds/symlink/tps63802.pdf", manufacturer="Texas Instruments", mpn="TPS63802DLAR"),
    C("U7", "MAX17048G+T10", "01_power_charging", "Connector_Generic", "Conn_01x09",
      "Package_DFN_QFN:TDFN-8-1EP_2x2mm_P0.5mm_EP0.8x1.2mm", 240, 125, 64, 56,
      nets={"1":"GND", "2":"BAT", "3":"BAT", "4":"GND", "5":"FUEL_ALERT_N", "6":"GND", "7":"I2C_SCL", "8":"I2C_SDA", "9":"GND"},
      datasheet="https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf", manufacturer="Analog Devices", mpn="MAX17048G+T10"),
    C("J2", "BATTERY_CONNECTOR", "01_power_charging", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 320, 125, 88, 43,
      nets={"1":"BAT", "2":"GND"}, manufacturer="JST", mpn="S2B-PH-K-S", datasheet="https://www.jst-mfg.com/product/pdf/eng/ePH.pdf", description="PCB battery header; PH family 2 A rating with AWG24; pack polarity must be verified"),
    C("J3", "EXTERNAL_10K_NTC", "01_power_charging", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 365, 125, 88, 54,
      nets={"1":"TS", "2":"GND"}, manufacturer="JST", mpn="S2B-PH-K-S", datasheet="https://www.jst-mfg.com/product/pdf/eng/ePH.pdf", description="PCB NTC header; off-board 10 kOhm NTC physically retained against battery pack"),

    C("U1", "ESP32-S3-WROOM-1-N16R8", "02_mcu_storage", "Connector_Generic", "Conn_01x41",
      "RF_Module:ESP32-S3-WROOM-1", 130, 115, 32, 18,
      nets={"1":"GND","2":"+3V3","3":"ESP_EN","4":"BT_MFB","5":"I2S_BCLK","6":"I2S_LRCLK","7":"I2S_DOUT","8":"BT_RST_N","9":"HP_DETECT","10":"SPK_EN","11":"DAC_XSMT","12":"I2C_SDA","13":"USB_DM_MCU","14":"USB_DP_MCU","17":"I2C_SCL","18":"BT_WAKE","19":"SD_CMD","20":"SD_CLK","21":"SD_D0","22":"TCA_INT_N","23":"BT_MCLK_OPTION","24":"TFT_MOSI","25":"TFT_SCLK","27":"BOOT_N","31":"TFT_BL_PWM","32":"HP_EN","33":"TFT_RST","34":"TFT_DC","35":"TFT_CS","36":"UART0_RX","37":"UART0_TX","38":"BT_UART_RX","39":"BT_UART_TX","40":"GND","41":"GND"},
      datasheet="https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf", manufacturer="Espressif Systems", mpn="ESP32-S3-WROOM-1-N16R8", unit_price="6.76"),
    C("J4", "Molex 104031-0811", "02_mcu_storage", "Connector", "Micro_SD_Card",
      "Connector_Card:microSD_HC_Molex_104031-0811", 265, 95, 62, 18,
      nets={"2":"SD_DAT3","3":"SD_CMD","4":"+3V3","5":"SD_CLK","6":"GND","7":"SD_D0","9":"SD_DETECT_N","10":"GND","SH":"GND"},
      datasheet="https://www.molex.com/en-us/products/part-detail/1040310811", manufacturer="Molex", mpn="104031-0811", unit_price="2.21"),

    C("U8", "PCM5102APWR", "03_local_audio", "Connector_Generic", "Conn_01x20",
      "Package_SO:Texas_PW0020A_TSSOP-20_4.4x6.5mm_P0.65mm", 95, 95, 62, 68,
      nets={"1":"+3V3_A","2":"PCM_CP_P","3":"GND","4":"PCM_CP_M","5":"PCM_VNEG","6":"DAC_L","7":"DAC_R","8":"+3V3_A","9":"GND","10":"GND","11":"GND","12":"GND","13":"I2S_BCLK_DAC","14":"I2S_DOUT_DAC","15":"I2S_LRCLK_DAC","16":"GND","17":"DAC_XSMT","18":"PCM_LDOO","19":"GND","20":"+3V3"},
      datasheet="https://www.ti.com/lit/ds/symlink/pcm5102a.pdf", manufacturer="Texas Instruments", mpn="PCM5102APWR", unit_price="4.04"),
    C("U9", "TPA6132A2RTER", "03_local_audio", "Connector_Generic", "Conn_01x17",
      "Package_DFN_QFN:VQFN-16-1EP_3x3mm_P0.5mm_EP1.6x1.6mm_ThermalVias", 205, 95, 78, 68,
      nets={"1":"HP_IN_L","2":"GND","3":"GND","4":"HP_IN_R","5":"HP_R","6":"GND","7":"GND","8":"HP_VSS","9":"HP_CP_N","10":"GND","11":"HP_CP_P","12":"HP_VDD","13":"HP_EN","14":"+3V3_A","15":"GND","16":"HP_L","17":"GND"},
      datasheet="https://www.ti.com/lit/ds/symlink/tpa6132a2.pdf", manufacturer="Texas Instruments", mpn="TPA6132A2RTER", unit_price="1.10"),
    C("U10", "MAX98357AETE+T", "03_local_audio", "Audio", "MAX98357A",
      "Package_DFN_QFN:TQFN-16-1EP_3x3mm_P0.5mm_EP1.23x1.23mm", 300, 95, 92, 68,
      nets={"17":"GND"},
      name_nets={"VDD":"VSYS_SW", "GND":"GND", "BCLK":"I2S_BCLK_SPK", "LRCLK":"I2S_LRCLK_SPK", "DIN":"I2S_DOUT_SPK", "SD_MODE":"SPK_SD_MODE", "GAIN_SLOT":"GND", "OUTP":"SPK_P", "OUTN":"SPK_N"},
      datasheet="https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf", manufacturer="Analog Devices", mpn="MAX98357AETE+T"),
    C("J5", "SJ3-350153AG-TR", "03_local_audio", "Connector_Generic", "Conn_01x06",
      "Nightwave:SameSky_SJ3-350153AG", 100, 175, 112.8, 66, rotation=270,
      nets={"1":"GND", "2":"HP_L", "3":"HP_R", "4":"GND", "5":"GND", "6":"HP_DETECT", "MS":"GND"},
      datasheet="https://www.sameskydevices.com/product/resource/sj3-35015x.pdf", manufacturer="Same Sky", mpn="SJ3-350153AG-TR"),
    C("J6", "BTL_SPEAKER_CONNECTOR", "03_local_audio", "Connector_Generic", "Conn_01x02",
      "Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal", 260, 175, 107, 77,
      nets={"1":"SPK_P", "2":"SPK_N"}, manufacturer="JST", mpn="S2B-PH-K-S", datasheet="https://www.jst-mfg.com/product/pdf/eng/ePH.pdf", description="Floating BTL speaker connector: neither pin is ground"),

    C("U11", "TCA9535PWR", "04_display_controls", "Connector_Generic", "Conn_01x24",
      "Package_SO:TSSOP-24_4.4x7.8mm_P0.65mm", 115, 100, 42, 74,
      nets={"1":"TCA_INT_N","2":"GND","3":"GND","4":"BTN_PREV_N","5":"BTN_PLAY_N","6":"BTN_NEXT_N","7":"BTN_VOL_DOWN_N","8":"BTN_VOL_UP_N","9":"SD_DETECT_N","10":"CHG_STAT","11":"FUEL_ALERT_N","12":"GND","13":"CHG_INT_N","14":"PGOOD_CHG","15":"PWR_GOOD_3V3","16":"PWR_GOOD_BT","17":"EXP_IN14","18":"EXP_IN15","19":"EXP_IN16","20":"EXP_IN17","21":"GND","22":"I2C_SCL","23":"I2C_SDA","24":"+3V3"},
      datasheet="https://www.ti.com/lit/ds/symlink/tca9535.pdf", manufacturer="Texas Instruments", mpn="TCA9535PWR"),
    C("J7", "TFT_24382_HARNESS", "04_display_controls", "Connector_Generic", "Conn_01x08",
      "Connector_JST:JST_GH_SM08B-GHS-TB_1x08-1MP_P1.25mm_Horizontal", 260, 85, 12, 32,
      nets={"1":"+3V3", "2":"GND", "3":"TFT_MOSI", "4":"TFT_SCLK", "5":"TFT_CS", "6":"TFT_DC", "7":"TFT_RST", "8":"TFT_BL_PWM", "MP":"GND"},
      datasheet="https://www.jst-mfg.com/product/pdf/eng/eGH.pdf", manufacturer="JST", mpn="SM08B-GHS-TB(LF)(SN)", description="Board harness header, not the TFT module; mating GHR-08V-S / SSHL-002T-P0.2"),
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
      "Nightwave:Microchip_BM83_GroundLands", 165, 115, 99, 20,
      nets={"1":"I2S_DOUT_BT","2":"I2S_LRCLK_BT","3":"I2S_BCLK_BT","5":"BT_MCLK_OPTION","16":"GND","23":"+3V6_BT","26":"BT_MFB","29":"BT_UART_TX","30":"BT_UART_RX","31":"BT_SERVICE_P34","32":"BT_LED","34":"BT_STATE","43":"BT_RST_N","49":"BT_WAKE","50":"GND"},
      datasheet="https://ww1.microchip.com/downloads/en/DeviceDoc/BM83-Bluetooth-Stereo-Audio-Module-Data-Sheet-DS70005402D.pdf", manufacturer="Microchip Technology", mpn="BM83SM1-00TA", unit_price="12.20"),
    C("SW6", "MAIN_POWER", "06_connectors_test", "Switch", "SW_SPDT",
      "Button_Switch_SMD:SW_SPDT_PCM12", 75, 70, 10, 70,
      nets={"1":"GND", "2":"PWR_SWITCH_ON", "3":"SYS"}, description="Low-current hardware load-switch control"),
    C("J8", "UART0_SERVICE", "06_connectors_test", "Connector_Generic", "Conn_01x06",
      "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical", 155, 70, 15, 57,
      nets={"1":"+3V3", "2":"GND", "3":"UART0_TX", "4":"UART0_RX", "5":"ESP_EN", "6":"BOOT_N"}, description="Bring-up header; not populated in sealed build"),
]

for index, net in enumerate(["VBUS5", "BAT", "SYS", "VSYS_SW", "+3V3", "+3V6_BT", "I2C_SDA", "I2C_SCL", "I2S_BCLK", "I2S_LRCLK", "I2S_DOUT", "GND", "BT_SERVICE_P34", "BT_LED", "BT_STATE", "HP_DETECT"]):
    components.append(C(f"TP{index+1}", net, "06_connectors_test", "Connector", "TestPoint",
                        "TestPoint:TestPoint_Pad_D1.0mm", 70 + (index % 6)*48, 135 + (index // 6)*45,
                        25 + (index % 6)*16, 94 + (index // 6)*3.5, nets={"1":net}, description="Accessible engineering test point"))


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

for idx, (net, value) in enumerate([("VBUS5","10u"),("BAT","10u"),("SYS","22u"),("VSYS_SW","22u"),("+3V3","22u"),("+3V6_BT","22u"),("+3V3_A","10u"),("REGN","4.7u")]):
    components.append(C(f"C{idx+1}", value, "01_power_charging", "Device", "C", "Capacitor_SMD:C_0603_1608Metric",
                        65 + (idx % 4)*65, 185 + (idx // 4)*22, 38 + idx*5.5, 50, nets={"1":net,"2":"GND"}, description="MLCC; voltage rating and effective capacitance require review"))

# Datasheet-derived support circuits.  Exact footprints below are generated from
# the manufacturer land patterns, including the split lands on the RYK charger.
def add_passive(ref, value, kind, sheet, n1, n2, bx, by, footprint=None):
    components.append(C(ref, value, sheet, "Device", kind,
                        footprint or {"R":"Resistor_SMD:R_0603_1608Metric", "C":"Capacitor_SMD:C_0805_2012Metric", "L":"Nightwave:Coilcraft_XFL4015"}[kind],
                        0, 0, bx, by, nets={"1":n1,"2":n2}))

for args in [
    ("L1","0.47uH XFL4015-471MEC","L","01_power_charging","SW1_3V3","SW2_3V3",65,37),
    ("L2","0.47uH XFL4015-471MEC","L","01_power_charging","SW1_BT","SW2_BT",76,37),
    ("L3","0.7uH XFL4015-701MEC","L","01_power_charging","SW_CHG","SYS",40,37),
    ("R16","511k","R","01_power_charging","+3V3","FB_3V3",63,46),
    ("R17","91k","R","01_power_charging","FB_3V3","GND",67,46),
    ("R18","562k","R","01_power_charging","+3V6_BT","FB_BT",74,46),
    ("R19","91k","R","01_power_charging","FB_BT","GND",78,46),
    ("R20","5.49k","R","01_power_charging","ILIM","GND",36,46),
    ("R21","5.23k","R","01_power_charging","TS_BIAS","TS",42,46),
    ("R22","30.1k","R","01_power_charging","TS","GND",46,46),
    ("R23","0","R","01_power_charging","CHASSIS","GND",13,48),
    ("R24","10k","R","02_mcu_storage","ESP_EN","+3V3",22,31),
    ("R25","10k","R","02_mcu_storage","BOOT_N","+3V3",26,31),
    ("R26","10k","R","04_display_controls","HP_DETECT","+3V3",80,60),
    ("R27","10k","R","03_local_audio","SPK_SD_MODE","GND",92,74),
    ("R28","10k","R","03_local_audio","HP_EN","GND",78,74),
    ("R29","10k","R","03_local_audio","DAC_XSMT","GND",62,74),
    ("R30","0","R","03_local_audio","+3V3","+3V3_A",58,62),
]: add_passive(*args)

for idx, net in enumerate(["BTN_PREV_N","BTN_PLAY_N","BTN_NEXT_N","BTN_VOL_DOWN_N","BTN_VOL_UP_N","SD_DETECT_N","CHG_STAT","FUEL_ALERT_N","CHG_INT_N","PGOOD_CHG","PWR_GOOD_3V3","PWR_GOOD_BT","TCA_INT_N","EXP_IN14","EXP_IN15","EXP_IN16","EXP_IN17"]):
    add_passive(f"R{31+idx}","10k","R","04_display_controls",net,"+3V3",30+(idx%6)*4,79+(idx//6)*2.5)

for idx,(value,sheet,n1,n2,bx,by) in enumerate([
    ("47n/10V","01_power_charging","BTST","SW_CHG",37,40),
    ("10u/25V","01_power_charging","PMID","GND",43,40),
    ("10u/10V","01_power_charging","VSYS_SW","GND",62,40),
    ("22u/10V","01_power_charging","+3V3","GND",68,40),
    ("10u/10V","01_power_charging","VSYS_SW","GND",73,40),
    ("22u/10V","01_power_charging","+3V6_BT","GND",79,40),
    ("100n/10V","01_power_charging","BAT","GND",62,56),
    ("100n/10V","02_mcu_storage","+3V3","GND",21,26),
    ("10u/10V","02_mcu_storage","+3V3","GND",24,26),
    ("1u/10V","02_mcu_storage","ESP_EN","GND",22,34),
    ("100n/10V","02_mcu_storage","+3V3","GND",59,29),
    ("10u/10V","02_mcu_storage","+3V3","GND",63,29),
    ("2.2u/10V","03_local_audio","PCM_CP_P","PCM_CP_M",59,66),
    ("2.2u/10V","03_local_audio","PCM_VNEG","GND",59,69),
    ("100n/10V","03_local_audio","PCM_LDOO","GND",66,66),
    ("100n/10V","03_local_audio","+3V3_A","GND",66,69),
    ("100n/10V","03_local_audio","+3V3","GND",65,63),
    ("1u/10V","03_local_audio","HP_CP_P","HP_CP_N",75,66),
    ("1u/10V","03_local_audio","HP_VSS","GND",75,69),
    ("2.2u/10V","03_local_audio","HP_VDD","GND",81,66),
    ("1u/10V","03_local_audio","DAC_L","HP_IN_L",71,62),
    ("1u/10V","03_local_audio","DAC_R","HP_IN_R",75,62),
    ("100n/10V","03_local_audio","+3V3_A","GND",81,69),
    ("100n/10V","03_local_audio","VSYS_SW","GND",89,65),
    ("10u/10V","03_local_audio","VSYS_SW","GND",95,65),
    ("100n/10V","04_display_controls","+3V3","GND",46,74),
    ("10u/10V","04_display_controls","+3V3","GND",17,32),
    ("10u/10V","05_bluetooth","+3V6_BT","GND",92,36),
    ("100n/10V","05_bluetooth","+3V6_BT","GND",96,36),
],start=9): add_passive(f"C{idx}",value,"C",sheet,n1,n2,bx,by)

byref={c.ref:c for c in components}
for args in [
    ("R48","10k","R","02_mcu_storage","SD_CMD","+3V3",61,31),
    ("R49","10k","R","02_mcu_storage","SD_D0","+3V3",65,31),
    ("R50","10k","R","02_mcu_storage","SD_DAT3","+3V3",69,31),
    # 3.3 V * (10k || internal 100k) / (47k + 10k || 100k)
    # = 0.53 V, between worst-case B0 and B1: averaged stereo. GPIO low
    # disables the amp. Validate VOH and resistor tolerances in review.
    ("R51","47k","R","03_local_audio","SPK_EN","SPK_SD_MODE",95,74),
]: add_passive(*args)
for ref,names in {
    "U1":["GND","3V3","EN","GPIO4","GPIO5","GPIO6","GPIO7","GPIO15","GPIO16","GPIO17","GPIO18","GPIO8","GPIO19_USB_D-","GPIO20_USB_D+","GPIO3","GPIO46","GPIO9","GPIO10","GPIO11","GPIO12","GPIO13","GPIO14","GPIO21","GPIO47","GPIO48","GPIO45","GPIO0","GPIO35_PSRAM","GPIO36_PSRAM","GPIO37_PSRAM","GPIO38","GPIO39","GPIO40","GPIO41","GPIO42","RXD0","TXD0","GPIO2","GPIO1","GND","EP_GND"],
    "U11":["INT","A1","A2","P0_0","P0_1","P0_2","P0_3","P0_4","P0_5","P0_6","P0_7","GND","P1_0","P1_1","P1_2","P1_3","P1_4","P1_5","P1_6","P1_7","A0","SCL","SDA","VCC"],
    "U3":["BTST","REGN","PG","ILIM","TS_BIAS","TS","QON","BAT","SYS","STAT","INT","SDA","SCL","CE","GND","SW","PMID","VBUS"],
    "U4":["VIN","VIN","ON","VBIAS","GND","CT","VOUT","VOUT","EP"],
    "U5":["EN","MODE","AGND","FB","PG","VOUT","L2","GND","L1","VIN"],
    "U6":["EN","MODE","AGND","FB","PG","VOUT","L2","GND","L1","VIN"],
    "U7":["CTG","CELL","VDD","GND","ALRT","QSTRT","SCL","SDA","EP"],
    "U8":["CPVDD","CAPP","CPGND","CAPM","VNEG","OUTL","OUTR","AVDD","AGND","DEMP","FLT","SCK","BCK","DIN","LRCK","FMT","XSMT","LDOO","DGND","DVDD"],
    "U9":["INL-","INL+","INR+","INR-","OUTR","G0","G1","HPVSS","CPN","PGND","CPP","HPVDD","EN","VDD","SGND","OUTL","EP"],
}.items(): byref[ref].pin_names={str(i):n for i,n in enumerate(names,1)}

# Keep switch-mode parts in compact clusters instead of allowing an area-sort
# legalizer to move the IC away from its own capacitors/inductor.
placement_anchors={
    "U3":(40,55),"L3":(40,50.5),"C1":(37,54),"C2":(43,58),
    "C3":(45,54),"C8":(37,57),"C9":(38.5,52.7),"C10":(36,51.5),
    "R20":(40,60),"R21":(44,60),"R22":(46,60),
    "U4":(52,55),"C4":(55,58),
    "U5":(65,55),"L1":(65,50.5),"C11":(62,54),"C5":(68,54),
    "C12":(68,58),"R16":(64,58),"R17":(64,60),
    "U6":(78,55),"L2":(78,50.5),"C13":(75,54),"C6":(81,54),
    "C14":(81,58),"R18":(77,58),"R19":(77,60),"U7":(52,63),"C15":(52,60),
}
for comp in components:
    if comp.ref in placement_anchors:
        comp.board_x,comp.board_y=placement_anchors[comp.ref]
    if comp.ref in {'L1','L2','L3'}:
        comp.manufacturer='Coilcraft'
        comp.mpn='XFL4015-701MEC' if comp.ref=='L3' else 'XFL4015-471MEC'
        comp.datasheet='https://www.coilcraft.com/getmedia/84927b8b-f089-421b-a7f4-a0fa23afe908/xfl4015.pdf'


def normalized(name: str) -> str:
    return re.sub(r"[^A-Z0-9]+", "", name.upper())


def custom_symbol_for_component(comp: Component) -> tuple[str, list[dict[str, object]]]:
    """Create a flattened symbol whose pin numbers exactly match the footprint.

    KiCad's inherited library symbols are intentionally not embedded here: an
    embedded child without its base silently loses graphics/pins and creates
    false schematic/PCB parity.  This deterministic block is review-friendly
    and makes the native ERC/netlist/parity tools authoritative.
    """
    original_names: dict[str, str] = {}
    try:
        _, original_pins = load_symbol(comp.lib, comp.symbol)
        original_names = {str(p["number"]): str(p["name"]) for p in original_pins}
    except (ValueError, FileNotFoundError):
        pass
    fp = load_footprint(comp.footprint)
    if fp is None:
        raise FileNotFoundError(f"Cannot load footprint {comp.footprint}")
    numbers = sorted({pad.GetNumber() for pad in fp.Pads() if pad.GetNumber()},
                     key=lambda value: (not value.isdigit(), int(value) if value.isdigit() else value))
    count = len(numbers)
    left_count = (count + 1) // 2
    height = max(10.16, (left_count + 1) * 2.54)
    half_h = height / 2
    half_w = 20.32 if count > 12 else 7.62
    pins: list[dict[str, object]] = []
    pin_exprs: list[str] = []
    for index, number in enumerate(numbers):
        left = index < left_count
        row = index if left else index - left_count
        x = -half_w - 2.54 if left else half_w + 2.54
        y = -half_h + 2.54 + row * 2.54
        angle = 0 if left else 180
        net = comp.nets.get(number)
        name = comp.pin_names.get(number) or original_names.get(number) or net or f"PAD_{number}"
        pins.append({"number": number, "name": name, "x": x, "y": y, "angle": angle})
        pin_exprs.append(f'''(pin passive line (at {x:.3f} {y:.3f} {angle}) (length 2.54)
          (name "{q(name)}" (effects (font (size 0.85 0.85))))
          (number "{q(number)}" (effects (font (size 0.85 0.85)))))''')
    symbol_name = f"Nightwave:{comp.ref}"
    hide_names = ' (hide yes)' if comp.ref.startswith(('R','C','L','TP','SW')) else ''
    block = f'''(symbol "{symbol_name}"
      (pin_names (offset 1.0){hide_names}) (exclude_from_sim no) (in_bom yes) (on_board yes)
      (property "Reference" "{q(re.sub(r'[0-9]+$', '', comp.ref) or comp.ref)}" (at 0 {-half_h-2.54:.3f} 0) (effects (font (size 1.27 1.27))))
      (property "Value" "{q(comp.value)}" (at 0 {half_h+2.54:.3f} 0) (effects (font (size 1.0 1.0))))
      (property "Footprint" "{q(comp.footprint)}" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))
      (property "Datasheet" "{q(comp.datasheet)}" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))
      (symbol "{q(comp.ref)}_0_1"
        (rectangle (start {-half_w:.3f} {-half_h:.3f}) (end {half_w:.3f} {half_h:.3f})
          (stroke (width 0.254) (type default)) (fill (type background))))
      (symbol "{q(comp.ref)}_1_1" {' '.join(pin_exprs)}))'''
    comp.lib = "Nightwave"
    comp.symbol = comp.ref
    return block, pins


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
    # Supply rails must be explicit. In particular BM83 VDD_IO is an internal
    # supply/output, not an invitation to connect the external 3.3 V rail.
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
    half_h=max(10.16, (((len(pins)+1)//2)+1)*2.54)/2
    return f'''(symbol (lib_id "{comp.lib}:{comp.symbol}") (at {comp.x:.3f} {comp.y:.3f} 0) (unit 1) (in_bom yes) (on_board yes) (dnp no)
        (uuid "{comp.symbol_uuid}")
        (property "Reference" "{q(comp.ref)}" (at {comp.x:.3f} {comp.y-half_h-3:.3f} 0) (effects (font (size 1.27 1.27))))
        (property "Value" "{q(comp.value)}" (at {comp.x:.3f} {comp.y+half_h+3:.3f} 0) (effects (font (size 1.0 1.0))))
        (property "Footprint" "{q(comp.footprint)}" (at {comp.x:.3f} {comp.y:.3f} 0) (effects (font (size 1.27 1.27)) (hide yes)))
        (property "Datasheet" "{q(comp.datasheet)}" (at {comp.x:.3f} {comp.y:.3f} 0) (effects (font (size 1.27 1.27)) (hide yes)))
{pin_lines}
        (instances (project "{PROJECT}" (path "{path}" (reference "{comp.ref}") (unit 1)))))'''


def label_or_nc(comp: Component, pin: dict[str, object]) -> str:
    # All selected symbols use zero-degree placement.  Pin 'at' is the electrical endpoint.
    x = comp.x + float(pin["x"])
    # Library-symbol Y coordinates are Cartesian (positive up); sheet Y is
    # positive down, so placement mirrors local Y at rotation zero.
    y = comp.y - float(pin["y"])
    net = net_for_pin(comp, pin)
    number = str(pin["number"])
    if net:
        local_x = float(pin["x"])
        # Flattened reference symbols place every electrical pin on a left or
        # right edge. Route labels horizontally outward; using coordinate
        # magnitude here would turn lower pins vertically and short neighbors.
        dx, dy = (-7.62 if local_x <= 0 else 7.62), 0.0
        lx, ly = x + dx, y + dy
        angle = 0 if dx < 0 else 180
        return (f'(wire (pts (xy {x:.3f} {y:.3f}) (xy {lx:.3f} {ly:.3f})) '
                f'(stroke (width 0) (type default)) (uuid "{uid("wire/" + comp.ref + "/" + number)}")) '
                f'(global_label "{q(net)}" (shape passive) (at {lx:.3f} {ly:.3f} {angle}) '
                f'(effects (font (size 0.9 0.9)) (justify left)) '
                f'(uuid "{uid("label/" + comp.ref + "/" + number)}"))')
    return f'(no_connect (at {x:.3f} {y:.3f}) (uuid "{uid("nc/" + comp.ref + "/" + number)}"))'


def build_schematics() -> None:
    blocks: dict[str, tuple[str, list[dict[str, object]]]] = {}
    for comp in components:
        blocks[comp.ref] = custom_symbol_for_component(comp)
        comp.parsed_pins = blocks[comp.ref][1]
    library = f'''(kicad_symbol_lib (version 20231120) (generator "nightwave_reference_generator")
      {' '.join(blocks[ref][0].replace(f'(symbol "Nightwave:{ref}"', f'(symbol "{ref}"', 1) for ref in blocks)}))'''
    (OUT / "Nightwave.kicad_sym").write_text(library, encoding="utf-8")
    (OUT / "sym-lib-table").write_text(
        '(sym_lib_table (version 7) (lib (name "Nightwave")(type "KiCad")(uri "${KIPRJMOD}/Nightwave.kicad_sym")(options "")(descr "Nightwave flattened review symbols")))',
        encoding="utf-8")

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
        unique = [blocks[comp.ref][0] for comp in comps]
        symbols = [component_symbol(c, blocks[c.ref][0], c.parsed_pins) for c in comps]
        labels = [label_or_nc(c, p) for c in comps for p in c.parsed_pins]
        note = "AI-authored engineering reference. Verify pinout, support values, footprint orientation, lithium safety, RF keepout and assembly before fabrication."
        child = f'''(kicad_sch (version 20260306) (generator "nightwave_reference_generator")
          (uuid "{uid('file/'+sheet)}") (paper "User" 650 {max(300, 110+((len(comps)-1)//5)*70)})
          (lib_symbols {' '.join(unique)})
          (text "{q(title)}" (exclude_from_sim no) (at 25 18 0) (effects (font (size 2.5 2.5) (bold yes)) (justify left bottom)))
          (text "{q(note)}" (exclude_from_sim no) (at 25 25 0) (effects (font (size 1.0 1.0)) (justify left bottom)))
          {' '.join(symbols)}
          {' '.join(labels)}
          (embedded_fonts no))'''
        (OUT / f"{sheet}.kicad_sch").write_text(child, encoding="utf-8")

    root = f'''(kicad_sch (version 20260306) (generator "nightwave_reference_generator")
      (uuid "{ROOT_UUID}") (paper "A3") (lib_symbols)
      (text "NIGHTWAVE AI-AUTHORED DIGITAL REFERENCE" (exclude_from_sim no) (at 35 15 0) (effects (font (size 2.5 2.5) (bold yes)) (justify left bottom)))
      (text "Complete source package for independent engineering review; unbuilt and not approved for fabrication or lithium connection." (exclude_from_sim no) (at 35 21 0) (effects (font (size 1.1 1.1)) (justify left bottom)))
      {' '.join(sheet_entries)}
      (sheet_instances (path "/" (page "1"))) (embedded_fonts no))'''
    (OUT / f"{PROJECT}.kicad_sch").write_text(root, encoding="utf-8")


def footprint_parts(fp_id: str) -> tuple[Path, str]:
    lib, name = fp_id.split(":", 1)
    if lib == "Nightwave":
        return OUT / "Nightwave.pretty", name
    return FOOTPRINTS / f"{lib}.pretty", name


def bm83_ground_windows(fp):
    """Expose only manufacturer-recommended ground lands in the body keepout.

    DS70005402D page 55 explicitly recommends host lands 56/57 as GND.
    The stock antenna exclusion (all copper layers) is never modified.
    """
    for z in fp.Zones():
        if len(list(z.GetLayerSet().Seq()))!=1 or z.GetLayer()!=pcbnew.F_Cu:
            continue
        poly=z.Outline()
        if poly.HoleCount(0):
            continue
        for pad in fp.Pads():
            if pad.GetNumber() not in ('56','57'):
                continue
            bb=pad.GetBoundingBox();h=poly.NewHole(0)
            for x,y in [(bb.GetLeft(),bb.GetTop()),(bb.GetLeft(),bb.GetBottom()),(bb.GetRight(),bb.GetBottom()),(bb.GetRight(),bb.GetTop())]:
                poly.Append(x,y,0,h)


def build_custom_footprints() -> None:
    """Land patterns transcribed from TI/Coilcraft/Same Sky drawings.

    Dimensions are mm and are the recommended PCB lands, not package-outline
    dimensions. Duplicate corner lands retain one electrical pin number.
    """
    library=OUT / "Nightwave.pretty"
    library.mkdir(exist_ok=True)
    bm83=pcbnew.FootprintLoad(str(FOOTPRINTS/'RF_Module.pretty'),'Microchip_BM83')
    bm83.SetFPID(pcbnew.LIB_ID('Nightwave','Microchip_BM83_GroundLands'))
    bm83_ground_windows(bm83)
    pcbnew.FootprintSave(str(library),bm83)
    def footprint(name, body, pads, description, holes=()):
        lines=[f'(footprint "{name}" (version 20241229) (generator "nightwave") (layer "F.Cu")',
               f'(descr "{description}") (attr smd)',
               '(property "Reference" "REF**" (at 0 -3) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))',
               f'(property "Value" "{name}" (at 0 3) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))']
        xmin,ymin,xmax,ymax=body
        for layer,offset in [('F.Fab',0),('F.CrtYd',0.3)]:
            lines.append(f'(fp_rect (start {xmin-offset} {ymin-offset}) (end {xmax+offset} {ymax+offset}) (stroke (width 0.05) (type default)) (fill none) (layer "{layer}"))')
        for number,x,y,w,h in pads:
            lines.append(f'(pad "{number}" smd roundrect (at {x} {y}) (size {w} {h}) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.1))')
        for number,x,y,w,h,drill in holes:
            ptype='np_thru_hole' if not number else 'thru_hole'
            dexpr=f'(drill {drill})' if isinstance(drill,(float,int)) else f'(drill oval {drill[0]} {drill[1]})'
            lines.append(f'(pad "{number}" {ptype} oval (at {x} {y}) (size {w} {h}) {dexpr} (layers "*.Cu" "*.Mask"))')
        lines.append(')')
        (library/f'{name}.kicad_mod').write_text('\n'.join(lines),encoding='utf-8')

    footprint('TI_DLA0010A',(-1,-1.5,1,1.5),
              [(str(i+1),-0.9,-1+i*0.5,0.6,0.25) for i in range(5)]+
              [(str(10-i),0.55 if i!=2 else 0.35,-1+i*0.5,0.9 if i!=2 else 1.3,0.25) for i in range(5)],
              'TI DLA0010A VSON-HR; land pattern drawing 4223750/D, TPS63802 datasheet page 36')
    footprint('TI_RYK0018A',(-1.25,-1.5,1.25,1.5),[
        ('1',-1.313,-0.85,0.775,0.2),('1',-1.025,-1.1,0.2,0.7),
        ('2',-1.388,-0.393,0.625,0.2),('3',-1.388,0.007,0.625,0.2),('4',-1.388,0.407,0.625,0.2),
        ('5',-1.313,0.85,0.775,0.2),('5',-1.025,1.1,0.2,0.7),
        ('6',-0.625,1.15,0.2,0.6),('7',-0.225,1.15,0.2,0.6),
        ('8',0.175,0.95,0.2,1),('9',0.575,0.95,0.2,1),
        ('10',0.975,1.1,0.2,0.7),('10',1.288,0.85,0.825,0.2),
        ('11',1.4,0.407,0.6,0.2),('12',1.4,0.007,0.6,0.2),('13',1.4,-0.393,0.6,0.2),
        ('14',1.288,-0.85,0.825,0.2),('14',0.975,-1.1,0.2,0.7),
        ('15',0.575,-0.95,0.2,1),('16',0.175,-0.95,0.2,1),('17',-0.225,-0.95,0.2,1),
        ('18',-0.625,-1.141,0.2,0.619)],
        'TI RYK0018A WQFN-HR-18; 22 lands; drawing 4226526/A, BQ25628E datasheet')
    footprint('Coilcraft_XFL4015',(-2,-2,2,2),[('1',-1.185,0,0.98,3.4),('2',1.185,0,0.98,3.4)],
              'Coilcraft XFL4015 recommended land pattern, document 769-2 revision 2026-03-10')
    footprint('SameSky_SJ3-350153AG',(-4.75,-3.9,4.75,9),[],
              'Same Sky SJ3-350153AG-TR top view, 2025-03-10; requires 1.2mm PCB',[
        ('1',2.7,-0.8,1.0,1.5,(0.6,1.1)),('4',-2.7,-0.8,1.0,1.5,(0.6,1.1)),
        ('3',3.5,0.7,1.0,1.5,(0.6,1.1)),('MS',-3.8,2.7,1.0,1.5,(0.6,1.1)),
        ('MS',3.8,3.2,1.0,1.5,(0.6,1.1)),('2',-2.7,7.3,1.0,1.5,(0.6,1.1)),
        ('5',2.2,7.3,1.0,1.5,(0.6,1.1)),('6',3.5,7.3,1.0,1.5,(0.6,1.1)),
        ('',0,0,1.2,1.2,1.2),('',0,5.4,1.2,1.2,1.2)])
    (OUT/'fp-lib-table').write_text('(fp_lib_table (version 7) (lib (name "Nightwave")(type "KiCad")(uri "${KIPRJMOD}/Nightwave.pretty")(options "")(descr "Manufacturer land patterns")))',encoding='utf-8')


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
    board.GetDesignSettings().SetCopperLayerCount(4)
    board.GetDesignSettings().m_TrackMinWidth=pcbnew.FromMM(0.15)
    board.GetDesignSettings().m_MinClearance=pcbnew.FromMM(0.15)
    board.GetDesignSettings().m_ViasMinSize=pcbnew.FromMM(0.6)
    board.GetDesignSettings().m_MinThroughDrill=pcbnew.FromMM(0.3)
    board.GetDesignSettings().SetBoardThickness(pcbnew.FromMM(1.2))
    net_objects: dict[str, pcbnew.NETINFO_ITEM] = {}
    def get_net(name: str) -> pcbnew.NETINFO_ITEM:
        if name not in net_objects:
            obj = pcbnew.NETINFO_ITEM(board, name)
            board.Add(obj)
            net_objects[name] = obj
        return net_objects[name]

    board.GetDesignSettings().m_HoleClearance=pcbnew.FromMM(0.15)
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
        fp = load_footprint(comp.footprint)
        if fp is None:
            raise FileNotFoundError(f"Cannot load footprint {comp.footprint}")
        fp.SetReference(comp.ref)
        fp.SetValue(comp.value)
        fp.SetField("Datasheet", comp.datasheet)
        fp.SetAttributes(fp.GetAttributes() & ~pcbnew.FP_EXCLUDE_FROM_BOM)
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
            if not num:
                continue
            pin = pin_by_num.get(num)
            net_name = net_for_pin(comp, pin) if pin else None
            if not net_name:
                pname=str(pin['name']).replace('/', '{slash}') if pin else f'PAD_{num}'
                net_name = f"unconnected-({comp.ref}-{pname}-Pad{num})"
            pad.SetNet(get_net(net_name))

    legalize_placement(board)

    pcbnew.SaveBoard(str(OUT / f"{PROJECT}.kicad_pcb"), board)


def write_support_files() -> None:
    pro = {
        "board": {"design_settings":{"rules":{"min_clearance":0.15,"min_track_width":0.15,"min_through_hole_diameter":0.2,"min_hole_clearance":0.15,"min_hole_to_hole":0.25,"min_copper_edge_clearance":0.5,"min_via_diameter":0.6,"min_via_annular_width":0.1,"min_text_height":0.8,"min_text_thickness":0.08}}}, "boards": [], "cvpcb": {}, "erc": {}, "libraries": {},
        "meta": {"filename": f"{PROJECT}.kicad_pro", "version": 1},
        "net_settings": {"classes": [
            {"name":"Default","clearance":0.15,"track_width":0.2,"via_diameter":0.6,"via_drill":0.3,"diff_pair_width":0.2,"diff_pair_gap":0.15},
            {"name":"Power","clearance":0.15,"track_width":0.6,"via_diameter":0.8,"via_drill":0.4,"diff_pair_width":0.6,"diff_pair_gap":0.15}],
            "netclass_patterns":[{"netclass":"Power","pattern":n} for n in ['VBUS5','BAT','SYS','VSYS_SW','+3V3','+3V3_A','+3V6_BT','SW_CHG','SW1_3V3','SW2_3V3','SW1_BT','SW2_BT','SPK_P','SPK_N']],
            "meta": {"version": 3}},
        "pcbnew": {}, "schematic": {}, "text_variables": {
            "DESIGN_STATUS": "AI-authored, unbuilt, independent review required"
        }
    }
    (OUT / f"{PROJECT}.kicad_pro").write_text(json.dumps(pro, indent=2), encoding="utf-8")
    (OUT / ".gitignore").write_text("*.kicad_prl\n*.lck\n~*.tmp\n.history/\nreports/filled-drc.json\nreports/placement-drc.rpt\nreports/render/detail.png\n", encoding="utf-8")

    manifest = {
        "schema": 1,
        "status": "AI-authored digital reference; unbuilt; not approved for fabrication or lithium connection",
        "board": {"layers": 4, "outline_mm": [110,100], "thickness_mm":1.2, "stackup": "final fab stackup pending independent review"},
        "selected_architecture": {
            "mcu": "ESP32-S3-WROOM-1-N16R8", "bluetooth": "BM83SM1-00TA",
            "display": "Waveshare 24382", "input_expander": "TCA9535PWR",
            "battery_reference": "Adafruit 5035 protected 1S 10050 mAh plus external 10k NTC",
            "charger": "BQ25628ERYKR", "logic_regulator": "TPS63802DLAR",
            "bluetooth_regulator": "TPS63802DLAR", "load_switch": "TPS22965DSGR"
        },
        "components": [
            {"reference": c.ref, "value": c.value, "manufacturer": c.manufacturer, "mpn": c.mpn or "PENDING_SELECTION",
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
            feature=c.ref.startswith('TP')
            writer.writerow({"Reference":c.ref,"Qty":1,"Manufacturer":c.manufacturer,"MPN":"NO_PURCHASE" if feature else c.mpn or "PENDING_SELECTION","Value":c.value,
                             "Footprint":c.footprint,"Datasheet":c.datasheet,"Unit_Price_USD":c.unit_price,"Supplier_URL":c.supplier_url,
                             "Status":"PCB_FEATURE" if feature else "REFERENCE_REVIEW_REQUIRED" if c.mpn else "MPN_PENDING_SELECTION"})
        for ref,maker,mpn,value,url,price in [
            ('OFFBOARD_BAT','Adafruit','5035','Protected 1S 10050mAh; connector limited to 2 A; polarity/pack fit pending','https://www.adafruit.com/product/5035','29.95'),
            ('OFFBOARD_SPK','Same Sky','CMS-28528N-L152','Speaker; terminate harness for J6','https://www.sameskydevices.com/product/resource/cms-28528n-l152.pdf','4.76'),
            ('OFFBOARD_TFT','Waveshare','24382','240x280 TFT module; custom J7 harness','https://www.waveshare.com/wiki/1.69inch_LCD_Module','9.49'),
            ('OFFBOARD_NTC','','PENDING_SELECTION','10k NTC; verify B curve, tolerances and insulated battery attachment','',''),
            ('OFFBOARD_HARNESS','','PENDING_SELECTION','PH/GH housings, contacts and wires; polarity and wire rating review required','',''),
        ]:
            writer.writerow({'Reference':ref,'Qty':1,'Manufacturer':maker,'MPN':mpn,'Value':value,'Footprint':'OFF_BOARD_NOT_A_PCB_FOOTPRINT','Datasheet':url,'Unit_Price_USD':price,'Supplier_URL':url,'Status':'OFFBOARD_REFERENCE_REVIEW_REQUIRED'})

    readme = """# Nightwave KiCad reference design

This directory is an AI-authored KiCad engineering reference. See `VALIDATION_STATUS.md` for the actual routing/check results of the exported revision. It is unbuilt, unmeasured, and **not approved for fabrication or lithium-cell connection**. It is not evidence of human-authored CAD or Pixl acceptance.

## What is here

- native KiCad 10 hierarchical schematic and four-layer PCB source;
- selected ESP32-S3, BM83, Waveshare TFT, TCA9535, local DAC/headphone/speaker, USB-C, charger, dual regulated rails, protected battery and service/test interfaces;
- exact logical net/GPIO assignments generated from the same source as the PCB;
- BOM and machine-readable design manifest;
- `manufacturing/` review exports generated by KiCad CLI and prominently marked **DO NOT FABRICATE**;
- `reports/` ERC/DRC/netlist and rendered-review evidence.

## Critical stop

Do not order this board yet. An independent reviewer must close `REVIEW_CHECKLIST.md`. Automated routing and clearance/connectivity checks do not establish USB impedance, power-loop stability, current capacity, RF performance, charger safety, manufacturability or enclosure fit. All block-symbol pins currently use passive electrical types: zero ERC means connectivity checks only, not power/driver-type qualification.

## Opening tomorrow

Open `Nightwave-Reference.kicad_pro`. Start with the root schematic, then inspect the PCB. The existing `nightwave-starter` project is intentionally separate and unchanged.
"""
    (OUT / "README.md").write_text(readme, encoding="utf-8")

    checklist = """# Independent review checklist

- [ ] Confirm exact USB4105 suffix, mating/cutout dimensions, shield strategy and CC implementation.
- [ ] Review fixed 5 V USB-C sink source-current policy: two Rd resistors, no PD or CC current detector; 5.49k ILIM targets about 455 mA typical. Do not assume a USB host permits this before enumeration. Review inrush, ESD and charger defaults/configuration before attaching USB/cell.
- [ ] Confirm BQ25628E RYK-18 pinout/22 split lands, charge current/voltage, safety timer, TS network and thermal limits from the current datasheet.
- [ ] Confirm Adafruit 5035 connector polarity, 3 A limits, mechanical retention, replaceability and external 10 kOhm NTC attachment.
- [ ] Confirm TPS63802 inductors, feedback dividers, effective capacitance, peak currents and thermal copper for both rails.
- [ ] Confirm ESP32-S3 N16R8 GPIO availability, straps, USB, PSRAM exclusions and antenna keepout.
- [ ] Confirm BM83SM1-00TA 50-pad land pattern, supply sequencing, AT source firmware, UART/MFB/service pins, I2S clock direction, sample rates and antenna keepout.
- [ ] Confirm Waveshare 24382 cable connector, pin order, 3.3 V levels, backlight current/PWM and enclosure orientation.
- [ ] Confirm TCA9535 address 0x20, external pulls for every input, interrupt polarity and unused-input treatment.
- [ ] Confirm PCM5102A/TPA6132A2/MAX98357A reference circuits, gain, pop/click behavior, SJ3-350153AG isolated headphone detection and BTL speaker isolation. Review the 47k/10k averaged-stereo SD_MODE divider including internal 100k and GPIO VOH.
- [ ] Review the native four-layer layout for switch-mode current loops, USB 90-ohm differential routing/ESD, RF keepouts, return paths, rail neck-down current density and via/thermal capacity. Autorouter output is not signal-integrity sign-off.
- [ ] Verify 1.2 mm board thickness required by the selected jack, 110x100 mm board outline, real mounting/port access, display cable and pack fit. The board STEP alone is not an enclosure STEP.
- [ ] Finish exact passive/connector/NTC MPNs, DC-bias/voltage ratings, sourcing, assembly capabilities and priced BOM; generic-value rows are pending selections, not purchase-ready MPNs.
- [ ] Reconcile the reference GPIO configuration with firmware. Do not flash the historical OLED/direct-button configuration onto this board and claim TFT/BT qualification.
- [ ] Re-run ERC, DRC, schematic parity, fabrication viewer, assembly drawing and enclosure interference checks.
- [ ] Obtain named independent reviewer sign-off before ordering or connecting a cell.
"""
    (OUT / "REVIEW_CHECKLIST.md").write_text(checklist, encoding="utf-8")


def legalize_placement(board):
    """Keep all component courtyards disjoint, preferring the circuit clusters.

    This is geometry legalization, not signal-integrity or power-loop approval.
    Package connector courtyards can cross an edge; all electrical pads must
    remain at least 0.5 mm inside the outline.
    """
    def bounds(fp):
        boxes=[g.GetBoundingBox() for g in fp.GraphicalItems() if g.GetLayer()==pcbnew.F_CrtYd]
        boxes += [p.GetBoundingBox() for p in fp.Pads()]
        if not boxes:
            boxes=[fp.GetBoundingBox(False,False)]
        return (min(b.GetLeft() for b in boxes)/1e6,min(b.GetTop() for b in boxes)/1e6,
                max(b.GetRight() for b in boxes)/1e6,max(b.GetBottom() for b in boxes)/1e6)
    occupied=[]
    fps=list(board.GetFootprints())
    priority={"J1":0,"U1":1,"U12":2,"U3":3,"U4":4,"U5":5,"U6":6}
    fps.sort(key=lambda fp: (priority.get(fp.GetReference(),7),-((bounds(fp)[2]-bounds(fp)[0])*(bounds(fp)[3]-bounds(fp)[1]))))
    for fp in fps:
        x,y=pcbnew.ToMM(fp.GetPosition().x),pcbnew.ToMM(fp.GetPosition().y)
        b=bounds(fp)
        offsets=[(0,0)]
        for radius in range(1,41):
            r=radius*0.5
            offsets.extend([(dx,dy) for dx,dy in [(-r,0),(r,0),(0,-r),(0,r),(-r,-r),(r,-r),(-r,r),(r,r)]])
        grid=[(6+gx*2.5-x,6+gy*2.5-y) for gx in range(43) for gy in range(39)]
        offsets.extend(sorted(grid,key=lambda v:v[0]**2+v[1]**2))
        placed=False
        for dx,dy in offsets:
            proposed=(b[0]+dx,b[1]+dy,b[2]+dx,b[3]+dy)
            if proposed[0]<5.5 or proposed[1]<5.5 or proposed[2]>114.5 or proposed[3]>104.5:
                # Edge-facing connectors may have an overhanging fab courtyard.
                if not fp.GetReference().startswith('J'):
                    continue
                pads=[p.GetBoundingBox() for p in fp.Pads() if p.GetNumber()]
                if any(p.GetLeft()/1e6+dx<5.5 or p.GetRight()/1e6+dx>114.5 or p.GetTop()/1e6+dy<5.5 or p.GetBottom()/1e6+dy>104.5 for p in pads):
                    continue
            if any(proposed[0]<o[2]+0.4 and proposed[2]>o[0]-0.4 and proposed[1]<o[3]+0.4 and proposed[3]>o[1]-0.4 for o in occupied):
                continue
            fp.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x+dx),pcbnew.FromMM(y+dy)))
            occupied.append(proposed)
            placed=True
            break
        if not placed:
            raise RuntimeError(f'Cannot legalize {fp.GetReference()} at {(x,y)}')


def main() -> None:
    parser=argparse.ArgumentParser()
    parser.add_argument('--schematic-only',action='store_true',help='Preserve routed PCB and reports')
    parser.add_argument('--rebuild-board',action='store_true',help='Explicitly replace the existing PCB with an unrouted placement')
    args=parser.parse_args()
    if (OUT/f'{PROJECT}.kicad_pcb').exists() and not args.schematic_only and not args.rebuild_board:
        parser.error('Existing PCB preserved. Use --schematic-only for documents; --rebuild-board explicitly discards routing.')
    OUT.mkdir(parents=True, exist_ok=True)
    build_custom_footprints()
    # Place block symbols in rows with enough room for every label and the
    # largest module; paginate the power sheet when required.
    for sheet,_ in SHEETS:
        group=[c for c in components if c.sheet==sheet]
        for index,comp in enumerate(group):
            comp.x=65+(index%5)*115
            comp.y=65+(index//5)*70
    for comp in components:
        comp.x = round(comp.x / 1.27) * 1.27
        comp.y = round(comp.y / 1.27) * 1.27
    build_schematics()
    if not args.schematic_only:
        build_board()
    write_support_files()
    print(f"Generated {len(components)} components in {OUT}")


if __name__ == "__main__":
    main()
