# KNOWN_ISSUES

Every `// STUB:` in source maps to an entry here. Severity: **blocker** (prevents a playable/shippable build), **major**, **minor**.

| ID | Severity | Area | Issue |
|----|----------|------|-------|
| KI-001 | blocker | Combat | `UACCombatComponent` is partial: state enum, legality order, replication of State/Health/Stamina/Meter and server-routed requests exist; **montage dispatch, hit sweeps, damage, stamina cost lookup, facing correction, dodge direction and action recovery (returning to Idle) are not implemented.** Actions currently stick in their state. |
| KI-002 | blocker | Build | **Nothing has been compiled against Unreal Engine.** The authoring environment has no UE install or GPU. Code was written against UE 5.5 APIs and passes a static convention check (`Build/sanity_check.py`) only. First Windows compile (UE 5.5 + VS2022) is expected to surface errors — report them verbatim. |
| KI-003 | blocker | Content | No characters, meshes, animations, maps, VO, UI, AI, netcode sessions or Steam integration content exist yet. |
| KI-004 | blocker | Release | Shipping build not produced; `Build/build_shipping.ps1` has never been run. |
| KI-005 | major | Input | Remapping uses `UEnhancedInputUserSettings::MapPlayerKey`/`UnMapPlayerKey`; `ResetRemapsToDefault` assumes `UnMapPlayerKey` restores the IMC default — verify on 5.5. Invert-Y / sensitivity from the profile are stored but not yet applied to input (needs modifier wiring). |
| KI-006 | minor | Streaming | Fighter body/outfit meshes and AnimBP are loaded with `LoadSynchronous` (hitch risk). Move to Asset Manager async loads. |
| KI-007 | minor | Audio | `VoiceProfile` is an untyped DataTable reference until the VO pipeline defines its row struct. |
| KI-008 | major | Netcode | Sprint speed is set locally on the owning client; not server-authoritative/predicted (will rubber-band in multiplayer). |
| KI-009 | minor | Save | Save version migrations are a stub (only v1 exists). Saves are synchronous on the game thread. |
| KI-010 | minor | Rendering | `r.NGX.*` cvars in DefaultEngine.ini do nothing until the NVIDIA DLSS/Streamline plugin is added to the project. |
| KI-011 | blocker | Editor assets | Input actions, IMC, test map, placeholder fighter BP and data asset must be created in the editor — see `Docs/EDITOR_SETUP.md`. Without them the game logs errors and the pawn cannot move. |
| KI-012 | minor | Steam | `Steam/steam_input_manifest.vdf` is a labelled placeholder; `SteamDevAppId=480` (Spacewar test app) in DefaultEngine.ini. Replace with the real App ID. |
| KI-013 | minor | Data | `Archetype.*` gameplay-tag children are not defined yet (root only). |
| KI-014 | minor | Editor | `AfterclassEditor` module is boilerplate only (no validators/tools yet). |
