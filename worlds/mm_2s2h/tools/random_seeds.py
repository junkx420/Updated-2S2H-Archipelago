"""Generate N seeds with randomized mm_2s2h options and report failures.

Usage (archipelago root, venv):  python worlds/mm_2s2h/tools/random_seeds.py <n> <work-dir> [option=value ...]
Extra option=value arguments override the rolled value in every seed (e.g. small_keys=own_dungeon).
Use mix_entrance_pools=all for all entrance shuffles, or mix_entrance_pools=random to vary the selected pools.
Each seed: random option set -> yaml -> Generate.py (full output incl. playthrough). Prints one line per seed
and a summary; failing yamls and logs are kept in <work-dir>/failed/.
"""
import json
import pathlib
import random
import runpy
import shutil
import subprocess
import sys
import time

GAME = "Majora's Mask (2 Ship 2 Harkinian)"
TOGGLES = ["clock_shuffle", "shuffle_owl_statues", "shuffle_shops", "shuffle_tingle_shops", "shuffle_boss_remains",
           "shuffle_cows", "shuffle_enemy_drops", "shuffle_frogs", "shuffle_butterflies", "shuffle_freestanding",
           "shuffle_wonder_items", "shuffle_gold_skulltulas", "shuffle_pots", "shuffle_crates", "shuffle_barrels",
           "shuffle_snowballs", "shuffle_grass", "shuffle_trees", "shuffle_hives", "shuffle_boss_souls",
           "shuffle_enemy_souls", "shuffle_npc_souls", "shuffle_swim", "shuffle_ocarina_buttons", "shuffle_song_sun",
           "shuffle_song_double_time", "shuffle_song_inverted_time", "shuffle_song_saria", "shuffle_tycoon_wallet",
           "shuffle_skeleton_key", "shuffle_dungeon_entrances", "shuffle_boss_entrances", "shuffle_door_keys", "shuffle_grottos", "shuffle_interiors", "shuffle_traps", "plentiful_items", "triforce_hunt", "starting_consumables",
           "starting_rupees", "starting_maps_and_compasses", "death_link"]
CHOICES = {"access_dungeons": ["form_and_song", "form_or_song", "form_only", "song_only", "open"],
           "access_trials": ["20_masks", "remains", "forms", "open"],
           "clock_shuffle_mode": ["any_order", "ascending", "descending"],
           "small_keys": ["anywhere", "own_dungeon", "start_with"], "boss_keys": ["anywhere", "own_dungeon", "start_with"],
           "stray_fairies": ["anywhere", "own_dungeon", "start_with"]}
RANGES = {"majora_remains_required": (0, 4), "majora_masks_required": (0, 20), "moon_remains_required": (0, 4),
          "moon_masks_required": (0, 20), "skulltulas_shuffled": (1, 30), "skulltula_tokens_required": (1, 30),
          "trap_amount": (1, 30), "triforce_pieces_max": (5, 60), "triforce_pieces_required": (1, 60),
          "stray_fairies_max": (1, 15), "stray_fairies_required": (1, 15), "starting_health": (1, 20)}


def roll(rng: random.Random) -> dict:
    density = rng.random()  # some seeds sparse, some allsanity-like
    opts = {t: rng.random() < density for t in TOGGLES}
    opts.update({k: rng.choice(v) for k, v in CHOICES.items()})
    opts.update({k: rng.randint(*v) for k, v in RANGES.items()})
    return opts


def main():
    n, work = int(sys.argv[1]), pathlib.Path(sys.argv[2]).resolve()
    shutil.rmtree(work, ignore_errors=True)
    (work / "failed").mkdir(parents=True)
    rng = random.Random(2026)
    forced = dict(a.split("=", 1) for a in sys.argv[3:])
    mix = forced.pop("mix_entrance_pools", None)
    pools = runpy.run_path(str(pathlib.Path(__file__).resolve().parents[1] / "enums_generated.py"))["ENTRANCE_MIX_POOLS"] if mix else {}
    mix_rng = random.Random(610)
    ok = fail = 0
    t0 = time.time()
    for i in range(n):
        opts = roll(rng) | forced
        if mix:
            selected = (mix_rng.sample(list(pools), mix_rng.randint(2, len(pools))) if mix == "random" else
                        list(pools) if mix == "all" else json.loads(mix))
            opts["mix_entrance_pools"] = json.dumps(selected)
            for name in selected:
                opts[pools[name][0].removeprefix("RO_").lower()] = True
            if mix == "all":
                opts.update(shuffle_boss_entrances=True, shuffle_grottos=True)
        pdir = work / f"p{i}"
        pdir.mkdir()
        body = "\n".join(f"  {k}: {str(v).lower() if isinstance(v, bool) else v}" for k, v in opts.items())
        (pdir / "p.yaml").write_text(f'name: Seed{i}\ngame: "{GAME}"\n"{GAME}":\n{body}\n', encoding="utf-8")
        t = time.time()
        r = subprocess.run([sys.executable, "Generate.py", "--player_files_path", str(pdir), "--outputpath",
                            str(work / "out"), "--seed", str(i)], input="\n", capture_output=True, text=True)
        dt = time.time() - t
        good = r.returncode == 0 and "Done. Enjoy." in r.stdout + r.stderr
        if good:
            ok += 1
        else:
            fail += 1
            shutil.copy(pdir / "p.yaml", work / "failed" / f"seed{i}.yaml")
            (work / "failed" / f"seed{i}.log").write_text(r.stdout + r.stderr, encoding="utf-8")
        tail = (r.stdout + r.stderr).strip().splitlines()[-1][:120] if not good else ""
        print(f"seed {i:3d} {'ok  ' if good else 'FAIL'} {dt:5.1f}s {tail}", flush=True)
        shutil.rmtree(pdir)
        shutil.rmtree(work / "out", ignore_errors=True)
    print(f"\n{ok}/{n} ok, {fail} failed, {time.time() - t0:.0f}s total")


if __name__ == "__main__":
    main()
