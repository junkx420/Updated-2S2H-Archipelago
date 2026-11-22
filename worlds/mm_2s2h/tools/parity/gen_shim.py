"""Prepare the C++ parity harness: compile 2S2H's *real* region logic against a mocked save file.

Usage: python gen_shim.py <path-to-2s2h-checkout> <build-dir>

Writes into <build-dir>:
  consts.h          every game constant the logic uses (ITEM_*, ACTOR_*, SCENE_*, ...) + name->value table.
                    Real values where the logic does arithmetic/ordering on them (ITEM_, QUEST_, OCARINA_SONG_,
                    EQUIP_VALUE_), arbitrary distinct values elsewhere.
  logic_body.inc    Logic.h from `namespace Rando {` to the end, verbatim (macros, canPlaySong, CanKillEnemy, ...)
  logic_cpp.inc     Logic.cpp body, verbatim (GetRegionIdFromEntrance, FindReachableRegions, RR_MAX)
  timelogic.inc     TimeLogic.cpp body, verbatim
  souls.inc         enemySoulMap and NPC soul rules from Souls.cpp, verbatim
  abilities.inc, doors.inc, extra_cpp.inc, ExtraChecks.inc  fork tables and their evaluators
  entrances.inc     shuffle tables, save pairing and warp composition from EntranceShuffle.cpp
  regions/*.cpp     copies of Logic/Regions/*.cpp with the same ability header used by the client
"""
import pathlib
import re
import shutil
import sys

root = pathlib.Path(sys.argv[1]).resolve() / "mm"
out = pathlib.Path(sys.argv[2]).resolve()
(out / "regions").mkdir(parents=True, exist_ok=True)
rando = root / "2s2h" / "Rando"
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from gen_from_2s2h import parse_enums, strip_comments  # noqa: E402


def body_from(text: str, start: str, end_marker: str | None = None) -> str:
    i = text.index(start)
    j = text.rindex(end_marker) if end_marker else len(text)
    return text[i:j]


logic_h = (rando / "Logic" / "Logic.h").read_text(encoding="utf-8")
body = body_from(logic_h, "namespace Rando {", "#endif // RANDO_LOGIC_H")
(out / "logic_body.inc").write_text(body, encoding="utf-8")
for name, src in (("logic_cpp.inc", "Logic.cpp"), ("timelogic.inc", "TimeLogic.cpp")):
    t = (rando / "Logic" / src).read_text(encoding="utf-8")
    (out / name).write_text(body_from(t, "namespace Rando {"), encoding="utf-8")
souls = (rando / "ActorBehavior" / "Souls.cpp").read_text(encoding="utf-8")
m = re.search(r"std::unordered_map<int16_t, RandoItemId> enemySoulMap = \{.*?\};", souls, re.S)
npc_rules = souls[souls.index("struct NpcSoul {"):souls.index("struct NpcSoulException {")]
(out / "souls.inc").write_text(m.group(0) + "\n" + npc_rules, encoding="utf-8")
abilities = (rando / "MiscBehavior" / "Abilities.cpp").read_text(encoding="utf-8")
(out / "abilities.inc").write_text(
    abilities[abilities.index("namespace Rando::Abilities {"):abilities.index("void RegisterHooks() {")] + "}\n",
    encoding="utf-8")
for name, source in (("doors.inc", "MiscBehavior/DoorKeys.cpp"), ("extra_cpp.inc", "Logic/ExtraChecks.cpp")):
    code = (rando / source).read_text(encoding="utf-8")
    (out / name).write_text(body_from(code, "namespace Rando"), encoding="utf-8")
entrances = (rando / "MiscBehavior" / "EntranceShuffle.cpp").read_text(encoding="utf-8")
tables = entrances[entrances.index("// Time macros"):entrances.index("const std::vector<GrottoEntrance>")]
pairing = entrances[entrances.index("const std::vector<ShuffleAreaRule>"):
                    entrances.index("static bool pendingCycleSpawnLoad")]
set_pairs = entrances[entrances.index("void SetPairs("):entrances.index("// The clear flags")]
spawn_save = entrances[entrances.index("void SetCycleSpawn("):entrances.index("void QueueCycleSpawnOnLoad()")]
entrance_logic = "namespace Rando::EntranceShuffle {\n" + tables + pairing + spawn_save + set_pairs + "}\n"
(out / "entrances.inc").write_text(entrance_logic, encoding="utf-8")
shutil.copy(rando / "Logic" / "ExtraChecks.inc", out / "ExtraChecks.inc")
for f in (rando / "Logic" / "Regions").glob("*.cpp"):
    (out / "regions" / f.name).write_text(
        '#include "2s2h/Rando/Logic/TrackerAbilityMacros.h"\n' + f.read_text(encoding="utf-8"), encoding="utf-8")

# ---- constants -------------------------------------------------------------------------------
sources = "".join(p.read_text(encoding="utf-8") for p in
                  [rando / "Logic" / "Logic.h", rando / "Logic" / "Logic.cpp", rando / "Logic" / "TimeLogic.cpp", rando / "ActorBehavior" / "Souls.cpp"]
                  + [rando / "Logic" / "ExtraChecks.inc", rando / "MiscBehavior" / "DoorKeys.cpp"]
                  + list((rando / "Logic" / "Regions").glob("*.cpp")))
