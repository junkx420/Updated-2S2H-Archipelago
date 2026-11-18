## Majora's Mask × Archipelago 0.12.0

**Every NPC can now be a soul.** Find a soul and its NPC appears in the world. Until then the NPC is simply not
there: no model, no shadow, no voice, no reward. Souls you find now show the NPC's own model instead of a blue
flame. Client and **apworld 0.12.0** are bundled together.

> **Regular release, new mechanics experimental.**
> Every automated check passes: 832 tests, 1200 of 1200 scenarios matching the original 2S2H logic, and 100 of 100
> random seeds with all NPC souls, alone and together with every other option of this fork. New options are **off by
> default**, and `classic` keeps exactly the 30 souls of 0.11.
> Nothing new in this release has been **played in game yet**. Use a separate save, keep backups, and please report
> anything odd.

### New since 0.11.1

| Feature | Option | What it does |
|---|---|---|
| **Souls for all NPCs** | `shuffle_npc_souls: all` | 65 NPC souls instead of 30: carpenters, Gorons, the Goron Elder and his son, Darmani, Zoras, the Indigo-Go's, Romani and Cremia, Kafei, Kotake and Koume, the Deku King and Princess, all shopkeepers, Bombers, guards, Dampé, the frogs and more. Without its soul an NPC is not there at all. |
| **Classic souls** | `shuffle_npc_souls: classic` | The 30 souls of 0.4 to 0.11, unchanged. Old YAMLs with `true` mean `classic`. |
| **NPC models** | always | A found soul shows the NPC it belongs to, in a blue light, on the ground, in shops and above Link's head. Kafei shows his mask, the lottery its kiosk. |
| **New item models** | always | Door keys show Anju's room key, Epona riding shows a horse. |
| **No shadow while hidden** | always | Hidden NPCs (and bosses without their soul) no longer cast a shadow. |

#### How hidden NPCs behave

- **Logic:** 141 checks and every event that needs an NPC require its soul. NPCs that only block a way (guards,
  Deku guards, pirates, the talking Gibdos, Biggoron) are not in the way while hidden. That only makes the game
  easier; the logic still expects them.
- **No invisible walls:** Cremia's cart has no collision while hidden.
- **Never hidden,** because the way forward depends on them: the Goron Shrine gatekeeper, Sakon (his hideout door),
  Pamela (the music box house), the Happy Mask Salesman, Skull Kid, Tatl and Tael, the Giants, the Moon children,
  the scarecrow, the Great Fairies and actors that only appear in cutscenes. Epona has her own riding item.
- **Small pools:** if a pool is too small for all 65 souls, the souls without checks go to the start inventory
  first, with a note in the generation log.

### Compatibility

- The 0.12 client accepts seeds from apworld **0.1 to 0.12**. Older saves load.
- Generate new seeds with the bundled **apworld 0.12.0**. Older clients cannot play 0.12 seeds.
- Everything from 0.11.1 is included: door keys, the entrance randomizer, abilities and the entrance tracker.

### What is checked, what is not

- **Checked automatically:** 832 tests, 1200/1200 parity scenarios with the new options off, 100/100 random seeds
  with `all`, and 100/100 with `all` plus every other option of this fork. Windows client built locally; all three
  platform packages built by GitHub Actions.
- **Not checked:** none of this has been played in game. Model size and position are estimated. Linux and macOS
  packages are untested in game.
- **Quick model check:** with a test save, open **Save Editor → Queue Randomizer Item Gives** and filter for
  "Soul of". This really gives the items, so never use your real save.
- **Play-test checklist:** [testing.md](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

### Coming next

Markers for objects whose check is still open, rocks as checks, and signs as checks (like Ocarina of Time).

### Download

Get the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from this release.
`BUILD-MANIFEST.json` lists the exact commits and SHA-256 checksums. Setup steps are in
`ARCHIPELAGO-INSTRUCTIONS.md` inside each client package.

Unofficial fan project. No ROM or extracted ROM data is included.
