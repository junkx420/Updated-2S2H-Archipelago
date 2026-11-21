## Majora's Mask × Archipelago 0.13.0

**Rocks can now hold checks, and open checks are easier to spot.** This release also fixes the starting time for
new Archipelago saves with Clock Shuffle. The client and **apworld 0.13.0** are bundled together.

> **Regular release, new mechanics experimental.**
> Automated checks passed. Shuffle Rocks is **off by default**. The new mechanics and the Clock Shuffle fix
> are **not tested in game**.

### New since 0.12.0

| Feature | Option | What it does |
|---|---|---|
| **Shuffle Rocks** | `shuffle_rocks` (off by default) | Adds 122 checks across small rocks, blast boulders, bronze boulders, rock rings and rolling boulders. |
| **Open check markers** | automatic | Open snowball, tree, beehive, Swordsman's School gong and owl statue checks are tinted yellow or by item class with Chest Size Matches Contents (CSMC). The Termina Field music wall and the Song of Soaring engraving sparkle. Objects return to their usual appearance after their checks are collected. |
| **Clock Shuffle startup** | `clock_shuffle` | A new AP save starts in the earliest half-day it owns, including its starting clock. File select waits for the initial items from the server. Press **B** to return to file select. Existing saves in the Final Hours stay there when loaded. |

#### How rock checks behave

- Lifting or breaking an eligible rock drops its check. Pending drops can reappear after you leave and return.
- With CSMC on, the Termina Field tree with two rupees stays yellow until both checks are collected. The music
  wall keeps its yellow sparkle until all of its checks are collected.
- The rock placement exceptions and the reasons for them are listed in the world's testing notes.
  [Shuffle Rocks testing notes](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md#stage-9--shuffle-rocks-apworld-0130)

### Compatibility

- The 0.13 client accepts seeds from apworld **0.1 to 0.13** and loads older saves.
- Generate new seeds with the bundled **apworld 0.13.0**.

### What is checked, what is not

- **Checked automatically:** 861 tests passed, 1200/1200 parity scenarios matched the original 2S2H logic with
  fork options off, and 100/100 random seeds generated with Shuffle Rocks alone and 100/100 with Shuffle Rocks
  plus all fork options. GitHub Actions built the Windows, Linux and macOS packages.
- **In-game testing:** The new mechanics, the clock fix and the Linux and macOS packages are **not tested in game**.
- [Play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

### Coming next

Signs as checks.

### Download

Get the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from this release.
`BUILD-MANIFEST.json` lists the exact commits and SHA-256 checksums. Setup steps are in
`ARCHIPELAGO-INSTRUCTIONS.md` inside each client package.

#### Linux

Extract the Linux ZIP. In the extracted folder, run `chmod +x 2ship.appimage` and then `./2ship.appimage`.
If the AppImage reports missing FUSE, install `libfuse2` (`libfuse2t64` on Ubuntu 24.04) or launch it with
`./2ship.appimage --appimage-extract-and-run`. The Linux package is **not tested in game**.
[AppImage FUSE troubleshooting](https://docs.appimage.org/user-guide/troubleshooting/fuse.html)

Unofficial fan project. No ROM or extracted ROM data is included.
