#!/usr/bin/env python3
"""Checks Content/Data/MoveTables/MT_Default.csv against ACFrameCore.h frame data and FACMoveRow / FACHitData fields."""
import csv, pathlib, re, sys
root = pathlib.Path(__file__).resolve().parent.parent
core = (root / "Source/Afterclass/Combat/ACFrameCore.h").read_text()
frames = {m.group(1): tuple(map(int, m.groups()[1:])) for m in re.finditer(r"(\w+)\{(\d+),(\d+),(\d+)\}", core)}
expect = {"Light": frames["Light"], "Heavy": frames["Heavy"], "Grab": frames["GrabMove"], "Special": frames["SpecialMove"], "Finisher": frames["FinisherMove"]}
types = (root / "Source/Afterclass/Combat/ACCombatTypes.h").read_text() + (root / "Source/Afterclass/Combat/ACHitTypes.h").read_text()
row_fields = set(re.findall(r"UPROPERTY\(.*?\)\s+[\w<>:]+\s+(\w+)\s*(?:=|;)", types))
fails = 0
def check(cond, msg):
    global fails
    if not cond: print("FAIL:", msg); fails += 1
for path in sorted((root / "Content/Data/MoveTables").glob("*.csv")):
    with path.open(newline="") as f:
        rows = list(csv.reader(f))
    header = rows[0]
    for col in header[1:]:
        check(col in row_fields, f"{path.name}: column {col} is not a FACMoveRow property")
    for r in rows[1:]:
        d = dict(zip(header, r))
        name = r[0]
        check(d["MoveId"] == name, f"{name}: MoveId must equal row name")
        if name in expect:
            got = (int(d["StartupFrames"]), int(d["ActiveFrames"]), int(d["RecoveryFrames"]))
            check(got == expect[name], f"{name}: frames {got} != ACFrameCore {expect[name]}")
        for field in re.findall(r"(\w+)=", d["HitData"]):
            check(field in row_fields, f"{name}: HitData field {field} not in FACHitData")
check(float([r for r in csv.reader(open(root / "Content/Data/MoveTables/MT_Default.csv"))][4][8]) == 50.0, "Special meter cost must be 50")
check(float([r for r in csv.reader(open(root / "Content/Data/MoveTables/MT_Default.csv"))][5][8]) == 100.0, "Finisher meter cost must be 100")
print("move_table: ALL PASS" if not fails else f"move_table: {fails} FAIL")
sys.exit(1 if fails else 0)
