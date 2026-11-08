# AFTERCLASS: BLOODLINE

Third-person C++ fighting game for **Unreal Engine 5.5**, Win64 (reference machine: RTX 5070).
Project `Afterclass` · runtime module `Afterclass` · editor module `AfterclassEditor`.

> **Honest status (core foundation, v0.1.0):** the C++ game framework, input, character, combat-state and save systems are
> written but **have never been compiled against Unreal Engine** — the authoring environment has no UE install.
> Only the engine-independent frame-rule core has been built and tested (g++). Expect compile errors on first Windows build;
> report them verbatim. No art, animation, maps, VO or UI content exists yet. See [`Docs/KNOWN_ISSUES.md`](Docs/KNOWN_ISSUES.md).

## Requirements
- Windows 10/11 x64
- Unreal Engine **5.5** (Epic Games Launcher)
- Visual Studio 2022 (17.8+) with workloads **Game development with C++** and **Desktop development with C++**,
  plus the MSVC v143 toolset and a Windows 10/11 SDK (as listed in UE 5.5 release notes)
- Optional: Python 3 and a POSIX shell (Git Bash) for the CI-style checks below

## Build (Development Editor)
1. Clone the repo.
2. Right-click `Afterclass.uproject` → **Generate Visual Studio project files** (choose 5.5 if prompted).
   Command-line equivalent:
   ```bat
   "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" -projectfiles -project="%CD%\Afterclass.uproject" -game -rocket -progress
   ```
3. Open `Afterclass.sln`, set configuration **Development Editor | Win64**, startup project **Afterclass**, then **Build**.
   Command-line equivalent:
   ```bat
   "C:\Program Files\Epic Games\UE_5.5\Engine\Build\BatchFiles\Build.bat" AfterclassEditor Win64 Development -Project="%CD%\Afterclass.uproject" -WaitMutex
   ```
4. Launch the editor (F5 or double-click the `.uproject`), then create the required assets listed in
   [`Docs/EDITOR_SETUP.md`](Docs/EDITOR_SETUP.md) (input actions, mapping context, test map, placeholder fighter).

## Tests and checks
```bash
# Frame-rule core (engine-independent) — any C++20 compiler
g++ -std=c++20 -Wall -Wextra -o Tests/t Tests/test_framecore.cpp && ./Tests/t      # expect: ALL PASS
# MSVC: cl /std:c++20 /EHsc Tests\test_framecore.cpp /Fe:Tests\t.exe && Tests\t.exe

sh Build/check_loc.sh            # FText::FromString banned in shipping UI
python3 Build/sanity_check.py    # static UE convention checks (NOT a compile)
```
UE Automation tests (Session Frontend → Automation) will be added with the gameplay systems.

## Shipping build
```powershell
$env:UE_ROOT = "C:\Program Files\Epic Games\UE_5.5"
./Build/build_shipping.ps1
```
Not yet run (KI-004).

## Layout
```
Source/Afterclass/
  Afterclass.h/.cpp           primary game module, LogAfterclass
  Core/                       ACGameMode, ACGameInstance, ACPlayerController, ACBuildInfo, ACGameplayTags
  Characters/                 ACCharacterBase → ACFighter, ACCharacterDataAsset (+ FACMoveRow, FACOutfit)
  Combat/                     ACFrameCore.h (engine-independent rules), ACCombatComponent
  Input/                      ACInputSettings (Project Settings → Game → Afterclass Input)
  Save/                       ACSaveSubsystem, ACSaveGames (profile + campaign)
Source/AfterclassEditor/      editor module (boilerplate)
Config/                       DefaultEngine/Game/Input/GameplayTags.ini
Steam/                        steam_input_manifest.vdf (placeholder)
Tests/                        g++ unit tests for ACFrameCore
Docs/                         KNOWN_ISSUES, BUILDS, CHANGELOG, EDITOR_SETUP, MAP_BRIEFS
```

## Design rules
- Every fighter is an `AACFighter`; all differentiation lives in `UACCharacterDataAsset`. No Blueprint-only gameplay.
- No `FText::FromString` in shipping UI — use String Tables / `LOCTEXT`.
- Every stub carries `// STUB:` and a `KNOWN_ISSUES` entry.
- Locked decisions: Union Square Subway is the Ch.5 polish centrepiece; Dex Marrow is boss-only, playable after Ch.5.
