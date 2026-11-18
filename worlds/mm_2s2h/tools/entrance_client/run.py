"""Compile and exercise the real client transition hook and door-key decision, without starting the game.

Usage from archipelago: python worlds/mm_2s2h/tools/entrance_client/run.py ../2s2h
Requires MSVC on Windows, or c++ on other systems. Only game state/API boundaries are mocked.
"""
from pathlib import Path
from itertools import combinations
import json
import shutil
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from gen_from_2s2h import parse_enums, strip_comments

here = Path(__file__).resolve().parent
root = Path(sys.argv[1]).resolve()
build = here / "build"
build.mkdir(exist_ok=True)
legacy09_commit = "87077c1bd19b46a6e82cfdea489867a3fad99ae3"
legacy10_commit = "aa6d5b494"


def historical_source(revision: str, path: str) -> str:
    return subprocess.check_output(["git", "-C", str(root), "show", f"{revision}:{path}"], encoding="utf-8")


enums = {}
for path in (root / "mm/include/z64scene.h", root / "mm/2s2h/Rando/Types.h"):
    enums.update(parse_enums(strip_comments(path.read_text(encoding="utf-8"))))
lines = []
for name in ("RandoOptionId", "EntranceSceneId", "RandoItemId", "RandoInf", "RandoEvent", "RandoRegionId"):
    lines.append("enum " + name + " {" + ",".join(f"{k}={v}" for k, v in enums[name].items()) + "};")
lines.append("constexpr int RO_GENERIC_OFF = 0;")
lines.append(f"constexpr int RC_MAX = {enums['RandoCheckId']['RC_MAX']};")
scene_table = (root / "mm/include/tables/scene_table.h").read_text(encoding="utf-8")
scenes = re.findall(r"DEFINE_SCENE\(\w+,\s*(SCENE_\w+)|DEFINE_SCENE_UNSET\((SCENE_\w+)", scene_table)
lines.append("enum SceneId {" + ",".join(a or b for a, b in scenes) + ",SCENE_MAX};")
(build / "constants.h").write_text("\n".join(lines), encoding="utf-8")

# Ways out of the door-key buildings, from the generated logic graph: every exit of a building region (and of the
# regions connected to it in the same scene) into another scene. The client must never lock any of them.
sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from worlds.mm_2s2h.enums_generated import DOOR_KEYS  # noqa: E402
from worlds.mm_2s2h.regions_generated import REGIONS  # noqa: E402


def entrance_value(key: str) -> int:  # z64scene.h ENTRANCE(scene, spawn)
    scene, spawn = key.split(":")
    return ((enums["EntranceSceneId"][f"ENTR_SCENE_{scene}"] & 0x7F) << 9) | ((int(spawn) & 0x1F) << 4)


outward, into = set(), []
for row in DOOR_KEYS:
    into.append((row["key"], [entrance_value(k) for k in row["into"]]))
    todo, seen = list(row["inside_regions"]), set()
    while todo:
        rr = todo.pop()
        if rr in seen:
            continue
        seen.add(rr)
        todo += [t for t, _ in REGIONS[rr][3] if REGIONS[t][1] == REGIONS[rr][1]]
        outward |= {entrance_value(k) for k, t, _ in REGIONS[rr][4] if REGIONS[t][1] != REGIONS[rr][1]}
assert not outward & {v for _, vs in into for v in vs}, "a way out is also a way in"
(build / "door_keys_graph.h").write_text(
    "#pragma once\n#include <utility>\n#include <vector>\n"
    f"inline const std::vector<unsigned> doorKeyOutward = {{ {', '.join(map(str, sorted(outward)))} }};\n"
    "inline const std::vector<std::pair<int, std::vector<unsigned>>> doorKeyInto = {"
    + ", ".join(f"{{ {ri}, {{ {', '.join(map(str, vs))} }} }}" for ri, vs in into) + "};\n", encoding="utf-8")
# Grottos as the apworld was generated: row index, logic "into", game entrance, grotto data (-1 = none), scene exit?
from worlds.mm_2s2h.enums_generated import ENTRANCE_SHUFFLE  # noqa: E402
grotto_rows = [(i, row["into_value"], entrance_value(row["grotto"]["game_entrance"]),
                -1 if row["grotto"]["data"] is None else row["grotto"]["data"], int(row["grotto"]["kind"] == "SCENE_EXIT"))
               for i, row in enumerate(ENTRANCE_SHUFFLE) if row["grotto"]]
