# Majora's Mask × Archipelago

**Play 2 Ship 2 Harkinian in an Archipelago multiworld.** Find your friends' items across Termina while they
send yours back — with 2S2H's check pools and options, plus a full entrance randomizer, door keys, souls for every NPC
and shuffled abilities.

[![Latest release](https://img.shields.io/badge/release-0.13.0-2ea44f)](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.13.0)
[![Status](https://img.shields.io/badge/new_features-experimental-orange)](#how-stable-is-it)
[![Game port](https://img.shields.io/badge/2S2H-5.0.1-blue)](https://github.com/HarbourMasters/2ship2harkinian)
[![Archipelago](https://img.shields.io/badge/Archipelago-0.6.6%2B-6c5ce7)](https://archipelago.gg)

**[Download 0.13.0](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.13.0)** ·
[Get started](PLAYING.md) · [Changelog](CHANGELOG.md) ·
[Report a problem](https://github.com/junkx420/Updated-2S2H-Archipelago/issues)

> **Unofficial fan project.** Not made by, affiliated with, or endorsed by Harbour Masters, the 2 Ship 2 Harkinian
> team, xirba13, MMRecompRando or Archipelago. Please report problems here, not to the original projects.

## How stable is it?

0.13.0 is a **regular release**. Automated checks passed and Shuffle Rocks is **off by default**. The new
mechanics and the Clock Shuffle fix are **not tested in game**.

Each release includes **Windows, Linux and macOS clients**, `mm_2s2h.apworld`, an example YAML and a manifest with
exact commits and SHA-256 checksums. Always use client and apworld from the **same release**.

## What's included

### Multiworld basics

| Feature | What you get |
|---|---|
| **Items & hints** | Send and receive items with names and owners. Exact-location in-game hints become Archipelago hints. Other worlds' items look gray (filler), colored (useful) or colored with an arrow (progression/trap). |
| **Check pools** | Songs, shops, owl statues, cows, fairies, tokens, enemy drops, grass, pots, crates, barrels, trees and more. |
| **Open check markers** | Snowballs, trees, beehives, the Swordsman's School gong and owl statues are tinted yellow or by item class with CSMC. The music wall and Song of Soaring engraving sparkle. Markers disappear when the checks are collected. |
| **Items & time** | Boss/enemy souls, ocarina buttons, swim, extra songs, clocks, Triforce Hunt, traps and plentiful items. |
| **Dungeon items & logic** | Keys and fairies anywhere, in their own dungeon or at start; Glitchless, No Logic or Nearly No Logic. |
| **Tracking** | Universal Tracker, item/location groups, starting inventory, DeathLink. |

### New mechanics · experimental, off by default

| Feature | Option | What it does |
|---|---|---|
| **Shuffle Rocks** | `shuffle_rocks` | Adds 122 checks to small rocks, blast boulders, bronze boulders, rock rings and rolling boulders. Off by default. |
| **NPC souls** | `shuffle_npc_souls` | `all`: 65 souls, every NPC from carpenters and Gorons to shopkeepers. Without its soul an NPC is not in the world at all. `classic`: the 30 souls of 0.11. A found soul shows the NPC's model. |
| **Temple entrances** | `shuffle_dungeon_entrances` | The four temple entrances lead to other temples, coupled. |
| **Boss rooms** | `shuffle_boss_entrances` | Odolwa, Goht, Gyorg and Twinmold sit behind other boss doors. Blue warps return outside the temple you entered. |
| **Door keys** | `shuffle_door_keys` | 26 buildings stay locked until you find their key. Ways out are never locked. |
| **Grottos** | `shuffle_grottos` | Grotto holes lead into other grottos; you come back out where you fell in. |
| **Interiors** | `shuffle_interiors` | Building doors lead into other buildings with the same opening hours. |
| **Overworld** | `shuffle_overworld` | Exits between outdoor areas are shuffled, coupled. Clock Town's gates always lead out. |
| **Special areas** | `shuffle_special_areas` | Spider Houses, Beneath the Well, Ikana Castle, Secret Shrine and Deku Shrine swap entrances. |
| **Mixed pools** | `mix_entrance_pools` | Mix temples, interiors, overworld and special areas into one pool — or copy the "shuffle everything" block from the example YAML. |
| **Epona riding** | `shuffle_epona_riding` | Riding Epona and Gorman's race need a new item. |
| **Bean planting** | `shuffle_bean_planting` | Planting magic beans needs a new item. |
| **Entrance tracker** | in game | A window next to the check tracker remembers where every shuffled entrance led — spoiler-free until you walk through. |

Boss rooms and grottos always shuffle within their own pool; the Clock Tower, the Moon, one-way paths and mandatory
cutscenes stay vanilla. [Every fixed exit and why](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

## Start a multiworld

1. Install [Archipelago 0.6.6+](https://archipelago.gg), download the release and install `mm_2s2h.apworld`.
2. Set your name and options in the example YAML; generate and host through Archipelago.
3. Extract the client and provide your own **Majora's Mask (US)** ROM on first launch.
4. **ESC → Dev Tools → Archipelago → Connect**, then **create a new save while connected**. Leave native
   **Enable Rando** off.
5. Optional: open **Entrance Tracker** from the randomizer menu when you play with shuffled entrances.

[Full setup instructions](PLAYING.md)

The 0.13 client accepts seeds from apworld 0.1 to 0.13 and loads older saves. Generate new seeds with apworld 0.13.0.
MMRecompRando (`mm_recomp`) seeds use a different world and are not compatible.

With Clock Shuffle, a new AP save waits for its initial server items and starts in the earliest owned half-day.
Press **B** to return to file select while it waits.

## What has been checked?

| Check | Result |
|---|---|
| Automated tests | **861 passed**, 0 failed |
| Original-logic parity (all new options off) | **1200 / 1200** scenarios identical to 2S2H |
| Random seeds | **100 / 100** with Shuffle Rocks, **100 / 100** with Shuffle Rocks plus all fork options |
| Native client test bench | 122 rock drop lifecycles and 12 Clock Shuffle startup cases |
| In-game play | **not yet** for the new mechanics; Linux/macOS untested in game |

The in-game *check* tracker does not evaluate shuffled entrances or extra area rules; use Universal Tracker or the
spoiler log for logic. [Detailed results and play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

## Source & development

| Branch | Contents |
|---|---|
| [`client`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/client) | 2S2H 5.0.1 with the AP client and new mechanics; platform builds run in GitHub Actions. |
| [`apworld`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/apworld/worlds/mm_2s2h) | Archipelago world 0.13.0, generated logic, options and validation tools. |
| [`main`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/main) | Project overview, setup, changelog and release packaging workflow. |

The world's logic is generated from the fork's C++ sources: enums, checks, items, regions and the feature tables
for souls, keys, abilities and entrances. AP IDs derive from enum values, options travel as `RO_*` names, and
saved pairings rebuild the same entrances in Universal Tracker. The original 2S2H region logic is never edited;
parity checks guard the default behavior.

## Credits

- [Harbour Masters / 2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian) — game port and randomizer
- [xirba13 / 2S2H Archipelago](https://github.com/xirba13/2ship2harkinian-archipelago) — original AP client layer
- [Archipelago](https://github.com/ArchipelagoMW/Archipelago) — multiworld framework
- [black-sliver / apclientpp](https://github.com/black-sliver/apclientpp) — client library

Original licenses and credits remain in their source branches. No ROMs or extracted ROM data are distributed;
you need your own legally obtained game copy.