prefixes = ["ITEM", "ACTOR", "SCENE", "QUEST", "OWL_WARP", "DUNGEON_SCENE_INDEX", "EQUIP_TYPE", "EQUIP_VALUE", "UPG",
            "WEEKEVENTREG", "OCARINA_SONG", "PLAYER_FORM", "DUNGEON", "RESPAWN_MODE"]
used = {n for n in re.findall(r"\b(?:%s)_[A-Z0-9_]+\b" % "|".join(prefixes), sources + entrance_logic)
        if not n.endswith("_")}


def enum_block(text: str, start_pat: str, end_pat: str) -> list[str]:
    block = re.search(start_pat + r"(.*?)" + end_pat, text, re.S).group(1)
    block = re.sub(r"/\*.*?\*/|//[^\n]*", "", block, flags=re.S)
    names = []
    for e in block.split(","):
        e = e.strip()
        if e:
            names.append(e.split("=")[0].strip())
    return names


z64item = (root / "include" / "z64item.h").read_text(encoding="utf-8")
real = {}
scene_h = (root / "include" / "z64scene.h").read_text(encoding="utf-8")
real.update(parse_enums(strip_comments(scene_h))["EntranceSceneId"])
used.update(n for n in real if n.startswith("ENTR_SCENE_"))
for i, n in enumerate(enum_block(z64item, r"typedef enum ItemId \{", r"\} ItemId;")):
    real[n] = i
ocarina = (root / "include" / "z64ocarina.h").read_text(encoding="utf-8")
songs = re.findall(r"\b(OCARINA_SONG_[A-Z0-9_]+)\b", ocarina[ocarina.index("OCARINA_SONG_SONATA,"):ocarina.index("OCARINA_SONG_MAX")])
for i, n in enumerate(dict.fromkeys(songs)):
    real[n] = i
real.update({"EQUIP_VALUE_SWORD_NONE": 0, "EQUIP_VALUE_SWORD_KOKIRI": 1, "EQUIP_VALUE_SWORD_RAZOR": 2,
             "EQUIP_VALUE_SWORD_GILDED": 3, "EQUIP_VALUE_SWORD_DEITY": 4, "EQUIP_VALUE_SHIELD_NONE": 0,
             "EQUIP_VALUE_SHIELD_HERO": 1, "EQUIP_VALUE_SHIELD_MIRROR": 2,
             "DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE": 0, "DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE": 1,
             "DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE": 2, "DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE": 3,
             "PLAYER_FORM_FIERCE_DEITY": 0, "PLAYER_FORM_GORON": 1, "PLAYER_FORM_ZORA": 2, "PLAYER_FORM_DEKU": 3,
             "PLAYER_FORM_HUMAN": 4})
# QUEST order from z64item.h with explicit 0x18 for LULLABY_INTRO
for hx, name in re.findall(r"/\* (0x[0-9A-Fa-f]+) \*/ (QUEST_[A-Z0-9_]+)", z64item):
    real[name] = int(hx, 16)
# keep every ITEM_ used and every ITEM_MASK_* (MoonMaskCount iterates the range)
used |= {n for n in real if n.startswith(("ITEM_", "QUEST_", "OCARINA_SONG_"))}
used |= {"ITEM_NONE", "ITEM_BOTTLE", "SCENE_MAX", "UPG_BOMB_BAG", "UPG_QUIVER", "UPG_WALLET"}
real.update(parse_enums(strip_comments(scene_h).replace("typedef enum OwlWarpId", "typedef enum"))["OwlWarpId"])
for i, n in enumerate(sorted(x for x in used if x.startswith("RESPAWN_MODE_"))):
    real[n] = i  # used as array index
values, nxt = {}, 1000  # actor ids must fit int16 (enemySoulMap)
for n in sorted(used):
    if n in real:
        values[n] = real[n]
    else:
        values[n] = nxt
        nxt += 1
missing_items = [n for n in used if n.startswith("ITEM_") and n not in real]
assert not missing_items, missing_items

lines = ["// AUTO-GENERATED by gen_shim.py", "#pragma once", "#include <string>", "#include <unordered_map>"]
for n in sorted(values):
    lines.append(f"constexpr int {n} = {values[n]};")
lines.append("inline const std::unordered_map<std::string, int>& ConstByName() {")
lines.append("    static const std::unordered_map<std::string, int> m = {")
lines += [f'        {{"{n}", {n}}},' for n in sorted(values)]
lines += ["    };", "    return m;", "}"]
(out / "consts.h").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"constants: {len(values)} ({sum(1 for n in values if n in real)} with real values)")

# ---- name tables for Types.h enums (scenario I/O) ----------------------------------------------
enums = parse_enums(strip_comments((rando / "Types.h").read_text(encoding="utf-8")))
nl = ["// AUTO-GENERATED by gen_shim.py", "#pragma once", "#include <string>", "#include <unordered_map>"]
for table, ename in (("RC", "RandoCheckId"), ("RR", "RandoRegionId"), ("RE", "RandoEvent"), ("RO", "RandoOptionId"),
                     ("INF", "RandoInf"), ("RI", "RandoItemId")):
    nl.append(f"inline const std::unordered_map<std::string, int>& {table}ByName() {{")
    nl.append("    static const std::unordered_map<std::string, int> m = {")
    nl += [f'        {{"{n}", {v}}},' for n, v in enums[ename].items()]
    nl += ["    };", "    return m;", "}"]
(out / "names.h").write_text(chr(10).join(nl) + chr(10), encoding="utf-8")
print("name tables written")
