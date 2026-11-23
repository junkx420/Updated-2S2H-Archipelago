# Playing with Archipelago

1. Download the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from release **0.18.0**.
   Install [Archipelago 0.6.6 or newer](https://archipelago.gg).
2. Extract the client package. Provide your own legally obtained **Majora's Mask (US)** ROM on first launch.
3. Double-click `mm_2s2h.apworld` to install it. Edit your name and options in `mm_2s2h_example.yaml`, place it
   in Archipelago's `Players` directory, generate and host your multiworld.
4. Start the client with `2ship.exe` on Windows, the AppImage on Linux, or the app in the DMG on macOS.
   Keep the packaged files together, including `networking/cacert.pem` for secure server connections.
5. Press **ESC → Dev Tools → Archipelago**, enter the server address, slot name and password, then connect.
6. **While connected, create a new save file.** Leave native **Enable Rando** off; the multiworld supplies the options.

## Linux

Extract the Linux ZIP, then run `chmod +x 2ship.appimage` and `./2ship.appimage` from the extracted folder.
If the AppImage reports missing FUSE, install `libfuse2` (`libfuse2t64` on Ubuntu 24.04) or launch it with
`./2ship.appimage --appimage-extract-and-run`. The Linux package is **not tested in game**.
[AppImage FUSE troubleshooting](https://docs.appimage.org/user-guide/troubleshooting/fuse.html)

## Trying the new mechanics (0.18.0)

NPC souls, door keys, Epona riding, bean planting and all entrance shuffles are optional and off by default.
Enable them in your YAML (`shuffle_npc_souls`, `shuffle_door_keys`, `shuffle_epona_riding`, `shuffle_bean_planting`,
`shuffle_dungeon_entrances`, `shuffle_boss_entrances`, `shuffle_grottos`, `shuffle_interiors`, `shuffle_overworld`,
`shuffle_special_areas`, `mix_entrance_pools`). The example YAML contains a ready-made block to shuffle everything.
`shuffle_npc_souls: all` hides every NPC until its soul is found; `classic` keeps the 30 souls of 0.11.
Use a separate save and back up existing saves before experimenting.

Enable `shuffle_rocks` to add 122 rock checks. Open checks on marked objects are tinted or sparkle until collected.
Enable `shuffle_signs` to add 87 checks that are awarded after reading the original sign or grave text.
With `clock_shuffle`, a new AP save waits in file select for the server's initial items, then starts in the
earliest half-day it owns. Press **B** to go back. Existing saves in the Final Hours stay there on load.

Enable `shuffle_owl_warps` to shuffle the ten destinations of the Song of Soaring. Enable `shuffle_spawn` to start
each Song of Time rewind in the same seed-assigned district in North, West or East Clock Town. Each district has a direct route
to South Clock Town.

Enable `decouple_entrances` for independently shuffled directions on eligible exits. To mix grottos with other
entrances, turn on `shuffle_grottos` and add `grottos` to `mix_entrance_pools` with at least one other enabled
pool. In mixed mode, 18 holes can shuffle and eight stay fixed. Grotto pairs stay coupled.

With shuffled entrances, open **Entrance Tracker** next to the check tracker: it remembers where each entrance led
and hides destinations you have not visited unless you turn on **Show All (Spoilers)**.

The **Check Tracker** applies AP fork rules and follows the saved entrance assignments when it calculates reachability.
Universal Tracker recreates the same assignments from the slot data.

The 0.18 client accepts apworld 0.1 to 0.18 seeds. Generate new seeds with the bundled apworld 0.18.0.
The new mechanics are **not tested in game**.

Report problems in [this repository's issues](https://github.com/junkx420/Updated-2S2H-Archipelago/issues),
with the release, platform, relevant options and reproduction steps. Do not send support requests to the original projects.

[Play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

No ROM or extracted ROM data is provided. This is an unofficial fan project, unaffiliated with the original developers.
