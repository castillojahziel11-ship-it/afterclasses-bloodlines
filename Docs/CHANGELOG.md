# CHANGELOG

## 0.2.0 — Combat, AI, localization, asset generator (unreleased, not yet UE-compiled)
- feat(combat): `UACCombatComponent` rewritten as a 60 Hz server-authoritative move state machine (spec §7): Light 7/3/11, Heavy 13/4/19, dodge i-frames 3–13 / end 22, perfect dodge (6f → 15f counter, +25 meter, 6f attacker hitstop), block cone 120° / 8 stamina / 40f guard break, parry 4f (20% reflect, 12f counter), grab escape 18f, one wall + one ground bounce, combo cap 12, meter gains/costs, knockdown/getup, YN Anonymous passive; `MulticastPlayMove` / `MulticastHitConfirmed`.
- feat(combat): `UACAnimNotifyState_HitWindow` (AN_HitWindow_Open bone chain), `UACHitValidation` (per-tick swept bone segments, swing-ID dedupe, hurt-capsule resolve), `UACHitstopSubsystem`, `FACHitData`, `FACMoveRow`; `Content/Data/MoveTables/MT_Default.csv` + `Tests/test_move_table.py`.
- feat(ai): `ACEvaluatorCore.h` (difficulty table, deliberate error 14/10/6/3 %, latency 320/220/150/90 ms, 12-entry adaptation) + `Tests/test_evaluator.cpp`; `UACCombatEvaluator`, `AACEnemyController` (sight 1800 uu / 100°, hearing 2500 uu), `UACBTTask_ExecuteDecision`, `UACBTService_UpdateCombatBlackboard`, `UACBossPhaseComponent` (66 % / 33 %, one telegraphed unblockable per phase). Controller runs decisions directly if BT_Fighter is missing or has an empty graph.
- feat(loc): `ST_UI` / `ST_Characters` / `ST_Combat` CSV string tables registered at startup (`ACText`), cultures en / ja / ko / es-419, translation-pending CSVs (no fake translations), `check_loc.sh` covers all Source; native tag `Unlock.SeveredEnding`.
- feat(tools): `Content/Python/generate_afterclass_assets.py` — idempotent one-shot generator for input actions, IMC (KBM + pad, remappable), 18 fighter data assets + placeholder, move-table DataTables, string tables, BP_Fighter_Placeholder, BP_GM_Dev, BB_Fighter / BT_Fighter, greybox L_TestArena. `UACEditorAssetLibrary` Python helpers in `AfterclassEditor`. `PythonScriptPlugin` + `EditorScriptingUtilities` enabled. `EDITOR_SETUP.md` is now "run this one script" with manual fallback.
- docs: KI-001, KI-011, KI-014 closed; KI-015..KI-019 added; build 0003.

## 0.1.0 — Core foundation (unreleased, not yet UE-compiled)
- build: retarget to UE 5.5 (`EngineAssociation 5.5`, `BuildSettingsVersion.V5`, `EngineIncludeOrderVersion.Unreal5_5`); add Game/Editor targets, primary module boilerplate, `AfterclassEditor` module; fix plugin name `Metasound`.
- fix(combat): `UACCombatComponent` now replicates correctly (`GetLifetimeReplicatedProps`, `Net/UnrealNetwork.h`, one declarator per `UPROPERTY`), routes requests through a server RPC, adds Grab/Special/Sprint and a state-changed delegate.
- feat(save): `UACSaveSubsystem` — Auto + Manual_1..5 + Profile slots; container with magic/version/CRC32; atomic temp-file + verify + move; corrupt files quarantined; version-too-new files never overwritten.
- feat(input): `UACInputSettings` (Project Settings > Game > Afterclass Input) with documented default asset paths; `DefaultInput.ini` uses EnhancedPlayerInput/EnhancedInputComponent with user settings enabled; Steam Input manifest placeholder.
- feat(characters): `AACCharacterBase` (spring arm + camera, Enhanced Input → combat component, data-driven walk/sprint), `AACFighter` (outfits), `UACCharacterDataAsset` (all spec fields) + `FACMoveRow`.
- feat(core): `AACGameMode`, `UACGameInstance`, `AACPlayerController` (adds IMC for local players, key remap persisted to profile, gamepad detection), `UACBuildInfo`, native `Unlock.*` tags, `DefaultGame.ini`, Asset Manager `Fighter` type.
- chore(build): `Build/sanity_check.py` static UE convention checker; `check_loc.sh` hardened.

## H0
- feat(combat): engine-independent frame data, scaling, meter, perfect-dodge rules + tests
- chore: project skeleton, uproject, Build.cs, DefaultEngine.ini, shipping script, map brief (subway)
