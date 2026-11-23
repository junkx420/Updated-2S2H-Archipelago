# Changelog

## 0.18.0 — Regular release (new mechanics experimental)

Client and **apworld 0.18.0**. Generate new seeds with the bundled world. The client accepts apworld 0.1 to 0.18.

- **Shuffle Signs** (`shuffle_signs`) adds 87 sign and grave checks. Read the original text to collect the check.
  A chopped sign's stump stays readable while its check is open.
- **Check Tracker** applies fork rules for NPC souls, door keys, abilities and extra checks. Reachability follows
  the saved entrance assignments, including mixed pools, directed exits and blue boss warps.
- **Shuffle Owl Warps** (`shuffle_owl_warps`) shuffles the ten Song of Soaring destinations. Activation, checks
  and saves stay at their original statues.
- **Shuffle Spawn** (`shuffle_spawn`) uses the same seed-assigned Clock Town district for a fresh save and after
  each Song of Time rewind. The district has direct access to South Clock Town.
- **Decouple Entrances** (`decouple_entrances`) lets eligible directions lead to different destinations while
  preserving source rules and checking escapes.
- **Mixed Grottos** lets `mix_entrance_pools` include 18 of the 26 grotto holes with `shuffle_grottos` enabled.
  Grotto pairs stay coupled.
- **Build fix** loads the JSON dependency before the forced tracker header loads the region logic. Windows,
  Linux and macOS builds passed.
- **Validation** passed 950 tests, 164,096 subtests, 1200/1200 parity scenarios and two runs of 100/100 seeds.
  The new shuffle options are off by default. The changes are **not tested in game**.

[Release notes and downloads](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.18.0)

## 0.13.0 — Regular release (new mechanics experimental)

Client and **apworld 0.13.0**. Generate new seeds with the bundled world. The client accepts apworld 0.1 to 0.13.

