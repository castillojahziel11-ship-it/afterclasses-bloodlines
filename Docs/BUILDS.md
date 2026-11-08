# BUILDS

| # | Gate | Result |
|---|------|--------|
| 0001 | H0 | Frame-rule core unit tests PASS (g++ -std=c++20, Linux). No UE build. |
| 0002 | Core foundation | Frame-rule tests PASS (g++ 12.2, -std=c++20 -Wall -Wextra). `Build/check_loc.sh` OK. `Build/sanity_check.py` OK (static only). **No UE compile — no UE install in authoring environment (KI-002).** |
| 0003 | Combat + AI + Localization + asset generator | `Build/run_tests.sh` ALL PASS (g++ 12.2 -std=c++20 -Wall -Wextra -Werror: framecore, evaluator incl. gate "Hard dodges/parries after three repeated heavies" seed 1337; `test_move_table.py`; `py_compile` + `test_generator.py`). `Build/check_loc.sh` OK. `Build/sanity_check.py` OK (static only). **No UE compile (KI-002); generator not executed in an editor (KI-017).** |
