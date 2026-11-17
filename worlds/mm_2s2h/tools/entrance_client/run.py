"""Compile and exercise the real client transition hook and door-key decision, without starting the game.

Usage from archipelago: python worlds/mm_2s2h/tools/entrance_client/run.py ../2s2h
Requires MSVC on Windows, or c++ on other systems. Only game state/API boundaries are mocked.
"""
from pathlib import Path
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
enums = {}
for path in (root / "mm/include/z64scene.h", root / "mm/2s2h/Rando/Types.h"):
    enums.update(parse_enums(strip_comments(path.read_text(encoding="utf-8"))))
lines = []
for name in ("RandoOptionId", "EntranceSceneId", "RandoItemId", "RandoInf"):
    lines.append("enum " + name + " {" + ",".join(f"{k}={v}" for k, v in enums[name].items()) + "};")
lines.append("constexpr int RO_GENERIC_OFF = 0;")
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
for name in ("Rando/Rando.h", "Rando/Logic/Logic.h", "2s2h/GameInteractor/GameInteractor.h", "variables.h", "functions.h"):
    header = build / name
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text('#include "mock.h"\n', encoding="utf-8")
exe = build / ("entrance_client.exe" if sys.platform == "win32" else "entrance_client")
if sys.platform == "win32":
    vcvars = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")
    batch = build / "compile.bat"
    batch.write_text(f'@echo off\ncall "{vcvars}" >nul\n'
                     f'cl /nologo /std:c++20 /EHsc /O1 /I"{build}" /I"{here}" '
                     f'/I"{root / "mm/2s2h"}" "{here / "main.cpp"}" /Fe:"{exe}" /Fo:"{build / "main.obj"}"\n', encoding="utf-8")
    subprocess.run(["cmd", "/c", str(batch)], check=True)
else:
    subprocess.run([shutil.which("c++") or "c++", "-std=c++20", "-O1", f"-I{build}", f"-I{here}",
                    f"-I{root / 'mm/2s2h'}", str(here / "main.cpp"), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
