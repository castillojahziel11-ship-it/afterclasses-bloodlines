#!/bin/sh
# CI gate: FText::FromString banned in shipping UI (spec §16)
if grep -rn "FText::FromString" Source/Afterclass/UI; then echo "FAIL"; exit 1; fi; echo "OK"
