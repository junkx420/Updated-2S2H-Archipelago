"""Extract the time-logic code verbatim from a 2S2H checkout into .inc files for harness.cpp.

Usage: python extract.py <path-to-2s2h-checkout> <out-dir>

Cuts are made by text markers, not line numbers, so this keeps working across small upstream edits.
If a marker is missing the script fails loudly: that means upstream changed the time logic and the
spike has to be re-checked.
"""
import pathlib
import sys


def cut(text: str, start: str, end: str, include_end: bool = False) -> str:
    i = text.index(start)
    j = text.index(end, i)
    if include_end:
        j = text.index("\n", j) + 1
    return text[i:j]


def main() -> None:
    root = pathlib.Path(sys.argv[1]) / "mm" / "2s2h" / "Rando" / "Logic"
    out = pathlib.Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)

    logic_h = (root / "Logic.h").read_text(encoding="utf-8")
    time_cpp = (root / "TimeLogic.cpp").read_text(encoding="utf-8")
    logic_cpp = (root / "Logic.cpp").read_text(encoding="utf-8")

    parts = {
        "types.inc": cut(logic_h, "enum TimeSlice {", "extern uint64_t gCurrentRegionTime;"),
        "setcur.inc": cut(logic_h, "inline void SetCurrentRegionTime(", "void FindReachableRegions("),
        "helpers.inc": cut(logic_h, "inline uint32_t ClockCount()", "#define BETWEEN(s, e)", include_end=True),
        "timelogic.inc": cut(time_cpp, "uint64_t ExpandTimeForward(", "// Validation helper for clock ownership"),
        "propagation.inc": cut(logic_cpp, "RegionTimeState InitialTimeState()", "// clang-format off"),
    }
    for name, body in parts.items():
        (out / name).write_text(f"// AUTO-EXTRACTED from 2S2H, do not edit\n{body}\n", encoding="utf-8")
        print(f"{name}: {body.count(chr(10))} lines")


if __name__ == "__main__":
    main()
