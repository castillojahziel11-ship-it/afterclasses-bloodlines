#!/bin/sh
# CI gate (spec §16), run from anywhere:
#  1. FText::FromString is banned in ALL of Source (FText::FromStringTable is allowed). Comment lines are ignored.
#  2. Every string-table key used through ACText helpers / FText::FromStringTable exists in the CSVs.
#  3. Translation-pending CSVs (ja, ko, es-419) are in sync with the English tables.
cd "$(dirname "$0")/.." || exit 2
status=0

hits=$(grep -rnE --include='*.h' --include='*.cpp' 'FText::FromString[[:space:]]*\(' Source | grep -vE '^[^:]+:[0-9]+:[[:space:]]*(//|\*|/\*)')
if [ -n "$hits" ]; then
	echo "$hits"
	echo "FAIL: FText::FromString in Source (use string tables via ACText.h)"
	status=1
fi

python3 - <<'PY' || status=1
import csv, pathlib, re, sys
root = pathlib.Path(".")
tables = {}
for p in (root / "Content/Localization/StringTables").glob("ST_*.csv"):
    with p.open(newline="", encoding="utf-8") as f:
        tables[p.stem] = {r["Key"] for r in csv.DictReader(f)}
helpers = {"UI": "ST_UI", "Characters": "ST_Characters", "Combat": "ST_Combat"}
bad = 0
for src in list(root.glob("Source/**/*.cpp")) + list(root.glob("Source/**/*.h")):
    text = src.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r'ACText::(UI|Characters|Combat)\(\s*TEXT\("([^"]+)"\)', text):
        if m.group(2) not in tables.get(helpers[m.group(1)], set()):
            print(f"FAIL: {src}: key '{m.group(2)}' missing from {helpers[m.group(1)]}.csv"); bad = 1
    for m in re.finditer(r'FText::FromStringTable\(\s*(?:TEXT\()?"(ST_\w+)"\)?\s*,\s*(?:TEXT\()?"([^"]+)"', text):
        if m.group(2) not in tables.get(m.group(1), set()):
            print(f"FAIL: {src}: key '{m.group(2)}' missing from {m.group(1)}.csv"); bad = 1
sys.exit(bad)
PY

python3 Build/loc_pending.py --check >/dev/null || { python3 Build/loc_pending.py --check | grep FAIL | head -5; status=1; }

[ $status -eq 0 ] && echo "OK"
exit $status
