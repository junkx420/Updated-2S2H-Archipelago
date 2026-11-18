"""Package exact GitHub Actions artifacts and the pinned apworld checkout for one release version."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
DIST.mkdir(exist_ok=False)
client_sha, world_sha = os.environ["CLIENT_SHA"], os.environ["APWORLD_SHA"]
version = os.environ["RELEASE_VERSION"]
world = ROOT / "world-source/worlds/mm_2s2h"
assert json.loads((world / "archipelago.json").read_text())["world_version"] == version

# Archipelago reads world_version only from a manifest with its container fields (worlds/Files.py,
# APWorldContainer); without them it logs "Invalid or missing manifest" and reports world_version 0.0.0.
manifest_source = json.loads((world / "archipelago.json").read_text())
manifest_source.update({"version": 7, "compatible_version": 7})
with zipfile.ZipFile(DIST / "mm_2s2h.apworld", "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(world.rglob("*")):
        if (path.is_file() and "__pycache__" not in path.parts and path.suffix not in {".pyc", ".pyo"}
                and path.relative_to(world) != Path("archipelago.json")):
            archive.write(path, Path("mm_2s2h") / path.relative_to(world))
    archive.writestr("mm_2s2h/archipelago.json", json.dumps(manifest_source, indent=4) + "\n")
shutil.copyfile(world / "docs/example.yaml", DIST / "mm_2s2h_example.yaml")

for platform, label, binary in [("windows", "Windows", "2ship.exe"),
                                ("linux", "Linux", "2ship.appimage"),
                                ("macos", "macOS", "2Ship.dmg")]:
    source = ROOT / "artifacts" / platform
    matches = list(source.rglob(binary))
    assert len(matches) == 1 and matches[0].stat().st_size > 0, (platform, matches)
    if platform == "windows":
        certs = list(source.rglob("networking/cacert.pem"))
        assert len(certs) == 1 and certs[0].stat().st_size > 0
    # GitHub artifact downloads may not retain executable permissions.
    if platform == "linux":
        matches[0].chmod(0o755)
    with zipfile.ZipFile(DIST / f"2S2H-Archipelago-{version}-{label}.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                archive.write(path, Path("2S2H-Archipelago") / path.relative_to(source))
        archive.write(ROOT / "PLAYING.md", "2S2H-Archipelago/ARCHIPELAGO-INSTRUCTIONS.md")
        archive.writestr("2S2H-Archipelago/BUILD-INFO.json", json.dumps({
            "release": version, "client_commit": client_sha, "apworld_commit": world_sha,
            "build_run": os.environ["BUILD_RUN"], "platform": label,
        }, indent=2) + "\n")

manifest = {"release": version, "client_commit": client_sha, "apworld_commit": world_sha,
            "build_run": os.environ["BUILD_RUN"], "files": {}}
for path in sorted(DIST.iterdir()):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            assert archive.testzip() is None, path
    manifest["files"][path.name] = {"bytes": path.stat().st_size,
                                   "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
(DIST / "BUILD-MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps(manifest, indent=2))
