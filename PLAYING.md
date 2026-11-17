# Playing with Archipelago

1. Download the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from the **same release**.
   Install [Archipelago 0.6.6 or newer](https://archipelago.gg).
2. Extract the client package. Provide your own legally obtained **Majora's Mask (US)** ROM on first launch.
3. Double-click `mm_2s2h.apworld` to install it. Edit your name and options in `mm_2s2h_example.yaml`, place it
   in Archipelago's `Players` directory, generate and host your multiworld.
4. Start the client: Windows `2ship.exe`, Linux the AppImage (make it executable), macOS the app in the DMG.
   Keep the packaged files together, including `networking/cacert.pem` for secure server connections.
5. Press **ESC → Dev Tools → Archipelago**, enter the server address, slot name and password, then connect.
6. **While connected, create a new save file.** Leave native **Enable Rando** off; the multiworld supplies the options.

## Trying 0.4.0

NPC souls, temple entrance shuffle and boss entrance shuffle are optional and off by default. Enable
`shuffle_npc_souls`, `shuffle_dungeon_entrances` or `shuffle_boss_entrances` in your YAML to try them.
Use a separate save and back up existing saves before experimenting.

The 0.4 client accepts 0.1–0.4 seeds. A 0.1 client cannot play a 0.4 seed. The in-game check tracker does not
evaluate extra access requirements for foreign temples/boss rooms; use the generated spoiler or Universal Tracker
when checking those requirements. The new features are **not tested in game**.

Report problems in [this repository's issues](https://github.com/junkx420/Updated-2S2H-Archipelago/issues),
with the release, platform, relevant options and reproduction steps. Do not send support requests to the original projects.

[Play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

No ROM or extracted ROM data is provided. This is an unofficial fan project, unaffiliated with the original developers.
