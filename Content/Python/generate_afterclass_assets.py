"""Afterclass: Bloodline - one-shot, idempotent editor asset generator (UE 5.5 Python).

Run ONE of:
  * Editor:  Tools > Execute Python Script... > Content/Python/generate_afterclass_assets.py
  * Command line (assets only; the map step needs the GUI editor or may be skipped by the commandlet):
      UnrealEditor-Cmd.exe <path>/Afterclass.uproject -run=pythonscript -script="<path>/Content/Python/generate_afterclass_assets.py"

Requires the C++ modules to be compiled (Afterclass + AfterclassEditor) and the PythonScriptPlugin +
EditorScriptingUtilities plugins (enabled in Afterclass.uproject). Re-running is safe: existing assets are loaded and
updated in place, never duplicated; actors the script spawns in L_TestArena are tagged and replaced.

Every section runs in its own try/except so one failure does not abort the rest; a summary is printed at the end.
Paths must match Docs/EDITOR_SETUP.md and Source/Afterclass/Input/ACInputSettings.cpp.
NOTE: this script has NOT been executed in a real Unreal Editor yet (KNOWN_ISSUES KI-017).
"""
import os
import traceback

import unreal

# --------------------------------------------------------------------------------------------- paths / data
INPUT_DIR = "/Game/Input"
ACTIONS_DIR = "/Game/Input/Actions"
IMC_PATH = "/Game/Input/IMC_Default"
FIGHTERS_DIR = "/Game/Characters/Fighters"
PLACEHOLDER_DIR = "/Game/Characters/Placeholder"
DA_PLACEHOLDER = PLACEHOLDER_DIR + "/DA_Fighter_Placeholder"
BP_FIGHTER = PLACEHOLDER_DIR + "/BP_Fighter_Placeholder"
BP_GM_DEV = "/Game/Core/BP_GM_Dev"
AI_DIR = "/Game/AI"
BB_PATH = AI_DIR + "/BB_Fighter"
BT_PATH = AI_DIR + "/BT_Fighter"
MOVE_TABLE_DIR = "/Game/Data/MoveTables"
STRING_TABLE_DIR = "/Game/Localization/StringTables"
MAP_PATH = "/Game/Maps/Dev/L_TestArena"
GEN_TAG = "AC_Generated"

DIGITAL = ["IA_Light", "IA_Heavy", "IA_Dodge", "IA_Block", "IA_Grab", "IA_Special", "IA_Sprint"]
AXIS2D = ["IA_Move", "IA_Look"]

# (action, key, modifiers, mappable name). Modifiers: "swizzle", "negate", "negate_y", "deadzone".
BINDINGS = [
    ("IA_Move", "W", ["swizzle"], "MoveForward_KBM"),
    ("IA_Move", "S", ["swizzle", "negate"], "MoveBack_KBM"),
    ("IA_Move", "A", ["negate"], "MoveLeft_KBM"),
    ("IA_Move", "D", [], "MoveRight_KBM"),
    ("IA_Move", "Gamepad_Left2D", ["deadzone"], "Move_Pad"),
    ("IA_Look", "Mouse2D", ["negate_y"], "Look_KBM"),
    ("IA_Look", "Gamepad_Right2D", ["deadzone"], "Look_Pad"),
    ("IA_Light", "LeftMouseButton", [], "Light_KBM"),
    ("IA_Light", "Gamepad_FaceButton_Left", [], "Light_Pad"),
    ("IA_Heavy", "RightMouseButton", [], "Heavy_KBM"),
    ("IA_Heavy", "Gamepad_FaceButton_Top", [], "Heavy_Pad"),
    ("IA_Dodge", "SpaceBar", [], "Dodge_KBM"),
    ("IA_Dodge", "Gamepad_FaceButton_Bottom", [], "Dodge_Pad"),
    ("IA_Block", "Q", [], "Block_KBM"),
    ("IA_Block", "Gamepad_LeftTrigger", [], "Block_Pad"),
    ("IA_Grab", "E", [], "Grab_KBM"),
    ("IA_Grab", "Gamepad_FaceButton_Right", [], "Grab_Pad"),
    ("IA_Special", "F", [], "Special_KBM"),
    ("IA_Special", "Gamepad_RightShoulder", [], "Special_Pad"),
    ("IA_Sprint", "LeftShift", [], "Sprint_KBM"),
    ("IA_Sprint", "Gamepad_LeftThumbstick", [], "Sprint_Pad"),
]

