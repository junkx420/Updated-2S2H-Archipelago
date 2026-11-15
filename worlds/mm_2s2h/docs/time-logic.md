# Time logic: 2S2H 5.0.1 → Archipelago (Phase 0 spike)

## How 2S2H does it

* 45 time slices (`TimeSlice`, Day 1 06:00 … Night 3 05:00) as a bitmask per region (`RegionTimeState`).
* Start: `RR_MAX` gets Day 1 06:00 (Clock Shuffle off) or all owned half-days (Clock Shuffle on), `canStayOverTime=false`.
* `FindReachableRegions` (Logic.cpp): on entering a region its mask is expanded forward
  (`ExpandTimeForward`). Every passable connection/exit then hands the **whole source mask** to the target.
  The connection's own time condition only decides *whether* it is passable and does not narrow the mask.
  New bits trigger re-exploration.
* `ExpandTimeForward` (TimeLogic.cpp): with no `STAY` restriction and no Clock Shuffle it fills every slice
  ≥ the earliest one. Otherwise it walks forward and stops at a failing `STAY(slice, cond)` or at an unowned half-day.
* With Clock Shuffle, picking up a clock resets **every** known region to the owned slices (GlitchlessLogic.cpp:137).
* Operators (`AT/BEFORE/AFTER/BETWEEN`, `IS_DAY1()` …) test `gCurrentRegionTime` and the clocks owned in the
  current mode (`OwnsHalfDayForMode`: random, ascending or descending).

## Finding: in 5.0.1 the mask is uniform

For the 5.0.1 region graph the fixpoint gives **the same mask for every reachable region**:

| Clock Shuffle | mask of every reachable region |
|---|---|
| off | `TIME_ALL_SLICES` |
| on  | slices of the owned half-days (`GetOwnedTimeSlices`, or Day 1 06:00 if none is owned) |

Why: no region sets `canStayOverTime=false`. Every `RR_MAX` exit target (South Clock Town and the owl statues)
has no `STAY` and expands to ALL at once. Because masks are handed on unnarrowed, everything downstream gets ALL,
and `STAY` cannot remove bits that are already there. With Clock Shuffle, the start mask and every reset are
already "owned", and expansion never leaves owned time. `RR_MAX` itself ends at ALL via
`RR_CLOCK_TOWN_SOUTH → RR_MAX`. Its conditions use no time operators, so the Day 1 06:00 start value never
matters.

**Consequence for AP:** the time operators are pure functions of the collected clock items. The apworld therefore
needs no per-region propagation and no LogicMixin. Each rule computes `TimeContext(shuffle, mode, clock_flags)`,
which is cheap and cacheable per clock-flag value (64 values), and calls `ctx.between(a, b)` and so on, exactly
as the C++ macros do. Fill costs nothing extra. Clock Shuffle (random, ascending, descending) is supported from
the start. The fallback "drop Clock Shuffle" is **not needed**.

This deliberately mirrors 2S2H, including its looseness. Example: West Clock Town → Curiosity Shop requires
night, but the shop still gets the full mask. Parity with 2S2H matters more than tightening it.

## Guard (the generator must enforce it)

The closed form only holds while these hold. `tools/gen_from_2s2h.py` must assert them and fail if one breaks:

1. No region sets `canStayOverTime` (currently 0).
2. No `RR_MAX` exit target has `timeStayRestrictions`.
3. No condition in `RR_MAX` (checks, exits, connections) uses a time operator.
4. `STAY` conditions use no time operators (currently only `false` or `Flags_GetRandoInf(...)`).

If a future release breaks one of them, switch to `tools/time_spike/time_reference.py`: a faithful port of the
propagation, cached per state in a LogicMixin and invalidated on collect/remove. Measured on a synthetic graph
the size of 5.0.1 (314 regions, ~700 edges): **1.9 ms** per recompute without Clock Shuffle and **6.4 ms** with
it, in pure Python. That is usable, but noticeably slower in fill.

## Verification (`tools/time_spike/`)

* `extract.py` cuts the real code (time types, clock and operator helpers, `ExpandTimeForward`,
  `GetOwnedTimeSlices`, `InitializeRegionTimeStates`, `FindReachableRegions`) **verbatim** out of the 2S2H
  checkout. `harness.cpp` compiles it with MSVC against minimal mocks and runs the GlitchlessLogic outer loop on
  scenario files.
* `compare.py` checks 4772 scenarios. 772 use a real Clock Town subgraph (SCT, ECT, WCT, Stock Pot Inn with its
  room-key `STAY`, Milk Bar, Curiosity Shop front and back, Laundry Pool, with the real conditions from
  Central.cpp) across every Clock Shuffle mode × 64 clock sets × item sets. The other 4000 are random graphs with
  `STAY`s and time conditions.
  * Python reference port vs C++: **4772/4772 identical** (masks and check results).
  * Closed form vs C++: **identical in every scenario where the guard holds**. Where the guard is broken on
    purpose, it differs in 462 cases, which shows the guard is necessary and that the test can tell them apart.

Reproduce:

```
cd worlds/mm_2s2h/tools/time_spike
python extract.py <2s2h-checkout> build
build.bat
python compare.py build\harness.exe 4000
```

## Open for Phase 1

* `RANDO_EVENTS[RE_ACCESS_ZORA_EGG] >= 7`: events are **counted** (one per `EVENT` occurrence). Model every
  occurrence in AP as its own event location and use `state.count(...)`.
* Starting clock under Clock Shuffle random (Menu.cpp:346, "computed given"): find out how 2S2H hands out the
  first clock and mirror it in AP (start inventory or precollected).
* `RI_TIME_PROGRESSIVE` resolves in game in mode order (ConvertItem.cpp:663). In AP: N progressive clocks equal
  the first N half-days of that order.
