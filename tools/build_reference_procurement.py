"""Merge checked MPN choices with the actual circuit inventory, no price invention."""
import csv
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"hardware/kicad/nightwave-reference/reference-bom.csv"
CHOICES=ROOT/"hardware/reference-resistor-selections.json"
OUTPUT=ROOT/"hardware/reference-procurement.csv"

def build(rows, choices):
    result=[]
    for source in rows:
        row=dict(source)
        # Old source price snapshots are retained as explicitly historical,
        # never silently promoted to current quotes or a complete order total.
        row["Price_Status"]="HISTORICAL_SNAPSHOT_REQUOTE" if row["Unit_Price_USD"] else "QUOTE_PENDING"
        row["Selection_Checked_On"]=""
        row["Rating_Notes"]=""
        ref=row["Reference"]
        if ref.startswith("R") and ref[1:].isdigit() and row["Value"] in choices["by_value"]:
            if row["Footprint"]!=choices["footprint"]:
                raise ValueError(f"{ref}: resistor selection does not match footprint")
            if row["MPN"] not in ("PENDING_SELECTION", choices["by_value"][row["Value"]]):
                raise ValueError(f"{ref}: existing MPN conflicts with selection")
            row["Manufacturer"]=choices["manufacturer"]
            row["MPN"]=choices["by_value"][row["Value"]]
            row["Datasheet"]=choices["datasheet_base"]+row["MPN"]
            row["Status"]="MPN_SELECTED_APPLICATION_REVIEW_REQUIRED"
            row["Selection_Checked_On"]=choices["checked_on"]
            row["Rating_Notes"]="1%; 0.1 W at 70 C; 75 V limiting element voltage; power/thermal derating applies"
        if row["Status"]=="PCB_FEATURE": row["Price_Status"]="NO_PURCHASE"
        result.append(row)
    return result

def main():
    with SOURCE.open(newline="",encoding="utf-8-sig") as f: rows=list(csv.DictReader(f))
    result=build(rows,json.loads(CHOICES.read_text()))
    with OUTPUT.open("w",newline="",encoding="utf-8") as f:
        writer=csv.DictWriter(f,fieldnames=list(result[0])); writer.writeheader(); writer.writerows(result)
    selected=sum(r["Status"]=="MPN_SELECTED_APPLICATION_REVIEW_REQUIRED" for r in result)
    unresolved=sum(r["MPN"]=="PENDING_SELECTION" for r in result)
    print(json.dumps({"resistors_selected":selected,"mpns_pending":unresolved,
                      "complete_priced_total":None,"purchase_ready":False,"output":str(OUTPUT)}))

if __name__=="__main__": main()