# Roster (18). Stats are PLACEHOLDER tuning values (KNOWN_ISSUES KI-018) - identical baseline for everyone until
# per-fighter balance data exists. Display names come from ST_Characters (Fighter.<Id>.Name).
DEFAULT_STATS = dict(max_health=1000.0, max_stamina=100.0, walk_speed=450.0, sprint_speed=700.0,
                     dodge_distance=350.0, preferred_range=150.0)
ROSTER = [
    ("Akari", {}), ("Min", {}), ("Yuzuki", {}), ("Hana", {}), ("Valerie", {}), ("Kanna", {}),
    ("Reina", {}), ("Rika", {}), ("EunChae", {}), ("Nari", {}), ("Madison", {}),
    ("Fighter12", {}),  # name TBD (KI-019)
    ("Vesper", {}), ("Kuroha", {}), ("AdelVaux", {}), ("Sable", {}),  # Orchid Court
    ("DexMarrow", {"unlock": "Unlock.Chapter5Complete"}),  # boss-only, playable after Ch.5 (locked decision)
    ("YN", {"unlock": "Unlock.SeveredEnding", "anonymous": True}),  # SEVERED ending secret fighter
]

BB_KEYS = [  # must match Source/Afterclass/AI/ACBlackboardKeys.h
    ("TargetActor", "Object"), ("TargetDist", "Float"), ("TargetStamina", "Float"), ("TargetWeapon", "Bool"),
    ("TargetState", "Int"), ("LastPlayerAttack", "Int"), ("AttackPatternBias", "Float"), ("OwnHealth", "Float"),
    ("OwnStamina", "Float"), ("PreferredRange", "Float"), ("CoverPoint", "Vector"),
]

MANNEQUIN_MESHES = ["/Game/Characters/Mannequins/Meshes/SKM_Manny", "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
                    "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"]
MANNEQUIN_ANIMBPS = ["/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed", "/Game/Characters/Mannequins/Animations/ABP_Manny",
                     "/Game/Characters/Mannequins/Animations/ABP_Quinn"]

# --------------------------------------------------------------------------------------------- helpers
EAL = unreal.EditorAssetLibrary
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
SUMMARY = []  # (section, status, detail)


def log(msg):
    unreal.log("[AfterclassGen] " + msg)


def warn(msg):
    unreal.log_warning("[AfterclassGen] " + msg)


def project_content_dir():
    return os.path.abspath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))


def split_path(asset_path):
    return asset_path.rsplit("/", 1)


def find_class(name):
    """unreal.<name> if exposed, else load from /Script/Afterclass or engine modules."""
    cls = getattr(unreal, name, None)
    if cls is not None:
        return cls
    for module in ("Afterclass", "AfterclassEditor", "Engine", "EnhancedInput", "AIModule"):
        try:
            cls = unreal.load_class(None, "/Script/%s.%s" % (module, name))
            if cls:
                return cls
        except Exception:
            pass
    raise RuntimeError("class %s not found (are the C++ modules compiled?)" % name)


def get_or_create(asset_path, asset_class, factory=None):
    """Idempotent: load if it exists, otherwise create. Returns (asset, created)."""
    if EAL.does_asset_exist(asset_path):
        asset = EAL.load_asset(asset_path)
        if asset is not None:
            return asset, False
    folder, name = split_path(asset_path)
    EAL.make_directory(folder)
    asset = ASSET_TOOLS.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError("could not create %s" % asset_path)
    return asset, True


def set_prop(obj, names, value):
    """set_editor_property trying several spellings (UE strips the b prefix of bools in Python)."""
    last = None
    for n in names if isinstance(names, (list, tuple)) else [names]:
        try:
            obj.set_editor_property(n, value)
            return True
        except Exception as e:  # noqa: PERF203
            last = e
    raise RuntimeError("cannot set %s on %s: %s" % (names, obj.get_name(), last))


def load_first(paths):
    for p in paths:
        if EAL.does_asset_exist(p):
            return EAL.load_asset(p)
    return None