(build / "grotto_graph.h").write_text(
    "#pragma once\n#include <array>\n#include <vector>\n"
    f"inline const size_t generatedRows = {len(ENTRANCE_SHUFFLE)};\n"
    "inline const std::vector<std::array<int, 5>> generatedGrottos = {"
    + ", ".join("{ " + ", ".join(map(str, g)) + " }" for g in grotto_rows) + "};\n", encoding="utf-8")
# Interiors as the apworld was generated: row index, "into", hours class (0 always open, 1 daytime, 2 own hours); and
# the entrances songs and owls lead to (RR_MAX's exits: Song of Time and the owl statues), never redirected.
hours_id = {"ALWAYS_OPEN": 0, "DAYTIME": 1, "OWN_HOURS": 2}
interior_rows = [(i, row["into_value"], hours_id[row["hours"]]) for i, row in enumerate(ENTRANCE_SHUFFLE) if row["hours"]]
warps = sorted({entrance_value(k) for k, _, _ in REGIONS["RR_MAX"][4]} | {entrance_value("SOUTH_CLOCK_TOWN:0")})
(build / "interior_graph.h").write_text(
    "#pragma once\n#include <array>\n#include <vector>\n"
    "inline const std::vector<std::array<int, 3>> generatedInteriors = {"
    + ", ".join("{ " + ", ".join(map(str, r)) + " }" for r in interior_rows) + "};\n"
    f"inline const std::vector<unsigned> songAndOwlEntrances = {{ {', '.join(map(str, warps))} }};\n", encoding="utf-8")
# The real randomizer save structs and JSON codec, verbatim.
def save_structs(source: str) -> str:
    return "\n".join(re.search(rf"typedef struct {name} \{{.*?\n\}} {name};", source, re.S).group(0)
                     for name in ("RandoSaveCheck", "RandoSaveInfo"))


def save_codec(source: str) -> str:
    return re.search(r"inline void to_json\(json& j, const RandoSaveCheck&.*?inline void from_json"
                     r"\(const json& j, RandoSaveInfo& rando\) \{.*?\n}\n", source, re.S).group(0)


(build / "rando_save.h").write_text("#pragma once\n" + save_structs(
    (root / "mm/include/z64save.h").read_text(encoding="utf-8")), encoding="utf-8")
(build / "grown_array.h").write_text("#pragma once\n#include <nlohmann/json.hpp>\nusing json = nlohmann::json;\n"
                                     + save_codec((root / "mm/2s2h/BenJsonConversions.hpp").read_text(encoding="utf-8")),
                                     encoding="utf-8")

# Encode a save with the historical types, IDs, row order and codec alongside the current reader.
old_enums = parse_enums(strip_comments(historical_source(legacy09_commit, "mm/2s2h/Rando/Types.h")))
old_scenes = parse_enums(strip_comments(historical_source(legacy09_commit, "mm/include/z64scene.h")))["EntranceSceneId"]
old_rows_source = historical_source(legacy09_commit, "mm/2s2h/Rando/MiscBehavior/EntranceShuffle.cpp")
old_table = re.search(r"shuffleEntrances\s*=\s*\{(.*?)\n\};", strip_comments(old_rows_source), re.S).group(1)
old_rows = [(old_scenes[f"ENTR_SCENE_{scene}"] << 9) | (int(spawn) << 4)
            for scene, spawn in re.findall(r"\{\s*RO_\w+,\s*ENTRANCE\(\s*(\w+),\s*(\d+)\s*\)", old_table)]
assert len(old_rows) == len(set(old_rows)) == 157
old_options = old_enums["RandoOptionId"]
old_limits = {name: old_enums[enum][name] for enum, name in (
    ("RandoInf", "RANDO_INF_MAX"), ("RandoEvent", "RE_MAX"), ("RandoCheckId", "RC_MAX"), ("RandoOptionId", "RO_MAX"))}
(build / "legacy09_codec.h").write_text(
    "#pragma once\n#include <unordered_map>\nnamespace legacy09 {\n"
    f'inline constexpr char revision[] = "{legacy09_commit}";\n'
    + "".join(f"constexpr int {key} = {value};\n" for key, value in old_limits.items())
    + "enum RandoItemId {" + ", ".join(f"{key} = {value}" for key, value in old_enums["RandoItemId"].items()) + "};\n"
    + "inline const std::unordered_map<std::string, size_t> optionIds = {"
    + ", ".join(f'{{"{key}", {value}}}' for key, value in old_options.items() if key != "RO_MAX") + "};\n"
    + "inline constexpr u16 entrances[] = {" + ", ".join(map(str, old_rows)) + "};\n"
    + save_structs(historical_source(legacy09_commit, "mm/include/z64save.h")) + "\n"
    + save_codec(historical_source(legacy09_commit, "mm/2s2h/BenJsonConversions.hpp")) + "}\n", encoding="utf-8")

