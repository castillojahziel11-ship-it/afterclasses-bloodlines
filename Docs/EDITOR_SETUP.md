# EDITOR_SETUP — create every editor asset with one script

C++ cannot write `.uasset` files outside the editor, so the assets the code expects are produced **in the editor** by
`Content/Python/generate_afterclass_assets.py`. Names and paths are **exactly** what the code expects (defaults in
`Source/Afterclass/Input/ACInputSettings.cpp`, `Source/Afterclass/AI/ACEnemyController.cpp`, `Config/DefaultEngine.ini`).

> Status: the script passes `python3 -m py_compile` and static checks but has **not yet been executed in a real Unreal
> Editor** (KNOWN_ISSUES KI-017). If a section fails, the summary at the end of the log names it; use the matching
> manual fallback below for that section only.

## 0. Prerequisites
1. Compile the project once (Afterclass + AfterclassEditor modules). The script uses reflected C++ types
   (`ACCharacterDataAsset`, `ACFighter`, `ACGameMode`, `/Script/Afterclass.ACMoveRow`) and the editor helper library
   `UACEditorAssetLibrary` (string-table CSV import, blackboard keys, BT blackboard, FText from string table).
2. `PythonScriptPlugin` and `EditorScriptingUtilities` are already enabled in `Afterclass.uproject`.
3. Optional but recommended: *Content Drawer → Add → Add Feature or Content Pack → Third Person* so the engine mannequin
   (`/Game/Characters/Mannequins/...`) exists. Without it the fighters get no BodyMesh/AnimBP (the log warns; rerun later).

## 1. Run the script (pick one)
- **Editor GUI (recommended):** *Tools → Execute Python Script…* → select `Content/Python/generate_afterclass_assets.py`.
- **Command line:**
  ```
  UnrealEditor-Cmd.exe "D:\Path\Afterclass\Afterclass.uproject" -run=pythonscript -script="D:\Path\Afterclass\Content\Python\generate_afterclass_assets.py"
  ```
  The asset sections work headless; if the map section fails under the commandlet, open the editor and run the script
  once from the GUI (re-running is safe).

The script is **idempotent**: existing assets are loaded and updated in place; actors it spawns in the map carry the
actor tag `AC_Generated` and are replaced on every run (anything you add by hand is left alone).
The Output Log ends with `[AfterclassGen] ==== SUMMARY ====` and one `OK`/`FAILED` line per section.

## 2. What it creates

