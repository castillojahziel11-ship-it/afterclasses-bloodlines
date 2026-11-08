# CHANGELOG

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