def make_key(name):
    try:
        return unreal.Key(key_name=name)
    except Exception:
        k = unreal.Key()
        k.import_text(name)
        return k


def make_tag(tag_name):
    tag = unreal.GameplayTag()
    tag.import_text('(TagName="%s")' % tag_name)
    return tag


def editor_lib():
    lib = getattr(unreal, "ACEditorAssetLibrary", None)
    if lib is None:
        raise RuntimeError("unreal.ACEditorAssetLibrary missing - compile the AfterclassEditor module")
    return lib


def save(asset):
    EAL.save_loaded_asset(asset, only_if_is_dirty=False)


def section(name):
    def deco(fn):
        def run():
            log("== %s" % name)
            try:
                detail = fn() or "ok"
                SUMMARY.append((name, "OK", detail))
            except Exception as e:  # keep going; report at the end
                SUMMARY.append((name, "FAILED", str(e)))
                unreal.log_error("[AfterclassGen] %s failed: %s\n%s" % (name, e, traceback.format_exc()))
        run.__name__ = fn.__name__
        return run
    return deco


# --------------------------------------------------------------------------------------------- sections
@section("String tables")
def gen_string_tables():
    src = os.path.join(project_content_dir(), "Localization", "StringTables")
    lib = editor_lib()
    done = []
    for csv_name in sorted(f for f in os.listdir(src) if f.endswith(".csv")):
        table_name = os.path.splitext(csv_name)[0]
        table, _ = get_or_create("%s/%s" % (STRING_TABLE_DIR, table_name), unreal.StringTable)
        if not lib.import_string_table_from_csv(table, os.path.join(src, csv_name)):
            raise RuntimeError("import failed for " + csv_name)
        save(table)
        done.append(table_name)
    # Runtime code registers the same CSVs as ST_UI/ST_Characters/ST_Combat (ACText::RegisterStringTables); these
    # assets are editor conveniences for browsing/editing and for the localization gather.
    return ", ".join(done)


@section("Move tables (DataTable from CSV)")
def gen_move_tables():
    src = os.path.join(project_content_dir(), "Data", "MoveTables")
    row_struct = unreal.load_object(None, "/Script/Afterclass.ACMoveRow")
    if row_struct is None:
        raise RuntimeError("row struct /Script/Afterclass.ACMoveRow not found (compile Afterclass)")
    EAL.make_directory(MOVE_TABLE_DIR)
    tasks = []
    for csv_name in sorted(f for f in os.listdir(src) if f.endswith(".csv")):
        factory = unreal.CSVImportFactory()
        settings = factory.get_editor_property("automated_import_settings")
        settings.set_editor_property("import_row_struct", row_struct)
        settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)
        factory.set_editor_property("automated_import_settings", settings)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(src, csv_name))
        task.set_editor_property("destination_path", MOVE_TABLE_DIR)
        task.set_editor_property("destination_name", os.path.splitext(csv_name)[0])
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("factory", factory)
        tasks.append(task)
    ASSET_TOOLS.import_asset_tasks(tasks)
    names = [t.get_editor_property("destination_name") for t in tasks]
    missing = [n for n in names if not EAL.does_asset_exist("%s/%s" % (MOVE_TABLE_DIR, n))]
    if missing:
        raise RuntimeError("import produced no asset for " + ", ".join(missing))
    return ", ".join(names)


@section("Input actions")
def gen_input_actions():
    for name in DIGITAL + AXIS2D:
        ia, _ = get_or_create("%s/%s" % (ACTIONS_DIR, name), unreal.InputAction)
        vt = unreal.InputActionValueType.AXIS2D if name in AXIS2D else unreal.InputActionValueType.BOOLEAN
        set_prop(ia, "value_type", vt)
        save(ia)
    return "%d actions" % (len(DIGITAL) + len(AXIS2D))


def _modifier(kind, outer):
    if kind == "swizzle":
        m = unreal.new_object(unreal.InputModifierSwizzleAxis, outer)
        set_prop(m, "order", unreal.InputAxisSwizzle.YXZ)
        return m
    if kind in ("negate", "negate_y"):
        m = unreal.new_object(unreal.InputModifierNegate, outer)
        if kind == "negate_y":
            set_prop(m, ["x", "b_x", "bX"], False)
            set_prop(m, ["y", "b_y", "bY"], True)
            set_prop(m, ["z", "b_z", "bZ"], False)
        return m
    if kind == "deadzone":
        return unreal.new_object(unreal.InputModifierDeadZone, outer)
    raise ValueError(kind)