| Section | Asset(s) | Details |
|---------|----------|---------|
| String tables | `/Game/Localization/StringTables/ST_UI`, `ST_Characters`, `ST_Combat` | Imported from the CSVs in the same folder. Runtime code registers the same CSVs as table ids `ST_UI`/`ST_Characters`/`ST_Combat` (`ACText::RegisterStringTables`), so these assets are for browsing/editing only. |
| Move tables | `/Game/Data/MoveTables/MT_Default` | DataTable, row struct `ACMoveRow`, imported from `Content/Data/MoveTables/*.csv` (spec §7 frame data). Re-import replaces. |
| Input actions | `/Game/Input/Actions/IA_*` | `IA_Move`, `IA_Look` = Axis2D; `IA_Light`, `IA_Heavy`, `IA_Dodge`, `IA_Block`, `IA_Grab`, `IA_Special`, `IA_Sprint` = Digital (bool). |
| Mapping context | `/Game/Input/IMC_Default` | KBM + gamepad bindings and modifiers from the table in §A2, each with *Player Mappable Key Settings* named e.g. `Light_KBM` / `Light_Pad` (the `MappingName` for `AACPlayerController::RemapKey`). |
| Fighters | `/Game/Characters/Fighters/DA_Fighter_<Id>` ×18 + `/Game/Characters/Placeholder/DA_Fighter_Placeholder` | `ACCharacterDataAsset`; DisplayName = string-table reference `ST_Characters` / `Fighter.<Id>.Name`; baseline stats (HP 1000, Stamina 100, Walk 450, Sprint 700, Dodge 350, PreferredRange 150 — placeholders, KI-018); MoveSet = MT_Default; mannequin mesh/AnimBP if present. Unlocks: `Unlock.Default`, except DexMarrow = `Unlock.Chapter5Complete` and YN = `Unlock.SeveredEnding` + `bAnonymousPassive`. |
| Fighter BP | `/Game/Characters/Placeholder/BP_Fighter_Placeholder` | Child of `ACFighter`; CharacterData = DA_Fighter_Placeholder; Mesh Z −96, Yaw −90. No gameplay logic in the BP. |
| Game mode BP | `/Game/Core/BP_GM_Dev` | Child of `ACGameMode`; DefaultPawnClass = BP_Fighter_Placeholder. |
| AI | `/Game/AI/BB_Fighter`, `/Game/AI/BT_Fighter` | Blackboard keys: TargetActor (Object/Actor), TargetDist, TargetStamina, AttackPatternBias, OwnHealth, OwnStamina, PreferredRange (Float), TargetWeapon (Bool), TargetState, LastPlayerAttack (Int), CoverPoint (Vector). BT_Fighter uses BB_Fighter; **its graph nodes are a manual step (§3)**. |
| Map | `/Game/Maps/Dev/L_TestArena` | 40 m floor, 4 walls, 3 cover blocks, DirectionalLight, SkyLight, SkyAtmosphere, height fog, PlayerStart, NavMeshBoundsVolume, one AI-possessed `BP_Fighter_Placeholder`; World Settings GameMode Override = BP_GM_Dev. |

## 3. Behavior Tree graph (manual — Python cannot author BT graph nodes)
Open `/Game/AI/BT_Fighter`:
1. From **ROOT** drag out a **Selector**.
2. Right-click the Selector → *Add Service* → **AC Update Combat Blackboard** (`UACBTService_UpdateCombatBlackboard`).
3. Under the Selector add task **AC Execute Combat Decision** (`UACBTTask_ExecuteDecision`).
4. Save.

Until this is done `AACEnemyController` sees the empty tree (no root node), logs a warning and runs the evaluator's
decisions directly — the AI still fights, just without a BT.

## 4. Verify
Press Play in `L_TestArena`: the Output Log shows `Afterclass: Bloodline <version> (build <n>, Development)` and **no**
`Input action ... missing` errors. WASD/left stick moves, mouse/right stick looks, Left Shift/L3 sprints, the placeholder
enemy engages. Save files appear in `Saved/SaveGames/Afterclass/Profile.acsav`.

---

# Appendix — manual fallback (only for sections the script reported as FAILED)

If you put assets elsewhere, update **Project Settings → Game → Afterclass Input**.

## A1. Input Actions — `/Game/Input/Actions/`
Content Browser → right-click → *Input → Input Action*.

| Asset name | Value Type | Notes |
|------------|-----------|-------|
| `IA_Move` | Axis2D (Vector2D) | X = right, Y = forward |
| `IA_Look` | Axis2D (Vector2D) | X = yaw, Y = pitch |
| `IA_Light` | Digital (bool) | |
| `IA_Heavy` | Digital (bool) | |
| `IA_Dodge` | Digital (bool) | |
| `IA_Block` | Digital (bool) | held; code binds Started + Completed |
| `IA_Grab` | Digital (bool) | |
| `IA_Special` | Digital (bool) | |
| `IA_Sprint` | Digital (bool) | held; code binds Started + Completed |

## A2. Input Mapping Context — `/Game/Input/IMC_Default`
*Input → Input Mapping Context*, named `IMC_Default` (keyboard/mouse + gamepad in the same IMC):

