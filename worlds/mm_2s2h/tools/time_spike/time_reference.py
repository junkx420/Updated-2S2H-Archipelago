"""Faithful Python port of the 2S2H per-region time propagation (Logic.cpp FindReachableRegions,
TimeLogic.cpp ExpandTimeForward, GlitchlessLogic outer loop). Used only as a reference to check
(a) that the port matches the C++ code and (b) when the closed form in time_logic.py is valid.
It is also the fallback implementation if a future 2S2H release makes the masks non-uniform.
"""
from __future__ import annotations

from dataclasses import dataclass, field

from time_logic import TIME_ALL_SLICES, TIME_DAY1_AM_06_00, TIME_SLICE_COUNT, TimeContext


@dataclass
class Region:
    can_stay: bool = True
    stay: dict[int, str] = field(default_factory=dict)  # slice -> condition
    conns: dict[int, str] = field(default_factory=dict)  # target -> condition (insertion = std::map order)


class Evaluator:
    """Same condition mini-language as harness.cpp."""

    def __init__(self, ctx_shuffle: bool, mode: int, clock_flags: int, items: int):
        self.shuffle, self.mode, self.flags, self.items = ctx_shuffle, mode, clock_flags, items
        self.ctx = TimeContext(ctx_shuffle, mode, clock_flags)
        self.cur_mask = 0

    def _atom(self, a: str) -> bool:
        p = a.split(":")
        k = p[0]
        c = self.ctx
        c.mask = self.cur_mask  # operators read gCurrentRegionTime
        if k == "true": return True
        if k == "false": return False
        if k == "item": return bool(self.items >> int(p[1]) & 1)
        if k == "at": return c.at(int(p[1]))
        if k == "before": return c.before(int(p[1]))
        if k == "after": return c.after(int(p[1]))
        if k == "between": return c.between(int(p[1]), int(p[2]))
        return {"day1": c.is_day1, "night1": c.is_night1, "day2": c.is_day2, "night2": c.is_night2,
                "day3": c.is_day3, "night3": c.is_night3, "isday": c.is_day, "isnight": c.is_night}[k]()

    def eval(self, expr: str) -> bool:
        return any(all(self._atom(a) for a in conj.split("&")) for conj in expr.split("|"))

    def owned(self) -> int:
        return TimeContext(self.shuffle, self.mode, self.flags).mask


def expand_time_forward(ev: Evaluator, t: int, region: Region) -> int:
    if not region.stay and not ev.shuffle:
        e = t
        for sh in (1, 2, 4, 8, 16, 32):
            e |= e << sh
        return e & TIME_ALL_SLICES
    f = t & ev.owned() if ev.shuffle else t
    e, can_wait = f, False
    for i in range(TIME_SLICE_COUNT):
        m = 1 << i
        if f & m:
            can_wait = True
            e |= m
        elif can_wait:
            if ev.shuffle and not (ev.owned() & m):
                can_wait = False
                continue
            if i in region.stay:
                if ev.eval(region.stay[i]):  # NB: C++ evaluates with a stale gCurrentRegionTime too
                    e |= m
                else:
                    can_wait = False
            else:
                e |= m
    return e


def solve(regions: dict[int, Region], start: int, ev: Evaluator) -> dict[int, int]:
    """Returns region -> time mask for every reachable region (same visiting order as C++)."""
    states: dict[int, list] = {start: [ev.owned() if ev.shuffle else 1 << TIME_DAY1_AM_06_00, False]}

    def find(cur: int, reach: set[int]) -> None:
        if cur not in states:
            states[cur] = [ev.owned(), regions[cur].can_stay]
        st = states[cur]
        t = st[0]
        if st[1]:
            t = expand_time_forward(ev, t, regions[cur])
            st[0] = t
        for tgt in sorted(regions[cur].conns):  # std::map iterates in key order
            ev.cur_mask = t
            if ev.eval(regions[cur].conns[tgt]):
                inc = [t, regions[tgt].can_stay]
                if tgt in states:
                    ex = states[tgt]
                    if ex[0] | inc[0] == ex[0]:
                        continue
                    states[tgt] = [ex[0] | inc[0], ex[1] or inc[1]]
                else:
                    reach.add(tgt)
                    states[tgt] = inc
                find(tgt, reach)

    reach = {start}
    while True:
        before = len(reach)
        for r in sorted(reach):  # approximation of iterating a growing std::set; fixpoint is the same
            find(r, reach)
        if len(reach) == before:
            break
    ev.cur_mask = 0
    return {r: states[r][0] for r in reach}
