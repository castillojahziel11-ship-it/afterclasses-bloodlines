# KNOWN_ISSUES

Every `// STUB:` in source maps to an entry here. Severity: **blocker** (prevents a playable/shippable build), **major**, **minor**.

| ID | Severity | Area | Issue |
|----|----------|------|-------|
| KI-001 | ~~blocker~~ **closed (0003)** | Combat | ~~Combat component partial.~~ Implemented in C++: 60 Hz move state machine, montage dispatch, AN_HitWindow_Open bone-chain sweeps (`UACHitValidation`), hitstop subsystem, dodge/perfect dodge, block/parry/guard break, grab escape, combo scaling + bounces + cap, knockdown/getup, meter. Remaining risk is engine verification → KI-002 / KI-018; art hooks → KI-016. |
| KI-002 | blocker | Build | **Nothing has been compiled against Unreal Engine.** The authoring environment has no UE install or GPU. Code was written against UE 5.5 APIs and passes a static convention check (`Build/sanity_check.py`) only. First Windows compile (UE 5.5 + VS2022) is expected to surface errors — report them verbatim. |
| KI-003 | blocker | Content | No final characters, maps, VO, UI widgets, netcode sessions or Steam content exist. Greybox/placeholder assets come from the generator (KI-017); art specifically → KI-016. |
| KI-004 | blocker | Release | Shipping build not produced; `Build/build_shipping.ps1` has never been run. |
| KI-005 | major | Input | Remapping uses `UEnhancedInputUserSettings::MapPlayerKey`/`UnMapPlayerKey`; `ResetRemapsToDefault` assumes `UnMapPlayerKey` restores the IMC default — verify on 5.5. Invert-Y / sensitivity from the profile are stored but not yet applied to input (needs modifier wiring). |
| KI-006 | minor | Streaming | Fighter body/outfit meshes and AnimBP are loaded with `LoadSynchronous` (hitch risk). Move to Asset Manager async loads. |
| KI-007 | minor | Audio | `VoiceProfile` is an untyped DataTable reference until the VO pipeline defines its row struct. |
| KI-008 | major | Netcode | Sprint speed is set locally on the owning client; not server-authoritative/predicted (will rubber-band in multiplayer). |
| KI-009 | minor | Save | Save version migrations are a stub (only v1 exists). Saves are synchronous on the game thread. |
| KI-010 | minor | Rendering | `r.NGX.*` cvars in DefaultEngine.ini do nothing until the NVIDIA DLSS/Streamline plugin is added to the project. |
| KI-011 | ~~blocker~~ **closed (0003)** | Editor assets | ~~Assets must be created by hand.~~ `Content/Python/generate_afterclass_assets.py` creates all of them in one run (see `Docs/EDITOR_SETUP.md`). It has not been run yet → KI-017. |
| KI-012 | minor | Steam | `Steam/steam_input_manifest.vdf` is a labelled placeholder; `SteamDevAppId=480` (Spacewar test app) in DefaultEngine.ini. Replace with the real App ID. |
| KI-013 | minor | Data | `Archetype.*` gameplay-tag children are not defined yet (root only). |
| KI-014 | ~~minor~~ **closed (0003)** | Editor | `AfterclassEditor` now hosts `UACEditorAssetLibrary` (Python-callable helpers used by the generator). No asset validators yet. |
| KI-015 | major | Localization | **ja / ko / es-419 translations are pending.** English source strings are final for now; `Localization/Game/{ja,ko,es-419}/TRANSLATION_PENDING.csv` list every key with an empty target and `Status=PENDING`. They need professional translation — no machine/fake translations were added. Regenerate after string-table edits with `python3 Build/loc_pending.py` (`check_loc.sh` fails if out of sync). |
| KI-016 | blocker | Art / Audio | **No animation, mesh, audio or VFX art.** Move montages in `MT_Default.csv` are empty (combat timing falls back to frame data; nothing animates), no death montage/ragdoll handoff, no Niagara impacts, no VO cues on `OnHitConfirmed`, no boss telegraph flash/sting on `OnTelegraph`. Fighters use the engine mannequin if present. |
| KI-017 | blocker | Tools | **The asset generator has never been executed in a real Unreal Editor** (no UE here). It passes `py_compile` and `Tests/test_generator.py` (static consistency with C++/config/data) only; Python property names for EnhancedInput/IMC (`mappings` vs newer layouts), `PlayerMappableKeySettings`, mannequin paths and the map commandlet path are the likeliest failure points. Each section is isolated and reports OK/FAILED; manual fallback in `EDITOR_SETUP.md`. The **BT_Fighter graph nodes cannot be authored from Python** — manual step (EDITOR_SETUP §3); until then the controller runs evaluator decisions directly. |
| KI-018 | major | Combat / AI | Combat component, hit validation, AI controller/evaluator/boss phases are **untested in engine**; only the engine-independent cores (`ACFrameCore.h`, `ACEvaluatorCore.h`) and the move-table CSV are unit-tested. Damage/stun/knockback values, fighter stats (all 18 share baseline HP 1000 / Stamina 100 / Walk 450 / Sprint 700 / PreferredRange 150) and AI weights are placeholder tuning. |
| KI-019 | minor | Roster | 12th base fighter (`DA_Fighter_Fighter12`, key `Fighter.Fighter12.Name`) has no decided name; display string is `TBD`. |
