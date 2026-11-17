# Majora's Mask × Archipelago

**Play 2 Ship 2 Harkinian in an Archipelago multiworld.** Find your friends' items across Termina while they
send yours back — with the check pools and options of 2S2H's randomizer, plus experimental new shuffles.

[![Regular release](https://img.shields.io/badge/regular_release-0.1.5-2ea44f)](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.1.5)
[![Preview](https://img.shields.io/badge/pre--release-0.4.0-orange)](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.4.0)
[![Game port](https://img.shields.io/badge/2S2H-5.0.1-blue)](https://github.com/HarbourMasters/2ship2harkinian)

[Get started](PLAYING.md) · [Changelog](CHANGELOG.md) · [Report a problem](https://github.com/junkx420/Updated-2S2H-Archipelago/issues)

> **Unofficial fan project.** Not made by, affiliated with, or endorsed by Harbour Masters, the 2 Ship 2 Harkinian
> team, xirba13, MMRecompRando or Archipelago. Please report problems here, not to the original projects.

## Choose your download

| | Regular release | Experimental preview |
|---|---|---|
| Release | **[0.1.5](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.1.5)** | **[0.4.0](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.4.0)** |
| Bundled apworld | 0.1.4 | 0.4.0 |
| Features | Established 0.1.x feature set and exact-location AP hints | Adds NPC souls, temple entrance shuffle, boss entrance shuffle and newer fixes |
| Play coverage | Limited; regular release does not mean bug-free | New features **not tested in game** |
| Best for | The established download line | Testing the new mechanics and reporting issues |

Preview downloads are published automatically after all three platform builds and package checks succeed.
If downloads are still being prepared, follow [publication progress](https://github.com/junkx420/Updated-2S2H-Archipelago/actions/workflows/publish-preview.yml).

Each release includes **Windows, Linux and macOS clients**, `mm_2s2h.apworld` and an example YAML.
Download the client and apworld from the **same release**. The preview also includes a manifest with exact commits
and SHA-256 checksums. Older releases do not contain the newer features listed below.

## What's included

| Feature | What you can do |
|---|---|
| **Multiworld items & hints** | Send and receive items; see their names and owners. Exact-location in-game hints become AP hints. |
| **Check pools** | Shuffle songs, shops, owl statues, cows, fairies, tokens, enemy drops, grass, pots, crates, barrels, trees and more. |
| **Item & time options** | Boss/enemy souls, ocarina buttons, swim, extra songs, clocks, Triforce Hunt, traps and plentiful items. |
| **Dungeon items & logic** | Place keys/fairies anywhere, in their own dungeon or at start; use Glitchless, No Logic or Nearly No Logic. |
| **Tracker & configuration** | Universal Tracker, item/location groups, starting inventory, access requirements and DeathLink. |
| **NPC souls · preview** | Find 30 souls to restore NPC rewards, sales and minigames. Optional, off by default. |
| **Temple entrances · preview** | Shuffle the four normal temple entrances with coupled return paths. Optional, off by default. |
| **Boss rooms · preview** | Shuffle Odolwa, Goht, Gyorg and Twinmold independently of temple entrances. Optional, off by default. |

In the preview, blue warps account for both entrance shuffles and return outside the temple whose boss door you
used. Clear effects stay with the boss. The inverted Stone Tower exterior, Woodfall prison door, Majora and the
Moon stay fixed. Grottos, houses, overworld transitions, spider houses and Pirate's Fortress are not shuffled.

## Start a multiworld

1. Install [Archipelago 0.6.6+](https://archipelago.gg), download your chosen release, and install `mm_2s2h.apworld`.
2. Set your name and options in the example YAML; generate and host through Archipelago.
3. Extract the client and provide your own **Majora's Mask (US)** ROM on first launch.
4. **ESC → Dev Tools → Archipelago → Connect**, then **create a new save while connected**. Leave native
   **Enable Rando** off.

[Full setup instructions](PLAYING.md) · [Preview play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

The 0.4 client accepts 0.1–0.4 seeds and loads older saves tolerantly. A 0.1 client cannot play 0.4 seeds.
MMRecompRando (`mm_recomp`) seeds use a different world and are not compatible.

## What has been checked?

The current 0.4 source passed **531 tests**, **1200/1200 original-logic parity scenarios** with the entrance
options off, and **200/200 randomized seeds** across boss-only and combined entrance shuffles. A native harness
also exercised **2304 option/permutation cases**, **23040 warps** and **44352 respawn checks** with mocked game APIs.

These checks do not run the game. First multiworld sessions on the older feature set worked, but the new NPC
souls and entrance shuffles still need in-game testing. Linux/macOS runtime behavior is also untested.
The in-game check tracker does not evaluate extra access rules for foreign temples/boss rooms.

[Detailed results, limitations and reproduction checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

## Source & development

| Branch | Contents |
|---|---|
| [`client`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/client) | 2S2H 5.0.1 with the AP client and new mechanics; platform builds run in GitHub Actions. |
| [`apworld`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/apworld/worlds/mm_2s2h) | Archipelago world 0.4.0, generated logic, options and validation tools. |
| [`main`](https://github.com/junkx420/Updated-2S2H-Archipelago/tree/main) | Project overview, setup, changelog and preview packaging workflow. |

The generator reads the fork's C++ enums, checks, items, regions and feature tables. AP IDs derive from those
enum values, options travel as `RO_*` names, and saved pairings rebuild the same entrances in Universal Tracker.
The original 2S2H region/combat logic remains unchanged; parity checks guard the default behavior.

## Credits

- [Harbour Masters / 2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian) — game port and randomizer
- [xirba13 / 2S2H Archipelago](https://github.com/xirba13/2ship2harkinian-archipelago) — original AP client layer
- [Archipelago](https://github.com/ArchipelagoMW/Archipelago) — multiworld framework
- [black-sliver / apclientpp](https://github.com/black-sliver/apclientpp) — client library

Original licenses and credits remain in their source branches. No ROMs or extracted ROM data are distributed;
you need your own legally obtained game copy.
