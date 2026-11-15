# Playing 2 Ship 2 Harkinian with Archipelago

This fork turns 2S2H into a client for the **"Majora's Mask (2 Ship 2 Harkinian)"**
Archipelago world (`mm_2s2h.apworld`). That apworld is generated from 2S2H's own
randomizer, so every native option works in a multiworld: grass, pots, crates,
barrels, hives, snowballs, trees, enemy drops, butterflies, frogs, owl statues,
souls, clocks, ocarina buttons and the rest.

## What you need

1. **2 Ship 2 Harkinian (this fork)** — grab the zip from the
   [Releases](../../releases) page (contains `2ship.exe` and
   `networking/cacert.pem`). As with regular 2S2H, you must provide your own
   legally dumped Majora's Mask US ROM on first launch — see the Quick Start
   section of the [README](README.md).
2. **Archipelago** (0.6.x or newer) — from [archipelago.gg](https://archipelago.gg).
3. **The apworld + yaml** — both attached to the release:
   - `mm_2s2h.apworld` (do **not** rename this file, the name must match the
     Python module inside)
   - `2S2H-Archipelago.yaml` (a template yaml for this project)

## Setup

1. Double-click `mm_2s2h.apworld` (or place it in Archipelago's
   `custom_worlds` folder).
2. Edit `2S2H-Archipelago.yaml`: set your `name` and any options you want,
   then put it in Archipelago's `Players` folder.
3. Run **Generate** from the Archipelago Launcher, then host the resulting
   multiworld (locally via **Host**, or upload to archipelago.gg).

## Playing

1. Launch `2ship.exe` (make sure `networking/cacert.pem` sits next to it —
   it's needed for `wss://` servers like archipelago.gg).
2. Press **ESC** → **Dev Tools** → **Archipelago**.
3. Enter the server address (`localhost:38281` for a local host, or
   `archipelago.gg:<port>`), your slot name, and the password if any. Hit
   **Connect** — you should see a green *Connected* and a summary of your
   slot's options.
4. **While connected, create a new save file.** It automatically becomes an
   Archipelago save: you start in South Clock Town as Human, and every check
   contains exactly what the multiworld placed there.
   - Leave the native randomizer (**Rando → Enable Rando**) **off**; the
     multiworld replaces it.
5. Play! Opening a check sends it to the server; items for you are granted
   instantly with the normal item fanfare; items for other players show an
   Archipelago logo and a "Sent X to Player" toast. Items other players find
   for you pop in with a notification, even while you're elsewhere.

## Good to know

- **Reconnecting / resuming:** connect to the same slot, load the same save —
  the client resyncs both directions automatically. Progress made offline is
  uploaded on the next connect; nothing is ever granted twice.
- **One save per slot.** Don't reuse a save across different multiworlds —
  create a fresh file for each seed.
- **DeathLink** is honored automatically when your yaml enables it.
- **Goal:** defeat Majora. The server is informed automatically.
- **Goal with Triforce Hunt:** collect the required pieces instead.
- **Versions must match:** the apworld is generated from one 2S2H version
  (shown in the console on connect); use the client build of that version.
- The console window (popout button in the menu) shows the full session log
  and server chat.

## Troubleshooting

- **"Connection error"** — check address/port; for local hosting make sure
  the ArchipelagoServer is running first.
- **"TLS handshake failed" then connected** — normal: the client tries
  secure first, then falls back for local plaintext servers.
- **Items/chests look vanilla** — you created the file while disconnected, or
  native "Enable Rando" was on. Connect first, then create a new file.
- **`wss://` fails** — `networking/cacert.pem` is missing next to the exe.