def _set_mappings(imc, mappings):
    try:
        imc.set_editor_property("mappings", mappings)
        return
    except Exception:
        pass
    data = imc.get_editor_property("default_key_mappings")  # newer engine layout
    data.set_editor_property("mappings", mappings)
    imc.set_editor_property("default_key_mappings", data)


@section("Input mapping context")
def gen_imc():
    imc, _ = get_or_create(IMC_PATH, unreal.InputMappingContext)
    mappings = []
    mappable = 0
    for action_name, key_name, mods, map_name in BINDINGS:
        action = EAL.load_asset("%s/%s" % (ACTIONS_DIR, action_name))
        if action is None:
            raise RuntimeError("missing " + action_name)
        m = unreal.EnhancedActionKeyMapping()
        m.set_editor_property("action", action)
        m.set_editor_property("key", make_key(key_name))
        m.set_editor_property("modifiers", [_modifier(k, imc) for k in mods])
        try:  # remappable via AACPlayerController::RemapKey(MappingName)
            m.set_editor_property("setting_behavior", unreal.PlayerMappableKeySettingBehaviors.OVERRIDE_SETTINGS)
            pmks = unreal.new_object(unreal.PlayerMappableKeySettings, imc)
            pmks.set_editor_property("name", map_name)
            m.set_editor_property("player_mappable_key_settings", pmks)
            mappable += 1
        except Exception as e:
            warn("mappable key settings unavailable for %s: %s" % (map_name, e))
        mappings.append(m)
    _set_mappings(imc, mappings)
    save(imc)
    return "%d mappings (%d remappable)" % (len(mappings), mappable)


def _fill_fighter(da, fighter_id, opts, move_set, mesh, anim_class):
    lib = editor_lib()
    if not lib.set_text_property_from_string_table(da, "DisplayName", "ST_Characters", "Fighter.%s.Name" % fighter_id):
        raise RuntimeError("DisplayName not set for " + fighter_id)
    for prop, value in DEFAULT_STATS.items():
        set_prop(da, prop, value)
    set_prop(da, "unlock_condition", make_tag(opts.get("unlock", "Unlock.Default")))
    set_prop(da, ["anonymous_passive", "b_anonymous_passive", "bAnonymousPassive"], bool(opts.get("anonymous", False)))
    if move_set is not None:
        set_prop(da, "move_set", move_set)
    if mesh is not None:
        set_prop(da, "body_mesh", mesh)
    if anim_class is not None:
        set_prop(da, ["anim_bp", "AnimBP"], anim_class)
    save(da)


@section("Character data assets")
def gen_fighters():
    da_class = find_class("ACCharacterDataAsset")
    move_set = EAL.load_asset(MOVE_TABLE_DIR + "/MT_Default") if EAL.does_asset_exist(MOVE_TABLE_DIR + "/MT_Default") else None
    mesh = load_first(MANNEQUIN_MESHES)
    anim_bp = load_first(MANNEQUIN_ANIMBPS)
    anim_class = anim_bp.generated_class() if anim_bp is not None else None
    if mesh is None:
        warn("engine mannequin not found (Content > Add Feature > Third Person) - BodyMesh/AnimBP left empty")

    def factory():
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", da_class)
        return f

    created = 0
    for fighter_id, opts in ROSTER:
        da, new = get_or_create("%s/DA_Fighter_%s" % (FIGHTERS_DIR, fighter_id), da_class, factory())
        created += int(new)
        _fill_fighter(da, fighter_id, opts, move_set, mesh, anim_class)
    placeholder, _ = get_or_create(DA_PLACEHOLDER, da_class, factory())
    _fill_fighter(placeholder, "Placeholder", {}, move_set, mesh, anim_class)
    return "%d roster assets (%d new) + placeholder; mannequin=%s" % (len(ROSTER), created, "yes" if mesh else "no")


def _blueprint(path, parent):
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", parent)
    bp, _ = get_or_create(path, unreal.Blueprint, f)
    return bp


