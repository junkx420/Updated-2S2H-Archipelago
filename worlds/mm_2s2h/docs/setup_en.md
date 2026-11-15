# Majora's Mask (2 Ship 2 Harkinian) Setup Guide

> Unofficial fan project. Not made by, affiliated with or supported by the Harbour Masters, the 2 Ship 2 Harkinian
> team or the Archipelago project. Please do not send them support requests about it.

## Required software

- The 2 Ship 2 Harkinian **client build** from the
  [Releases](https://github.com/junkx420/Updated-2S2H-Archipelago/releases) of `junkx420/Updated-2S2H-Archipelago`
  (`2S2H-Archipelago-<version>-Windows.zip`). The official 2S2H release does not include an Archipelago client.
- Your **own legally obtained** Majora's Mask (US) ROM. 2S2H asks for it on first launch.
- [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) 0.6.6 or newer.
- `mm_2s2h.apworld` from the same repository (double-click it to install).

Client and apworld must come from the same version of the repository. The client refuses seeds generated with
a different apworld major.minor version.

## Creating a seed

1. Start from `example.yaml` (every option with its description) or the template from the Archipelago Launcher.
2. Set your `name` and options, put the file into Archipelago's `Players` folder and run **Generate**.
3. Host the result with **Host** (local) or upload it to archipelago.gg.

All options are the native options of 2S2H's own randomizer (Rando menu), with the same defaults.

## Playing

1. Start `2ship.exe` (keep `networking/cacert.pem` next to it for `wss://` servers).
2. **ESC → Dev Tools → Archipelago**: enter server address, slot name, password; **Connect**.
3. **While connected, create a new save file.** It becomes an Archipelago save automatically.
   Leave **Rando → Enable Rando** off.
4. Every check you collect is sent to the server; items from other worlds arrive with a notification.

Reconnecting resumes both directions; nothing is granted twice. Goal: defeat Majora (or complete the Triforce
Hunt), which is reported automatically.
