# EDITOR_SETUP — assets you must create in the Unreal Editor

C++ cannot create `.uasset` files outside the editor, so these must be authored once after the first successful compile.
Names and paths below are **exactly** what the code expects (defaults in `Source/Afterclass/Input/ACInputSettings.cpp`).
If you put them elsewhere, update **Project Settings → Game → Afterclass Input**.

## 1. Input Actions — `/Game/Input/Actions/`
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

## 2. Input Mapping Context — `/Game/Input/IMC_Default`
*Input → Input Mapping Context*, named `IMC_Default`. Suggested defaults (keyboard/mouse + gamepad in the same IMC):

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

**Remapping:** for each mapping set *Settings Behavior = Override Settings*, then add *Player Mappable Key Settings* with a unique **Name** (e.g. `Light_KBM`, `Light_Pad`). That Name is the `MappingName` passed to `AACPlayerController::RemapKey`. Mappings without Player Mappable Key Settings cannot be remapped.

## 3. Test map — `/Game/Maps/Dev/L_TestArena`
*File → New Level → Basic*, save as `/Game/Maps/Dev/L_TestArena`. Add a floor, a PlayerStart and a few blocking cubes.
`Config/DefaultEngine.ini` already points `GameDefaultMap` and `EditorStartupMap` here, and `GlobalDefaultGameMode` at `/Script/Afterclass.ACGameMode`.

## 4. Fighter data asset — `/Game/Characters/Placeholder/DA_Fighter_Placeholder`
*Miscellaneous → Data Asset → ACCharacterDataAsset*. It must live under `/Game/Characters/` (Asset Manager scan path for primary type `Fighter`).
Fill: DisplayName (from a String Table, not literal), BodyMesh = `SKM_Manny` (enable *Content → Add Feature → Third Person* for the mannequin, or any skeletal mesh), AnimBP = `ABP_Manny` (or `ABP_Unarmed`), WalkSpeed 450, SprintSpeed 700, MaxHealth 1000, MaxStamina 100, UnlockCondition = `Unlock.Default`.
For **Dex Marrow** later: UnlockCondition = `Unlock.Chapter5Complete` (boss-only until Ch.5 completion — locked decision).

## 5. Placeholder fighter Blueprint — `/Game/Characters/Placeholder/BP_Fighter_Placeholder`
*Blueprint Class → All Classes → ACFighter*. In Class Defaults set **Character Data = DA_Fighter_Placeholder**. Do not add gameplay logic in the BP (spec bans Blueprint-only architecture; BPs are data/config only).
Mesh offset: Mesh component Location Z = -96, Rotation Yaw = -90 (standard mannequin alignment).

## 6. Dev game mode Blueprint — `/Game/Core/BP_GM_Dev`
*Blueprint Class → ACGameMode*, set **Default Pawn Class = BP_Fighter_Placeholder**. Either set it as the World Settings GameMode Override in `L_TestArena`, or change `GlobalDefaultGameMode` to `/Game/Core/BP_GM_Dev.BP_GM_Dev_C`.

## 7. Verify
Press Play in `L_TestArena`: the Output Log should show `Afterclass: Bloodline 0.1.0 (build 0000, Development)` and **no** `Input action ... missing` errors. WASD/left stick moves, mouse/right stick looks, Left Shift/L3 sprints. Save files appear in `Saved/SaveGames/Afterclass/Profile.acsav`.
