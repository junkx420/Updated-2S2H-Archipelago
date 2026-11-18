## Majora's Mask × Archipelago 0.11.1

The biggest update so far: a **full entrance randomizer**, **door keys**, **shuffled abilities** and an
**in-game entrance tracker**, on top of the NPC souls from the 0.4.0 preview. Client and **apworld 0.11.1** are
bundled together.

> **Fixed apworld (re-uploaded 2026-11-18):** the first `mm_2s2h.apworld` of this release lacked two manifest
> fields, so Archipelago reported it as version 0.0.0 and the client refused the seed ("needs 0.11.x").
> If you downloaded it earlier: download `mm_2s2h.apworld` again, reinstall it, and **generate your seed again**.
> The client packages are unchanged.

> **Why a regular release, and why still experimental?**
> This is published as a regular release because every automated check passes: 805 tests, 1200 of 1200
> scenarios matching the original 2S2H logic, 100 of 100 random seeds per new option and with all options combined,
> and a native client test bench covering every entrance pairing. Every new option is **off by default**, so a seed
> without them plays like the established feature set.
> The new mechanics have **not been played in game yet**. Treat them as experimental: use a separate save, keep
> backups, and please report anything odd.

### New since 0.4.0

| Feature | Option | What it does |
|---|---|---|
| **Door keys** | `shuffle_door_keys` | 26 buildings in Termina stay locked until you find their key. One key opens every way in; ways out are never locked. The door tells you which key is missing. |
| **Grotto shuffle** | `shuffle_grottos` | The 26 grotto holes lead into other grottos. You always come back out at the hole you fell into, and each grotto keeps its own checks. |
| **Interior shuffle** | `shuffle_interiors` | Doors into buildings lead into other buildings. Only buildings with the same opening hours mix, so night-only or day-only logic stays correct. |
| **Overworld shuffle** | `shuffle_overworld` | Exits between outdoor areas lead elsewhere, coupled. The Clock Town gates always lead out of town; underwater ways mix only with each other; riding through a shuffled exit, Link gets off Epona. |
| **Special areas** | `shuffle_special_areas` | Spider Houses, Beneath the Well, Ikana Castle, the Secret Shrine and the Deku Shrine swap entrances. |
| **Mixed pools** | `mix_entrance_pools` | Mix temples, interiors, overworld and special areas into one pool. With temples selected, the inverted Stone Tower Temple joins too. A ready-made "shuffle everything" block is in the example YAML. |
| **Epona riding** | `shuffle_epona_riding` | Riding Epona and Gorman's race need a new item. Romani's practice ride stays free. |
| **Bean planting** | `shuffle_bean_planting` | Planting magic beans needs a new item, including for the Day 2 rain. |
| **Entrance tracker** | always available | New in-game window next to the check tracker. It lists every shuffled entrance, spoiler-free: destinations show as `???` until you walk through. Search, group by area or pool, highlight the current scene, or show everything. |

Also improved:

- **Fewer failed generations:** an overfull item pool drops traps and extra Plentiful copies before giving up.
  Every original item stays.
- **Consistent names:** the tracker uses the same entrance names as the Archipelago spoiler log.

### Still stays fixed

Boss rooms and grottos shuffle within their own pools even when mixing; the Clock Tower, the Moon, one-way paths,
mandatory cutscenes and a few special transitions stay vanilla. The full list of fixed exits, each with its reason,
is in the [testing notes](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md).

### Compatibility

- The 0.11 client accepts seeds from apworld **0.1 to 0.11**. Older saves load; new save fields start empty.
- Generate new seeds with the bundled **apworld 0.11.1** and play them with this client. Older clients cannot play
  0.11 seeds.
- The in-game *check* tracker does not know shuffled entrances or extra area rules. Use Universal Tracker or the
  spoiler log for logic; use the new *entrance* tracker to remember where doors lead.

### What is checked, what is not

- **Checked automatically:** 805 tests, 1200/1200 parity scenarios with all new options off, 100/100 random seeds
  for every new option and for all options together, and a native test bench that runs the real redirect code
  for every entrance pairing, including warps, respawns and loading old saves.
- **Not checked:** none of the new mechanics have been played in game. Linux and macOS packages are untested in game.
- **Play-test checklist:** [testing.md](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

### Download

Get the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from this release.
`BUILD-MANIFEST.json` lists the exact commits and SHA-256 checksums. Setup steps are in
`ARCHIPELAGO-INSTRUCTIONS.md` inside each client package.

Unofficial fan project. No ROM or extracted ROM data is included.
