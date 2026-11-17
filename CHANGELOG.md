# Changelog

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