- **Shuffle Rocks** (`shuffle_rocks`, off by default) adds 122 checks across small rocks, blast boulders, bronze
  boulders, rock rings and rolling boulders.
  [Rock placement exceptions and reasons](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md#stage-9--shuffle-rocks-apworld-0130)
- **Open check markers** tint snowballs, trees, beehives, the Swordsman's School gong and owl statues yellow or by
  item class with CSMC. The music wall and Song of Soaring engraving sparkle. Once collected, the objects return
  to their usual appearance.
- **Clock Shuffle** starts a new AP save in the earliest owned half-day. File select waits for the server's
  initial items. Press **B** to go back. Existing saves in the Final Hours stay there on load.
- **Validation:** 861 tests passed, parity 1200/1200, and 100/100 seeds generated with Shuffle Rocks alone and
  100/100 with it plus all fork options. The changes are **not tested in game**.

[Release notes and downloads](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.13.0)

## 0.12.0 — Regular release (new mechanics experimental)

Client and **apworld 0.12.0**. Every automated check passes and new options are off by default; nothing new has been
**played in game** yet.

- **Souls for all NPCs** (`shuffle_npc_souls: all`): 65 souls. Without its soul an NPC is not in the world: no model,
  shadow, voice or reward. `classic` (and `true` in old YAMLs) keeps the 30 souls of 0.11 unchanged.
- NPCs that open a way stay (Goron Shrine gatekeeper, Sakon, Pamela, story and cutscene characters). Blockers such
  as guards and pirates are simply gone while hidden. Cremia's cart has no collision while hidden.
- Small pools put souls without checks into the start inventory instead of failing generation.
- **Models:** found souls show their NPC instead of a blue flame; door keys and Epona riding have their own models.
- Hidden NPCs and bosses no longer cast a shadow.
- Validation: 832 tests, 1200/1200 parity, 100/100 seeds with `all` alone and with every option. Not tested in game.

[Release notes and downloads](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.12.0)

## 0.11.1 — Regular release (new mechanics experimental)

Client and **apworld 0.11.1**. Published as a regular release because every automated check passes and every new
option is off by default; the new mechanics have **not been played in game** and remain experimental.

### New since 0.4.0

- **Door keys** (`shuffle_door_keys`): 26 buildings locked until their key is found; ways out never locked.
- **Grotto shuffle** (`shuffle_grottos`): holes lead into other grottos; each grotto keeps its checks; you return
  to the hole you used.
- **Interior shuffle** (`shuffle_interiors`): building doors swap within the same opening-hours class.
- **Overworld shuffle** (`shuffle_overworld`): outdoor exits coupled; Clock Town gates always lead out; underwater
  ways mix only with each other; Link dismounts Epona on a shuffled exit.
- **Special areas** (`shuffle_special_areas`): Spider Houses, Beneath the Well, Ikana Castle, Secret Shrine, Deku Shrine.
- **Mixed pools** (`mix_entrance_pools`): temples, interiors, overworld and special areas in one pool; the inverted
  Stone Tower Temple joins with temples. Boss rooms and grottos stay in their own pools.
- **Abilities**: `shuffle_epona_riding` and `shuffle_bean_planting` add items that unlock riding and planting.
- **Entrance tracker**: spoiler-free in-game window listing shuffled entrances; discoveries are saved.
- Overfull pools drop traps and extra Plentiful copies instead of failing generation.

### Validation and limits

- 805 tests; 1200/1200 parity scenarios with new options off; 100/100 random seeds per new option and combined;
  native test bench for every entrance pairing, warps, respawns and old saves.
- Not tested in game. Linux/macOS untested in game. The in-game check tracker does not evaluate shuffled entrances.
- Fixed exits and their reasons: [testing.md](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

[Release notes and downloads](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.11.1)

## 0.4.0 — Pre-release

This is the first downloadable preview containing the changes after regular release **0.1.5**.
It bundles a new client and **apworld 0.4.0**. The new features have **not been tested in game**.

### New since 0.1.5

- **NPC souls:** 30 additional souls for townsfolk and other NPCs, including Anju, the Banker, Tingle and cows.
  Their rewards, sales and minigames require their soul. Off by default.
- **Dungeon entrance shuffle:** the four normal temple entrances can lead to another temple, with coupled exits.
  The inverted Stone Tower exterior and Woodfall prison door stay fixed. Off by default.
- **Boss entrance shuffle:** Odolwa, Goht, Gyorg and Twinmold can be behind different boss doors, independently of
  dungeon entrance shuffle. Boss keys stay at the door; fights, hearts, warp rewards and clear effects follow the boss.
  Majora and the Moon stay unchanged. Off by default.
- **Composed blue-warp returns:** when either shuffle moves a room, its blue warp returns outside the temple whose
  boss door was used. The completion scene is skipped where necessary; the boss's clear effect is preserved.
  Death/void return points are refreshed at the destination, including first-warps through the Giants' Chamber skip.
- **Area requirements in logic:** foreign temples and boss rooms add conservative item requirements to preserve
  assumptions made by the original temple logic. The in-game check tracker does not evaluate these extra rules.
- **Saved entrance pairings and Universal Tracker:** both shuffles survive saving/loading and rebuild exactly from slot data.
- **Other worlds' item presentation:** the Archipelago model and get-item animation reflect filler, useful and
  progression/trap classification. Named items, owners and exact-location hints remain supported.
- **DeathLink:** bottled-fairy revivals no longer report a shared death.
- **Compatibility:** the new client accepts apworld 0.1, 0.2, 0.3 and 0.4 seeds; old save fields load tolerantly.
  Use the bundled 0.4.0 apworld and client for new 0.4 seeds. Older clients cannot run 0.4 seeds.

### Validation and limits

- **531 tests passed**; **1200/1200 parity scenarios** match the original logic with both entrance options off.
- **100/100 boss-only** and **100/100 combined entrance-shuffle seeds** generated with other options randomized.
- The native mocked-client harness checks **2304 option/permutation cases**, **23040 warps** and **44352 respawns**.
- Windows Release client built locally. Published platform packages come from the exact client commit's GitHub Actions build.
- These automated checks do not run the game. NPC souls, shuffled transitions, cutscenes and full playthroughs
  still need in-game testing. Linux and macOS packages have not been tested in game.
- Grottos, houses, overworld transitions, spider houses and Pirate's Fortress are not shuffled.

[Full validation and play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

## 0.1.5 — Regular release

The established download line: client **0.1.5**, bundled apworld **0.1.4**. It has limited play coverage and is
not a guarantee of bug-free seeds. The newer NPC soul and entrance-shuffle features are not included.

- In-game hints with an exact item location also become Archipelago hints: gossip stones, purchased hints,
  the bank sign and Spider House hints. Region-only hints stay in game.
- Includes earlier 0.1.x improvements: named multiworld items and owners, item/location groups, Universal Tracker,
  logic-aware own-dungeon item placement, No Logic and Nearly No Logic, and a weapon requirement for Majora.
- Windows, Linux and macOS client packages, apworld and example YAML are available on the release page.

[Download 0.1.5 and read its original release notes](https://github.com/junkx420/Updated-2S2H-Archipelago/releases/tag/v0.1.5)
