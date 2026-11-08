#!/bin/sh
# CI gate: FText::FromString banned in shipping UI (spec §16). Run from repo root.
cd "$(dirname "$0")/.." || exit 2
if [ ! -d Source/Afterclass/UI ]; then echo "OK (no Source/Afterclass/UI yet)"; exit 0; fi
if grep -rn "FText::FromString" Source/Afterclass/UI; then echo "FAIL"; exit 1; fi
echo "OK"