def _compile(bp):
    lib = getattr(unreal, "BlueprintEditorLibrary", None)
    if lib is not None:  # EditorScriptingUtilities; otherwise the editor compiles the BP on next load
        lib.compile_blueprint(bp)
    save(bp)


@section("Placeholder fighter Blueprint")
def gen_fighter_bp():
    bp = _blueprint(BP_FIGHTER, find_class("ACFighter"))
    _compile(bp)  # make sure generated_class exists before touching the CDO
    cdo = unreal.get_default_object(bp.generated_class())
    da = EAL.load_asset(DA_PLACEHOLDER)
    if da is not None:
        set_prop(cdo, "character_data", da)
    mesh_comp = cdo.get_editor_property("mesh")
    set_prop(mesh_comp, "relative_location", unreal.Vector(0.0, 0.0, -96.0))
    set_prop(mesh_comp, "relative_rotation", unreal.Rotator(0.0, 0.0, -90.0))  # Rotator(roll, pitch, yaw) -> yaw -90
    mesh = load_first(MANNEQUIN_MESHES)
    if mesh is not None:  # editor preview only; ApplyCharacterData sets the mesh at runtime from the data asset
        try:
            set_prop(mesh_comp, ["skeletal_mesh_asset", "skeletal_mesh"], mesh)
        except Exception as e:
            warn("preview mesh not set: %s" % e)
    _compile(bp)
    return "parent ACFighter, CharacterData=%s" % ("DA_Fighter_Placeholder" if da else "unset")


@section("Dev game mode Blueprint")
def gen_game_mode():
    bp = _blueprint(BP_GM_DEV, find_class("ACGameMode"))
    _compile(bp)
    fighter = EAL.load_asset(BP_FIGHTER)
    if fighter is not None:
        set_prop(unreal.get_default_object(bp.generated_class()), "default_pawn_class", fighter.generated_class())
    _compile(bp)
    return "DefaultPawn=%s" % ("BP_Fighter_Placeholder" if fighter else "unset")


@section("Blackboard + Behavior Tree")
def gen_ai():
    lib = editor_lib()
    kinds = unreal.ACBlackboardKeyKind
    bb, _ = get_or_create(BB_PATH, unreal.BlackboardData)
    for key, kind in BB_KEYS:
        enum = {"Float": kinds.FLOAT, "Int": kinds.INT, "Bool": kinds.BOOL, "Vector": kinds.VECTOR, "Object": kinds.OBJECT}[kind]
        if not lib.set_blackboard_key(bb, key, enum, unreal.Actor if kind == "Object" else None):
            raise RuntimeError("blackboard key %s failed" % key)
    save(bb)
    bt, _ = get_or_create(BT_PATH, unreal.BehaviorTree)
    lib.set_behavior_tree_blackboard(bt, bb)
    save(bt)
    # The BT *graph* (Selector + AC Update Combat Blackboard service + AC Execute Combat Decision task) cannot be
    # authored from Python; see Docs/EDITOR_SETUP.md section 3. Until it is, AACEnemyController detects the empty
    # tree and runs the evaluator directly (same decisions, no BT).
    return "%d keys; BT graph nodes are a manual step (EDITOR_SETUP section 3)" % len(BB_KEYS)


def _spawn(eas, cls_or_asset, label, location, rotation=None, scale=None):
    rot = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    if isinstance(cls_or_asset, unreal.Object) and not isinstance(cls_or_asset, unreal.Class):
        actor = eas.spawn_actor_from_object(cls_or_asset, location, rot)
    else:
        actor = eas.spawn_actor_from_class(cls_or_asset, location, rot)
    if actor is None:
        raise RuntimeError("spawn failed: " + label)
    actor.set_actor_label(label)
    actor.set_editor_property("tags", [unreal.Name(GEN_TAG)])
    if scale is not None:
        actor.set_actor_scale3d(scale)
    return actor


def _world_settings(eas):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    try:
        return world.get_world_settings()
    except Exception:
        for actor in eas.get_all_level_actors():
            if isinstance(actor, unreal.WorldSettings):
                return actor
    raise RuntimeError("WorldSettings not found")


