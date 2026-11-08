#!/usr/bin/env python3
"""Keeps Localization/Game/<culture>/TRANSLATION_PENDING.csv in sync with the English string tables.

  python3 Build/loc_pending.py           # regenerate (keeps any Translation/Status already filled in)
  python3 Build/loc_pending.py --check   # CI: fail if a pending file is missing keys or has a stale SourceString

Targets are intentionally EMPTY: ja / ko / es-419 need professional translation (Docs/KNOWN_ISSUES.md KI-015).
"""
import csv
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TABLES = ROOT / "Content" / "Localization" / "StringTables"
CULTURES = ["ja", "ko", "es-419"]
FIELDS = ["Table", "Key", "SourceString", "Translation", "Status", "Comment"]


def load_source():
    rows = []
    for path in sorted(TABLES.glob("ST_*.csv")):
        with path.open(newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                rows.append({"Table": path.stem, "Key": r["Key"], "SourceString": r["SourceString"], "Comment": r.get("Comment", "")})
    return rows


def load_pending(path):
    if not path.exists():
        return {}
    with path.open(newline="", encoding="utf-8") as f:
        return {(r["Table"], r["Key"]): r for r in csv.DictReader(f)}


def main():
    check = "--check" in sys.argv
    source = load_source()
    keys = [(r["Table"], r["Key"]) for r in source]
    if len(keys) != len(set(keys)):
        print("FAIL: duplicate string-table keys")
        return 1
    failed = False
    for culture in CULTURES:
        path = ROOT / "Localization" / "Game" / culture / "TRANSLATION_PENDING.csv"
        existing = load_pending(path)
        if check:
            for r in source:
                e = existing.get((r["Table"], r["Key"]))
                if e is None or e["SourceString"] != r["SourceString"]:
                    print(f"FAIL: {path.relative_to(ROOT)} missing/stale {r['Table']}:{r['Key']} (run python3 Build/loc_pending.py)")
                    failed = True
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=FIELDS, lineterminator="\n")
            w.writeheader()
            for r in source:
                e = existing.get((r["Table"], r["Key"]), {})
                same = e.get("SourceString") == r["SourceString"]
                translation = e.get("Translation", "") if same else ""
                w.writerow({**r, "Translation": translation,
                            "Status": (e.get("Status") or "PENDING") if (same and translation) else "PENDING"})
        print(f"wrote {path.relative_to(ROOT)} ({len(source)} entries)")
    if check:
        print("FAIL" if failed else f"loc_pending: OK ({len(source)} keys x {len(CULTURES)} cultures)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