old10 = parse_enums(strip_comments(historical_source(legacy10_commit, "mm/2s2h/Rando/Types.h")))
for enum, sentinel in (("RandoItemId", "RI_MAX"), ("RandoOptionId", "RO_MAX"), ("RandoInf", "RANDO_INF_MAX")):
    assert all(enums[enum][key] == value for key, value in old10[enum].items() if key != sentinel), enum
(build / "legacy10_codec.h").write_text(
    '#pragma once\nnamespace legacy10 {\n'
    + f'inline constexpr int RO_MAX = {old10["RandoOptionId"]["RO_MAX"]};\n'
    + f'inline constexpr int RANDO_INF_MAX = {old10["RandoInf"]["RANDO_INF_MAX"]};\n'
    + save_structs(historical_source(legacy10_commit, "mm/include/z64save.h")) + "\n"
    + save_codec(historical_source(legacy10_commit, "mm/2s2h/BenJsonConversions.hpp")) + "}\n", encoding="utf-8")
# Game functions the overworld redirect calls, verbatim (z_scene.c, z_play.c).
scene_c = (root / "mm/src/code/z_scene.c").read_text(encoding="utf-8").replace("\r\n", "\n")
play_c = (root / "mm/src/code/z_play.c").read_text(encoding="utf-8").replace("\r\n", "\n")
functions = [re.search(r"u16 Entrance_Create\(s32 scene, s32 spawn, s32 layer\) \{.*?\n}\n", scene_c, re.S).group(0),
             re.search(r"s16 Play_GetOriginalSceneId\(s16 sceneId\) \{.*?\n}\n", play_c, re.S).group(0)]
(build / "game_functions.h").write_text("#pragma once\n" + "".join("inline " + f for f in functions), encoding="utf-8")

# Compile the actual action entry points against the mock state. Rename the C parameter `this` for C++.
def game_functions(path, names):
    source = (root / "mm/src" / path).read_text(encoding="utf-8")
    result = []
    for name in names:
        body = re.search(rf"^(?:void|s32) {name}\([^;]*?\) \{{.*?\n\}}\n", source, re.M | re.S).group(0)
        result.append(re.sub(r"\bthis\b", "self", body))
    return "\n".join(result)


gorman = (root / "mm/src/overlays/actors/ovl_En_In/z_en_in.c").read_text(encoding="utf-8")
race_starts = re.findall(r"case 0x3475:\n(.*?)\n\s*break;", gorman, re.S)
assert len(race_starts) == 2 and race_starts[0] == race_starts[1]
skip_race = (root / "mm/2s2h/Enhancements/Minigames/SkipHorseRace.cpp").read_text(encoding="utf-8")
skip_body = re.search(r"COND_VB_SHOULD\(VB_PLAY_TRANSITION_CS, CVAR, \{(.*?)\n    \}\);", skip_race, re.S).group(1)
(build / "game_ability_functions.h").write_text(
    game_functions("code/z_actor.c", ("Player_MountHorse",))
    + game_functions("code/z_horse.c", ("Horse_SpawnOverworld", "Horse_SpawnMinigame", "Horse_Spawn"))
    + game_functions("overlays/actors/ovl_En_In/z_en_in.c", ("func_808F4150", "func_808F4270"))
    + game_functions("overlays/actors/ovl_Obj_Bean/z_obj_bean.c", ("func_80937FC8", "func_80938834"))
    + "\nvoid StartGormanRace(PlayState* play) {\n" + race_starts[0] + "\n}\n"
    + "\nvoid SkipGormanRace() {" + skip_body + "\n}\n", encoding="utf-8")
# Overworld rows as the apworld was generated: row index, "into", index of the opposite row.
over = [i for i, row in enumerate(ENTRANCE_SHUFFLE) if row["option"] == "RO_SHUFFLE_OVERWORLD"]
over_index = {ENTRANCE_SHUFFLE[i]["into"]: i for i in over}
(build / "overworld_graph.h").write_text(
    "#pragma once\n#include <array>\n#include <vector>\n"
    "inline const std::vector<std::array<int, 3>> generatedOverworld = {"
    + ", ".join(f"{{ {i}, {ENTRANCE_SHUFFLE[i]['into_value']}, {over_index[ENTRANCE_SHUFFLE[i]['out']]} }}" for i in over)
    + "};\n", encoding="utf-8")
