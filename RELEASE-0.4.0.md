> **Experimental pre-release — the new features have not been tested in game.**
> Unofficial fan update, unaffiliated with the original projects. Report problems in this repository.

This release bundles **2 Ship 2 Harkinian 5.0.1 with the updated AP client** and **apworld 0.4.0**.
It contains all changes since the regular **0.1.5** release (whose bundled apworld is 0.1.4).

### New mechanics

- **30 NPC souls** restore townsfolk rewards, sales and minigames as you find their souls.
- **Dungeon entrance shuffle** couples the four normal temple doors to randomly assigned temples.
- **Boss entrance shuffle** independently mixes Odolwa, Goht, Gyorg and Twinmold among their boss doors.
- Both shuffles compose their blue-warp returns, keep the boss's rewards/clear effect and preserve saved pairings
  and Universal Tracker reconstruction. Redirected warps refresh death/void return points at the exterior destination.
- Foreign temples/boss rooms add conservative item requirements in logic. The in-game check tracker does not
  evaluate these supplemental requirements.

All three mechanics are **off by default**. To try them, set `shuffle_npc_souls`, `shuffle_dungeon_entrances`
and/or `shuffle_boss_entrances` in the supplied YAML. The inverted Stone Tower exterior, prison door, Majora and
Moon stay fixed. Grottos, houses, overworld, spider houses and Pirate's Fortress are not shuffled.

### Other improvements since 0.1.5

- Other players' items use their AP classification for model color, progression arrow and get-item animation.
- Bottled-fairy revivals no longer trigger DeathLink.
- The new client accepts apworld 0.1–0.4 seeds and loads old save fields tolerantly. Older clients cannot play 0.4 seeds.

### Downloads and setup

| File | Contents |
|---|---|
| `2S2H-Archipelago-0.4.0-Windows.zip` | New Windows client; run `2ship.exe`. |
| `2S2H-Archipelago-0.4.0-Linux.zip` | New Linux AppImage; make executable before launching. |
| `2S2H-Archipelago-0.4.0-macOS.zip` | New macOS DMG; unsigned, subject to macOS confirmation. |
| `mm_2s2h.apworld` | World **0.4.0**; requires Archipelago **0.6.6+**. |
| `mm_2s2h_example.yaml` | All options and descriptions. |
| `BUILD-MANIFEST.json` | Exact source commits, build run, file sizes and SHA-256 checksums. |

Use the client and apworld attached **here** together. Install the apworld, edit the YAML, generate and host.
Start the client, supply your own Majora's Mask (US) ROM, then **ESC → Dev Tools → Archipelago → Connect**.
**Create a new save while connected**, with native **Enable Rando** off.
[Setup guide](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/main/PLAYING.md).

### Validation and remaining play tests

Recorded checks on this source: **531 tests**, **1200/1200 parity scenarios** with entrance options off,
**100/100 boss-only seeds**, **100/100 combined-shuffle seeds**. The mocked native client harness passed
**2304 option/permutation cases**, **23040 warps**, and **44352 respawn checks**. All platform packages are taken
from one GitHub Actions run at the exact client commit; publication waits for all three platforms to succeed.

These checks do not execute the game. The new NPC souls and entrance mechanics, cutscenes, full playthroughs and
Linux/macOS runtime behavior are **not tested in game**. When a shuffled room needs a different warp destination,
its completion cutscene is skipped while its clear effect is retained. Please test first/repeat warps, death/void,
Song of Time and saving/loading; back up existing saves before experimenting.

[Detailed validation and play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)
 · [Full changelog](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/main/CHANGELOG.md)

The regular download remains [0.1.5](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.1.5).
No ROM or extracted ROM data is included.