@section("Greybox map L_TestArena")
def gen_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if EAL.does_asset_exist(MAP_PATH):
        if not les.load_level(MAP_PATH):
            raise RuntimeError("could not load " + MAP_PATH)
    else:
        EAL.make_directory(split_path(MAP_PATH)[0])
        if not les.new_level(MAP_PATH):
            raise RuntimeError("could not create " + MAP_PATH + " (run from the GUI editor if using the commandlet)")

    removed = 0
    for actor in eas.get_all_level_actors():
        if GEN_TAG in [str(t) for t in actor.get_editor_property("tags")]:
            eas.destroy_actor(actor)
            removed += 1

    plane = EAL.load_asset("/Engine/BasicShapes/Plane")
    cube = EAL.load_asset("/Engine/BasicShapes/Cube")
    floor = _spawn(eas, plane, "AC_Floor", unreal.Vector(0, 0, 0), scale=unreal.Vector(40, 40, 1))  # 40 m x 40 m
    walls = 0
    for i, (x, y, sx, sy) in enumerate([(2000, 0, 0.5, 40), (-2000, 0, 0.5, 40), (0, 2000, 40, 0.5), (0, -2000, 40, 0.5)]):
        _spawn(eas, cube, "AC_Wall_%d" % i, unreal.Vector(x, y, 150), scale=unreal.Vector(sx, sy, 3))
        walls += 1
    for i, (x, y) in enumerate([(600, 600), (-700, 300), (300, -800)]):  # cover blocks
        _spawn(eas, cube, "AC_Cover_%d" % i, unreal.Vector(x, y, 75), scale=unreal.Vector(1.5, 1.5, 1.5))
    sun = _spawn(eas, unreal.DirectionalLight, "AC_Sun", unreal.Vector(0, 0, 1000), unreal.Rotator(0, -45, 30))
    try:
        sun.light_component.set_editor_property("atmosphere_sun_light", True)
    except Exception:
        pass
    _spawn(eas, unreal.SkyLight, "AC_SkyLight", unreal.Vector(0, 0, 800))
    _spawn(eas, unreal.SkyAtmosphere, "AC_SkyAtmosphere", unreal.Vector(0, 0, 0))
    _spawn(eas, unreal.ExponentialHeightFog, "AC_Fog", unreal.Vector(0, 0, 0))
    _spawn(eas, unreal.PlayerStart, "AC_PlayerStart", unreal.Vector(0, -600, 100), unreal.Rotator(0, 0, 90))
    _spawn(eas, unreal.NavMeshBoundsVolume, "AC_NavBounds", unreal.Vector(0, 0, 100), scale=unreal.Vector(20, 20, 3))
    fighter_bp = EAL.load_asset(BP_FIGHTER)
    if fighter_bp is not None:  # AI-possessed sparring partner (AutoPossessAI = PlacedInWorldOrSpawned)
        _spawn(eas, fighter_bp.generated_class(), "AC_Enemy_Placeholder", unreal.Vector(0, 600, 100), unreal.Rotator(0, 0, -90))
    gm = EAL.load_asset(BP_GM_DEV)
    if gm is not None:
        set_prop(_world_settings(eas), "default_game_mode", gm.generated_class())
    if floor is None or not les.save_current_level():
        raise RuntimeError("saving the level failed")
    return "replaced %d generated actors; floor, %d walls, 3 cover, lights, PlayerStart, NavMesh, enemy=%s" % (
        removed, walls, "yes" if fighter_bp else "no")


# --------------------------------------------------------------------------------------------- main
def main():
    log("Afterclass asset generator starting (content dir: %s)" % project_content_dir())
    for step in (gen_string_tables, gen_move_tables, gen_input_actions, gen_imc, gen_fighters,
                 gen_fighter_bp, gen_game_mode, gen_ai, gen_map):
        step()
    try:
        EAL.save_directory("/Game", only_if_is_dirty=True, recursive=True)
    except Exception as e:
        warn("save_directory: %s" % e)
    log("================ SUMMARY ================")
    failed = 0
    for name, status, detail in SUMMARY:
        (unreal.log_error if status != "OK" else unreal.log)("[AfterclassGen] %-34s %-6s %s" % (name, status, detail))
        failed += status != "OK"
    log("%d/%d sections OK" % (len(SUMMARY) - failed, len(SUMMARY)))
    return failed == 0


if __name__ == "__main__":
    main()
