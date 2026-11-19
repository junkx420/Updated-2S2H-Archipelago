"""Time logic for the mm_2s2h apworld (Phase-0 candidate, moves into worlds/mm_2s2h in Phase 1).

2S2H 5.0.1 tracks a 45-bit "time slice" mask per region. For the 5.0.1 region graph that mask is the
same for every reachable region (see docs/time-logic.md):

* Clock Shuffle off: TIME_ALL_SLICES
* Clock Shuffle on:  the slices of the owned half-days (or Day 1 06:00 if no clock is owned)

so the time operators become pure functions of the collected clock items. No per-region propagation,
no LogicMixin cache, no extra cost for AP fill.
"""
from __future__ import annotations

from .enums_generated import TIME_SLICE  # noqa: F401  (re-exported for primitives)

TIME_SLICE_COUNT = 45
TIME_ALL_SLICES = (1 << TIME_SLICE_COUNT) - 1
# (first, last) slice of each half-day: D1, N1, D2, N2, D3, N3 (Logic.h HALF_DAY_TIME_RANGES)
HALF_DAY_TIME_RANGES = ((0, 6), (7, 15), (16, 22), (23, 30), (31, 36), (37, 44))
TIME_DAY1_AM_06_00 = 0
TIME_NIGHT1_PM_06_00 = 7
TIME_DAY2_AM_06_00 = 16
TIME_NIGHT2_PM_06_00 = 23
TIME_DAY3_AM_06_00 = 31
TIME_NIGHT3_PM_06_00 = 37

CLOCK_RANDOM, CLOCK_ASCENDING, CLOCK_DESCENDING = 0, 1, 2


def half_day_mask(i: int) -> int:
    a, b = HALF_DAY_TIME_RANGES[i]
    return ((1 << (b + 1)) - 1) & ~((1 << a) - 1)


class TimeContext:
    """Everything the time macros need. Built once per (clock flags) value and cached by the caller."""

    __slots__ = ("shuffle", "mode", "flags", "mask", "owns_mode")

    def __init__(self, clock_shuffle: bool, mode: int, clock_flags: int):
        self.shuffle = clock_shuffle
        self.mode = mode
        self.flags = clock_flags  # bit i = RANDO_INF_OBTAINED_CLOCK_DAY_1 + i
        self.mask = self._owned_time_slices()
        count = bin(clock_flags).count("1")
        if not clock_shuffle:
            self.owns_mode = (True,) * 6
        elif mode == CLOCK_RANDOM:
            self.owns_mode = tuple(bool(clock_flags >> i & 1) for i in range(6))
        elif mode == CLOCK_ASCENDING:
            self.owns_mode = tuple(count > i for i in range(6))
        elif mode == CLOCK_DESCENDING:
            self.owns_mode = tuple(count > 5 - i for i in range(6))
        else:
            self.owns_mode = (False,) * 6

    def _owned_time_slices(self) -> int:  # TimeLogic::GetOwnedTimeSlices
        if not self.shuffle:
            return TIME_ALL_SLICES
        m = 0
        for i in range(6):
            if self.flags >> i & 1:
                m |= half_day_mask(i)
        return m or (1 << TIME_DAY1_AM_06_00)

    # ---- Raw operators (Logic.h), evaluated against the region mask -------------------------
    def raw_at(self, s: int) -> bool:
        return bool(self.mask >> s & 1)

    def raw_before(self, s: int) -> bool:
        return s != 0 and bool(self.mask & ((1 << s) - 1))

    def raw_after(self, s: int) -> bool:
        return bool(self.mask & ~((1 << s) - 1) & TIME_ALL_SLICES)

    def raw_between(self, a: int, b: int) -> bool:
        return bool(self.mask & ((1 << b) - 1) & ~((1 << a) - 1))

    # ---- composite macros --------------------------------------------------------------------
    def is_day1(self): return self.raw_before(TIME_NIGHT1_PM_06_00) and self.owns_mode[0]
    def is_night1(self): return self.raw_between(TIME_NIGHT1_PM_06_00, TIME_DAY2_AM_06_00) and self.owns_mode[1]
    def is_day2(self): return self.raw_between(TIME_DAY2_AM_06_00, TIME_NIGHT2_PM_06_00) and self.owns_mode[2]
    def is_night2(self): return self.raw_between(TIME_NIGHT2_PM_06_00, TIME_DAY3_AM_06_00) and self.owns_mode[3]
    def is_day3(self): return self.raw_between(TIME_DAY3_AM_06_00, TIME_NIGHT3_PM_06_00) and self.owns_mode[4]
    def is_night3(self): return self.raw_after(TIME_NIGHT3_PM_06_00) and self.owns_mode[5]

    def clock_filter(self) -> bool:
        if not self.shuffle:
            return True
        return (self.is_day1() or self.is_night1() or self.is_day2() or self.is_night2()
                or self.is_day3() or self.is_night3())

    def is_day(self): return self.is_day1() or self.is_day2() or self.is_day3()
    def is_night(self): return self.is_night1() or self.is_night2() or self.is_night3()
    def first_day(self): return self.is_day1() or self.is_night1()
    def second_day(self): return self.is_day2() or self.is_night2()
    def final_day(self): return self.is_day3() or self.is_night3()

    def at(self, s): return self.raw_at(s) and self.clock_filter()
    def before(self, s): return self.raw_before(s) and self.clock_filter()
    def after(self, s): return self.raw_after(s) and self.clock_filter()
    def between(self, a, b): return self.raw_between(a, b) and self.clock_filter()

    def clock(self, i: int) -> bool:  # CLOCK_DAY1() .. CLOCK_NIGHT3()
        return self.owns_mode[i]

    def extra_check_available(self, half_days: int, layout: str, *,
                              snowhead_cleared: bool, great_bay_cleared: bool) -> bool:
        """Whether a placement exists in an available half-day of the specified scene layout."""
        if layout == "SNOWHEAD_SPRING" and not snowhead_cleared:
            return False
        if layout == "GREAT_BAY_BASE" and great_bay_cleared:
            return False
        return half_days == 0x3F or any(
            half_days & (1 << i) and owns and self.mask & half_day_mask(i)
            for i, owns in enumerate(self.owns_mode)
        )