# Special areas as generated: row index, "into". Real 0.8 slot data (test/data): its options and pairs.
special = [(i, row["into_value"]) for i, row in enumerate(ENTRANCE_SHUFFLE) if row["option"] == "RO_SHUFFLE_SPECIAL_AREAS"]
legacy = []
for name in ("all_own", "overworld"):
    data = json.loads((here.parents[1] / "test" / "data" / f"slot_data_0_8_0_{name}.json").read_text(encoding="utf-8"))
    assert data["apworld_version"].startswith("0.8"), data["apworld_version"]
    on = [k for k, v in data["options"].items() if k.startswith("RO_SHUFFLE_") and v and k in enums["RandoOptionId"]]
    legacy.append("{ { " + ", ".join(on) + " }, { " + ", ".join(f"{{ {a}, {b} }}" for a, b in data["entrance_pairs"])
                  + " } }")
(build / "special_graph.h").write_text(
    "#pragma once\n#include <array>\n#include <utility>\n#include <vector>\n"
    "inline const std::vector<std::array<int, 2>> generatedSpecial = {"
    + ", ".join(f"{{ {i}, {v} }}" for i, v in special) + "};\n"
    "struct LegacySlot { std::vector<int> options; std::vector<std::pair<int32_t, int32_t>> pairs; };\n"
    "inline const size_t rows08 = 134; // shuffleEntrances rows of apworld 0.8\n"
    "inline const std::vector<LegacySlot> legacy08 = {" + ", ".join(legacy) + "};\n", encoding="utf-8")

from worlds.mm_2s2h.enums_generated import ENTRANCE_MIX_POOLS  # noqa: E402
from worlds.mm_2s2h.test.test_pool_trim import build as build_world  # noqa: E402
from worlds.mm_2s2h.test.test_special_areas import ALL_OWN, DATA  # noqa: E402

legacy09 = (DATA / "slot_data_0_9_0_all_pools.json").read_text(encoding="utf-8")
legacy10 = (DATA / "slot_data_0_10_0_mixed.json").read_text(encoding="utf-8")
assert all(key in old10["RandoOptionId"] for key in json.loads(legacy10)["options"])
mixed_slots = [build_world(ALL_OWN | {"mix_entrance_pools": pools}, 610, fill=False).worlds[1].fill_slot_data()
               for count in range(2, 5) for pools in combinations(ENTRANCE_MIX_POOLS, count)]
(build / "mixed_slots.h").write_text(
    '#pragma once\ninline const char* legacy09Slot = R"slot(' + legacy09 + ')slot";\n'
    'inline const char* legacy10Slot = R"slot(' + legacy10 + ')slot";\n'
    'inline const char* mixedSlots[] = {\n' +
    "\n".join('R"slot(' + json.dumps(slot) + ')slot",' for slot in mixed_slots) + '\n};\n', encoding="utf-8")
json_include = root / "build/x64/vcpkg/installed/x64-windows-static/include"
for name in ("Rando/Rando.h", "Rando/Logic/Logic.h", "2s2h/Rando/Rando.h", "2s2h/Rando/Types.h",
             "2s2h/GameInteractor/GameInteractor.h", "variables.h", "functions.h", "z64horse.h"):
    header = build / name
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text('#include "mock.h"\n', encoding="utf-8")
exe = build / ("entrance_client.exe" if sys.platform == "win32" else "entrance_client")
if sys.platform == "win32":
    vcvars = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")
    batch = build / "compile.bat"
    batch.write_text(f'@echo off\ncall "{vcvars}" >nul\n'
                     f'cl /nologo /std:c++20 /EHsc /O1 /I"{build}" /I"{here}" /I"{json_include}" '
                     f'/I"{root / "mm/2s2h"}" "{here / "main.cpp"}" /Fe:"{exe}" /Fo:"{build / "main.obj"}"\n', encoding="utf-8")
    subprocess.run(["cmd", "/c", str(batch)], check=True)
else:
    subprocess.run([shutil.which("c++") or "c++", "-std=c++20", "-O1", f"-I{build}", f"-I{here}", f"-I{json_include}",
                    f"-I{root / 'mm/2s2h'}", str(here / "main.cpp"), "-o", str(exe)], check=True)
subprocess.run([str(exe), str(build / "rando_save_0_9_0.json")], check=True)
