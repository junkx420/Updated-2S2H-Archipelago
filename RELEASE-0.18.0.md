## Majora's Mask × Archipelago 0.18.0

**Read signs for checks, shuffle owl flights and cycle starts, and mix grotto holes with other entrances.** The
client and **apworld 0.18.0** are bundled together.

> **Regular release, new mechanics experimental.**
> The automated checks passed. The new shuffle options are **off by default** and the new mechanics are
> **not tested in game**.

### New since 0.13.0

| Feature | Option | What it does |
|---|---|---|
| **Shuffle Signs** | `shuffle_signs` | Adds 87 checks to the `Signs` group. Read a sign or grave and finish its original text or hint to collect the check. A chopped sign's stump stays readable until its check is collected. |
| **Check Tracker** | automatic | Applies fork rules for NPC souls, door keys, abilities and extra checks. It follows the saved entrance assignments, including mixed pools, directed exits, grottos and blue boss warps. |
| **Shuffle Owl Warps** | `shuffle_owl_warps` | Sends each of the ten Song of Soaring selections to a different owl for the whole seed. Owl activations, checks and saves stay at their original statues. Some flights require an escape item or a playable Song of Time. |
| **Shuffle Spawn** | `shuffle_spawn` | Starts new AP saves and each rewind in North, West or East Clock Town. The district stays fixed for the seed and keeps its direct route to South Clock Town. |
| **Decouple Entrances** | `decouple_entrances` | Draws the two directions independently on eligible routes. Source doors retain their keys and rules, and the generator checks that arrivals have a way out. Town gates, boss doors, water pairs and grotto pairs keep their coupling. |
| **Mixed Grottos** | `shuffle_grottos` and `mix_entrance_pools` | Add `grottos` alongside another enabled pool to mix 18 holes with temples, buildings, land routes or special areas. Eight holes stay fixed in mixed mode. The separate grotto pool still shuffles all 26. |
| **Linux and macOS builds** | automatic | The tracker header now loads its JSON dependency before it loads the region logic, so the Linux and macOS compilers can build the client. |

With AP fork rules enabled, the Check Tracker starts its reachability search at the same root as the apworld.
With those rules off, the current region remains an additional starting point. The Entrance Tracker reveals
shuffled routes as you use them.

### Compatibility

- The 0.18 client accepts seeds from apworld **0.1 to 0.18** and loads older saves.
- Generate new seeds with the bundled **apworld 0.18.0**.

### Testing

- The 0.18.0 world passed **950 tests and 164,096 subtests**, including all **78** Golden fingerprints.
  Random generation passed **100/100** seeds with all five mixed pools and **100/100** with all fork options.
- The build fix passed **1200/1200** parity scenarios with fork options off. The 0.18.0 Check Tracker matched
  **600 graphs**, **4,800 inventory snapshots** and **32 targeted cases**. The native transition harness also
  validated **15** AP-generated 0.18.0 slots.
- GitHub Actions built the Windows, Linux and macOS packages. The new mechanics and the client packages are
  **not tested in game**.
- [Detailed test results and play-test checklist](https://github.com/junkx420/Updated-2S2H-Archipelago/blob/apworld/worlds/mm_2s2h/docs/testing.md)

### Download

Get the client for your platform, `mm_2s2h.apworld` and `mm_2s2h_example.yaml` from this release.
`BUILD-MANIFEST.json` lists the exact commits and SHA-256 checksums. See `ARCHIPELAGO-INSTRUCTIONS.md`
inside each client package for setup.

#### Linux

Extract the Linux ZIP. In the extracted folder, run `chmod +x 2ship.appimage` and then `./2ship.appimage`.
If the AppImage reports missing FUSE, install `libfuse2` (`libfuse2t64` on Ubuntu 24.04) or launch it with
`./2ship.appimage --appimage-extract-and-run`. The Linux package is **not tested in game**.
[AppImage FUSE troubleshooting](https://docs.appimage.org/user-guide/troubleshooting/fuse.html)

Unofficial fan project. No ROM or extracted ROM data is included.