| Action | Keyboard / Mouse | Gamepad | Modifiers |
|--------|------------------|---------|-----------|
| IA_Move | W / S / A / D | Left Thumbstick 2D-Axis | W: *Swizzle Input Axis Values (YXZ)*; S: *Swizzle* + *Negate*; A: *Negate*; stick: *Dead Zone* |
| IA_Look | Mouse XY 2D-Axis | Right Thumbstick 2D-Axis | Mouse: *Negate (Y only)*; stick: *Dead Zone* |
| IA_Light | Left Mouse Button | Face Button Left (X / □) | |
| IA_Heavy | Right Mouse Button | Face Button Top (Y / △) | |
| IA_Dodge | Space Bar | Face Button Bottom (A / ✕) | |
| IA_Block | Q | Left Trigger | |
| IA_Grab | E | Face Button Right (B / ○) | |
| IA_Special | F | Right Shoulder (RB / R1) | |
| IA_Sprint | Left Shift | Left Thumbstick Button (L3) | |

**Remapping:** for each mapping set *Settings Behavior = Override Settings*, then add *Player Mappable Key Settings* with a
unique **Name** (`MoveForward_KBM`, `MoveBack_KBM`, `MoveLeft_KBM`, `MoveRight_KBM`, `Move_Pad`, `Look_KBM`, `Look_Pad`,
`Light_KBM`, `Light_Pad`, … `Sprint_Pad`). That Name is the `MappingName` passed to `AACPlayerController::RemapKey`.

## A3. String tables / move table
- String tables are optional in the editor (runtime registers the CSVs). To create them: *Miscellaneous → String Table*
  at `/Game/Localization/StringTables/ST_*`, open it → *Import from CSV* → pick the matching CSV.
- Move table: drag `Content/Data/MoveTables/MT_Default.csv` into `/Game/Data/MoveTables/`, choose
  *DataTable*, Row Type **ACMoveRow**.

## A4. Fighter data assets
*Miscellaneous → Data Asset → ACCharacterDataAsset*, under `/Game/Characters/` (Asset Manager scan path for primary
type `Fighter`): `/Game/Characters/Fighters/DA_Fighter_<Id>` for Akari, Min, Yuzuki, Hana, Valerie, Kanna, Reina, Rika,
EunChae, Nari, Madison, Fighter12, Vesper, Kuroha, AdelVaux, Sable, DexMarrow, YN, plus
`/Game/Characters/Placeholder/DA_Fighter_Placeholder`. DisplayName: pick *String Table* `ST_Characters`, key
`Fighter.<Id>.Name` (never a literal). Other fields as in §2.

## A5. Placeholder fighter Blueprint — `/Game/Characters/Placeholder/BP_Fighter_Placeholder`
*Blueprint Class → All Classes → ACFighter*. Class Defaults: **Character Data = DA_Fighter_Placeholder**; Mesh component
Location Z = −96, Rotation Yaw = −90. No gameplay logic in the BP (spec bans Blueprint-only architecture).

## A6. Dev game mode Blueprint — `/Game/Core/BP_GM_Dev`
*Blueprint Class → ACGameMode*, **Default Pawn Class = BP_Fighter_Placeholder**. Set it as the World Settings GameMode
Override in `L_TestArena`, or change `GlobalDefaultGameMode` to `/Game/Core/BP_GM_Dev.BP_GM_Dev_C`.

## A7. Blackboard / Behavior Tree
*Artificial Intelligence → Blackboard* `/Game/AI/BB_Fighter` with the keys in §2; *Artificial Intelligence → Behavior
Tree* `/Game/AI/BT_Fighter`, Blackboard Asset = BB_Fighter; then §3.

## A8. Test map — `/Game/Maps/Dev/L_TestArena`
*File → New Level → Basic*, save as `/Game/Maps/Dev/L_TestArena`; add a floor, PlayerStart, NavMeshBoundsVolume, a few
blocking cubes and optionally a `BP_Fighter_Placeholder` as sparring partner. `Config/DefaultEngine.ini` already points
`GameDefaultMap` and `EditorStartupMap` here.
