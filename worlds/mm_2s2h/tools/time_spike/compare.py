"""Phase-0 check: C++ (verbatim 2S2H code) vs Python reference port vs closed form.

Usage: python compare.py <harness.exe> [n_random]
"""
from __future__ import annotations

import random
import subprocess
import sys
from itertools import product

from time_logic import TIME_ALL_SLICES, TimeContext
from time_reference import Evaluator, Region, solve

# ---- 1. real subgraph, transcribed from 2S2H 5.0.1 Logic/Regions/Central.cpp ---------------------
# items: 0 room key, 1 RE_MAIL_LETTER_TO_KAFEI, 2 RE_MEET_KAFEI, 3 Kafei's Mask, 4 Romani Mask,
#        5 RE_SAVE_BOMB_SHOP_LADY / RE_KILL_SAKON
RR_MAX, SCT, ECT, WCT, INN, MILK, CS_FRONT, LAUNDRY, CS_BACK = range(9)
NO = "false"
REAL = {
    RR_MAX: Region(conns={SCT: "true"}),
    SCT: Region(conns={ECT: "true", WCT: "true", LAUNDRY: "true"}),
    ECT: Region(conns={SCT: "true",
                       INN: "item:0|between:2:8|between:18:24|after:33",
                       MILK: "between:3:9|between:19:25|after:33"}),
    WCT: Region(conns={SCT: "true", CS_FRONT: "between:10:16|between:26:31|after:40"}),
    INN: Region(conns={ECT: "true"}, stay={8: "item:0", 16: "item:0", 24: "item:0", 31: "item:0"}),
    MILK: Region(conns={ECT: "true"}, stay={10: NO, 15: NO, 26: NO, 30: NO, 40: NO, 44: NO}),
    CS_FRONT: Region(conns={WCT: "true"}),
    LAUNDRY: Region(conns={SCT: "true", CS_BACK: "item:1&between:22:26|item:2&between:36:40"}),
    CS_BACK: Region(conns={LAUNDRY: "true"}, stay={26: NO, 40: NO, 16: NO, 31: NO}),
}
REAL_CHECKS = [  # (region, id, condition)
    (CS_BACK, 0, "between:36:40"),                       # Kafei's Hideout Keaton Mask
    (CS_BACK, 1, "item:1&between:22:26"),                # Pendant of Memories
    (CS_FRONT, 2, "night3"),                             # Curiosity Shop item
    (CS_FRONT, 3, "item:5&night3"),                      # Curiosity Shop special item
    (INN, 4, "item:3&between:4:9"),                      # RE_SETUP_MEET_ANJU (event, as check)
    (INN, 5, "item:1&between:16:25|item:1&between:31:35"),  # RE_DELIVER_PENDANT-like
    (MILK, 6, "item:4&between:10:15|item:4&between:26:30"),  # Circus Leader's Mask time part
    (MILK, 7, "item:3&between:37:39|item:3&after:40"),   # Madame Aroma
]


def scenario_text(name, regions, start, checks, shuffle, mode, flags, items) -> str:
    out = [f"scenario {name}", f"options {int(shuffle)} {mode} {flags} {items}"]
    for rid, r in regions.items():
        out.append(f"region {rid} {int(r.can_stay)}")
    out.append(f"start {start}")
    for rid, r in regions.items():
        for s, c in r.stay.items():
            out.append(f"stay {rid} {s} {c}")
        for t, c in r.conns.items():
            out.append(f"conn {rid} {t} {c}")
    for rid, cid, c in checks:
        out.append(f"check {rid} {cid} {c}")
    out.append("end")
    return "\n".join(out)


def python_result(regions, start, checks, shuffle, mode, flags, items):
    ev = Evaluator(shuffle, mode, flags, items)
    masks = solve(regions, start, ev)
    res = {}
    for rid, cid, c in checks:
        if rid in masks:
            ev.cur_mask = masks[rid]
            res[cid] = int(ev.eval(c))
        else:
            res[cid] = 0
    return masks, res


def closed_form(regions, start, checks, shuffle, mode, flags, items):
    """What the apworld will do: one uniform mask, plain reachability."""
    ctx = TimeContext(shuffle, mode, flags)
    ev = Evaluator(shuffle, mode, flags, items)
    uniform = ctx.mask  # ALL without clock shuffle, owned slices with it
    reach, todo = {start}, [start]
    while todo:
        r = todo.pop()
        ev.cur_mask = uniform if (r != start or shuffle) else 1  # start region: Day 1 06:00 only
        for t, c in regions[r].conns.items():
            if t not in reach and ev.eval(c):
                reach.add(t)
                todo.append(t)
    masks = {r: (uniform if (r != start or shuffle) else 1) for r in reach}
    res = {}
    for rid, cid, c in checks:
        ev.cur_mask = masks.get(rid, 0)
        res[cid] = int(rid in reach and ev.eval(c))
    return masks, res


