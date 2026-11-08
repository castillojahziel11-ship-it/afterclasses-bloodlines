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
python3 Tests/test_move_table.py
python3 -m py_compile Content/Python/generate_afterclass_assets.py
python3 Tests/test_generator.py
echo "run_tests: ALL PASS"
