#!/bin/sh
# Builds and runs the engine-independent unit tests (frame rules + AI evaluator). Run from anywhere.
set -e
cd "$(dirname "$0")/.."
OUT="${TMPDIR:-/tmp}/afterclass_tests"; mkdir -p "$OUT"
CXX="${CXX:-g++}"
for t in test_framecore test_evaluator; do
  "$CXX" -std=c++20 -Wall -Wextra -Werror -O1 -o "$OUT/$t" "Tests/$t.cpp"
  "$OUT/$t"
done
echo "run_tests: ALL PASS"