def parse_cpp(text: str):
    out, cur = {}, None
    for line in text.splitlines():
        p = line.split()
        if p[0] == "scenario":
            cur = out[p[1]] = ({}, {})
        elif p[0] == "mask":
            cur[0][int(p[1])] = int(p[2], 16)
        elif p[0] == "check":
            cur[1][int(p[1])] = int(p[2])
    return out


# ---- 2. random graphs --------------------------------------------------------------------------
def rand_cond(rng, allow_time=True):
    def atom():
        k = rng.random()
        if k < 0.25 or not allow_time:
            return rng.choice(["true", "true", f"item:{rng.randrange(6)}"])
        s = rng.randrange(45)
        e = rng.randrange(s, 46)
        return rng.choice([f"at:{s}", f"before:{s}", f"after:{s}", f"between:{s}:{e}",
                           rng.choice(["day1", "night1", "day2", "night2", "day3", "night3", "isday", "isnight"])])
    return "|".join("&".join(atom() for _ in range(rng.randint(1, 2))) for _ in range(rng.randint(1, 3)))


def rand_graph(rng, precondition: bool):
    n = rng.randint(4, 12)
    regions = {i: Region() for i in range(n)}
    for i in range(n):
        if rng.random() < 0.3:
            for _ in range(rng.randint(1, 4)):
                regions[i].stay[rng.randrange(45)] = rng.choice(["false", f"item:{rng.randrange(6)}"])
        for _ in range(rng.randint(1, 3)):
            t = rng.randrange(n)
            if t != i:
                regions[i].conns[t] = rand_cond(rng, allow_time=not (precondition and i == 0))
    if precondition:  # mirrors the 5.0.1 graph: RR_MAX has no time conditions, its targets have no STAY
        for t in regions[0].conns:
            regions[t].stay.clear()
        for r in regions.values():  # and nothing leads back into RR_MAX
            r.conns.pop(0, None)
    checks = [(rng.randrange(n), k, rand_cond(rng)) for k in range(6)]
    return regions, checks


def main():
    exe = sys.argv[1]
    n_random = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
    rng = random.Random(2026)
    cases = []
    for shuffle, mode, flags, items in product([False, True], [0, 1, 2], range(64), [0, 0b111111, 0b000001, 0b011110]):
        if not shuffle and (mode or flags):
            continue
        cases.append(("real", REAL, RR_MAX, REAL_CHECKS, shuffle, mode, flags, items, True))
    for k in range(n_random):
        pre = k % 2 == 0
        regions, checks = rand_graph(rng, pre)
        shuffle = rng.random() < 0.5
        cases.append((f"rand{'P' if pre else 'X'}", regions, 0, checks, shuffle, rng.randrange(3),
                      rng.randrange(64), rng.randrange(64), pre))
    names = [f"{c[0]}_{i}" for i, c in enumerate(cases)]
    feed = "\n".join(scenario_text(n, *c[1:8]) for n, c in zip(names, cases)) + "\n"
    cpp = parse_cpp(subprocess.run([exe], input=feed, capture_output=True, text=True, check=True).stdout)

    stats = {"port_ok": 0, "port_bad": 0, "closed_ok": 0, "closed_bad_pre": 0, "closed_bad_nopre": 0,
             "nonuniform_nopre": 0}
    for n, c in zip(names, cases):
        args = c[1:8]
        py = python_result(*args)
        if py == cpp[n]:
            stats["port_ok"] += 1
        else:
            stats["port_bad"] += 1
            if stats["port_bad"] <= 3:
                print("PORT MISMATCH", n, "\n cpp", cpp[n], "\n py ", py)
        cf = closed_form(*args)
        if cf == cpp[n]:
            stats["closed_ok"] += 1
        elif c[8]:
            stats["closed_bad_pre"] += 1
            if stats["closed_bad_pre"] <= 3:
                print("CLOSED-FORM MISMATCH (precondition holds!)", n, "\n cpp", cpp[n], "\n cf ", cf)
        else:
            stats["closed_bad_nopre"] += 1
        if not c[8]:
            ms = {m for r, m in cpp[n][0].items() if r != 0}
            stats["nonuniform_nopre"] += len(ms) > 1
    print(f"scenarios: {len(cases)} (real subgraph: {sum(1 for c in cases if c[0] == 'real')})")
    for k, v in stats.items():
        print(f"  {k}: {v}")
    real_masks = {cpp[n][0][CS_BACK] for n, c in zip(names, cases) if c[0] == "real" and CS_BACK in cpp[n][0]}
    print("  real subgraph, distinct CS_BACK masks:", sorted(hex(m) for m in real_masks)[:8])
    return 1 if stats["port_bad"] or stats["closed_bad_pre"] else 0


if __name__ == "__main__":
    sys.exit(main())
