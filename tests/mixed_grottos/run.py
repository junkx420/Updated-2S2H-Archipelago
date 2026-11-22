"""Run the existing native suite with the client-only mixed grotto cases.

Pass the Archipelago checkout as the first argument. Its world and harness stay
untouched while the two implementations are being landed separately.
"""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def replace(path, old, new):
    source = path.read_text(encoding="utf-8")
    if source.count(old) != 1:
        raise ValueError(f"{path.name}: expected one occurrence of {old}")
    path.write_text(source.replace(old, new), encoding="utf-8")


def main():
    client = Path(__file__).resolve().parents[2]
    ap = Path(sys.argv[1]).resolve()
    world = Path("worlds/mm_2s2h")
    suite = world / "tools/entrance_client"
    with tempfile.TemporaryDirectory(prefix="mixed-grottos-", dir=client / "build") as tmp:
        staging = Path(tmp)
        harness = staging / suite
        shutil.copytree(ap / suite, harness, ignore=shutil.ignore_patterns("build", "__pycache__"))
        shutil.copy2(ap / world / "tools/gen_from_2s2h.py", harness.parent)
        shutil.copy2(ap / world / "archipelago.json", staging / world)
        shutil.copytree(ap / world / "test/data", staging / world / "test/data")
        shutil.copy2(Path(__file__).with_name("mixed_grottos.h"), harness)
        replace(harness / "mock.h", "s8 data = 0; int position = 0;",
                "s8 data = 0; int position = 0; struct { float x, y, z; } pos{}; "
                "s16 yaw = 0, playerParams = 0; u8 roomIndex = 0; "
                "u32 tempSwitchFlags = 0, unk_18 = 0, tempCollectFlags = 0;")
        replace(harness / "directed.h", "ShuffledBossExterior(boss) ==",
                "ShuffledBossExterior(boss)->entrance ==")
        replace(harness / "main.cpp", '#include "directed.h"',
                '#include "directed.h"\n#include "mixed_grottos.h"')
        replace(harness / "main.cpp", "    TestDirectedEntrances();",
                "    TestDirectedEntrances();\n    MixedGrottoTest::Run();")
        env = os.environ | {"PYTHONPATH": os.pathsep.join(filter(None, (str(ap), os.environ.get("PYTHONPATH"))))}
        subprocess.run([sys.executable, "-B", str(harness / "run.py"), str(client)], cwd=ap, env=env, check=True)


if __name__ == "__main__":
    main()
