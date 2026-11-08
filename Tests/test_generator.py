#!/usr/bin/env python3
"""Static consistency checks for Content/Python/generate_afterclass_assets.py (cannot import `unreal` here).
Verifies the generator's constants against the C++ and the data it must stay in sync with. NOT an editor run."""
import ast
import csv
import pathlib
import py_compile
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
gen = root / "Content/Python/generate_afterclass_assets.py"
errors = []
py_compile.compile(str(gen), doraise=True)
tree = ast.parse(gen.read_text())
consts = {}
for node in tree.body:
    if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
        try:
            consts[node.targets[0].id] = ast.literal_eval(node.value)
        except ValueError:
            if isinstance(node.value, ast.BinOp):  # e.g. BT_PATH = AI_DIR + "/BT_Fighter"
                try:
                    consts[node.targets[0].id] = eval(compile(ast.Expression(node.value), "gen", "eval"), {}, dict(consts))
                except Exception:
                    pass

# Input actions/IMC paths match ACInputSettings.cpp
settings = (root / "Source/Afterclass/Input/ACInputSettings.cpp").read_text()
code_actions = set(re.findall(r'/Game/Input/Actions/(IA_\w+)\.', settings))
gen_actions = set(consts["DIGITAL"]) | set(consts["AXIS2D"])
if code_actions != gen_actions: errors.append(f"input actions differ: code={sorted(code_actions)} gen={sorted(gen_actions)}")
if f'{consts["IMC_PATH"]}.IMC_Default' not in settings: errors.append("IMC path differs from ACInputSettings")
if set(consts["AXIS2D"]) != {"IA_Move", "IA_Look"}: errors.append("Axis2D actions must be IA_Move, IA_Look")
bound = {b[0] for b in consts["BINDINGS"]}
if bound != gen_actions: errors.append(f"actions without bindings: {sorted(gen_actions - bound)}")
names = [b[3] for b in consts["BINDINGS"]]
if len(names) != len(set(names)): errors.append("duplicate mappable key names")
for a in gen_actions:
    kinds = {("Pad" if b[3].endswith("_Pad") else "KBM") for b in consts["BINDINGS"] if b[0] == a}
    if kinds != {"KBM", "Pad"}: errors.append(f"{a} needs both KBM and gamepad bindings")

# Roster: 18 fighters, every display-name key exists in ST_Characters.csv, unlock rules
with open(root / "Content/Localization/StringTables/ST_Characters.csv", newline="") as f:
    keys = {r["Key"] for r in csv.DictReader(f)}
roster = dict(consts["ROSTER"])
if len(roster) != 18: errors.append(f"roster has {len(roster)} fighters, expected 18")
for fid in list(roster) + ["Placeholder"]:
    if f"Fighter.{fid}.Name" not in keys: errors.append(f"missing ST_Characters key Fighter.{fid}.Name")
if roster.get("DexMarrow", {}).get("unlock") != "Unlock.Chapter5Complete": errors.append("DexMarrow must unlock after Ch.5")
if roster.get("YN", {}) != {"unlock": "Unlock.SeveredEnding", "anonymous": True}: errors.append("YN unlock/passive wrong")
tags = (root / "Source/Afterclass/Core/ACGameplayTags.cpp").read_text()
for opts in roster.values():
    t = opts.get("unlock", "Unlock.Default")
    if f'"{t}"' not in tags: errors.append(f"tag {t} not defined natively")

# Blackboard keys match ACBlackboardKeys.h
bb_code = dict(re.findall(r'inline const FName (\w+)\(TEXT\("\w+"\)\);\s*//\s*(\w+)', (root / "Source/Afterclass/AI/ACBlackboardKeys.h").read_text()))
bb_gen = dict(consts["BB_KEYS"])
if set(bb_code) != set(bb_gen): errors.append(f"BB keys differ: code={sorted(bb_code)} gen={sorted(bb_gen)}")
for k, t in bb_gen.items():
    if k in bb_code and bb_code[k] != t: errors.append(f"BB key {k}: code type {bb_code[k]} vs gen {t}")

# Paths the code / config reference
ctrl = (root / "Source/Afterclass/AI/ACEnemyController.cpp").read_text()
if f'{consts["BT_PATH"]}.BT_Fighter' not in ctrl: errors.append("BT path differs from ACEnemyController")
engine_ini = (root / "Config/DefaultEngine.ini").read_text()
if f'GameDefaultMap={consts["MAP_PATH"]}.L_TestArena' not in engine_ini: errors.append("map path differs from DefaultEngine.ini")
if not consts["FIGHTERS_DIR"].startswith("/Game/Characters"): errors.append("fighters must live under /Game/Characters (Asset Manager)")
if not list((root / "Content/Data/MoveTables").glob("*.csv")): errors.append("no move table CSVs")
uproject = (root / "Afterclass.uproject").read_text()
for p in ("PythonScriptPlugin", "EditorScriptingUtilities"):
    if f'"{p}"' not in uproject: errors.append(f"{p} not enabled in uproject")
doc = (root / "Docs/EDITOR_SETUP.md").read_text()
for k in ("IMC_PATH", "DA_PLACEHOLDER", "BP_FIGHTER", "BP_GM_DEV", "BB_PATH", "BT_PATH", "MAP_PATH"):
    if consts[k] not in doc: errors.append(f"{consts[k]} not documented in EDITOR_SETUP.md")

print("\n".join(errors) if errors else "test_generator: OK (static only; generator not executed in an editor)")
sys.exit(1 if errors else 0)
