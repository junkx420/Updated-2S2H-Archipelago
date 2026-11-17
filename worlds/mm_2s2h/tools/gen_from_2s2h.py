"""Generate the data/logic modules of the mm_2s2h apworld from a 2 Ship 2 Harkinian checkout.

Usage:  python gen_from_2s2h.py <path-to-2s2h-checkout>

Reads (never compiles) the 2S2H sources:
  mm/2s2h/Rando/Types.h                 enums -> IDs and constants
  mm/2s2h/Rando/StaticData/Checks.cpp   check type / scene / vanilla item
  mm/2s2h/Rando/StaticData/Items.cpp    item names / types
  mm/2s2h/Rando/Logic/Logic.h           #define macros + CanKillEnemy switch
  mm/2s2h/Rando/Logic/Regions/*.cpp     region DSL
  mm/2s2h/Rando/ActorBehavior/Souls.cpp enemy -> soul map
and writes next to this script's parent:
  enums_generated.py, items_generated.py, locations_generated.py,
  macros_generated.py, regions_generated.py, UNTRANSLATED.md

Anything the translator does not understand is listed in UNTRANSLATED.md (file:line) and compiled to a
call of h.UNTRANSLATED(<id>), which raises at generation time unless explicitly allowed. Nothing is dropped
silently.
"""
from __future__ import annotations

import pathlib
import re
import sys
from collections import defaultdict

HERE = pathlib.Path(__file__).resolve().parent
OUT = HERE.parent

# Primitives are hand-written in primitives.py. The generator must not translate these from Logic.h
# (they read raw save data, use ## token pasting, or are time macros handled by TimeContext).
PRIMITIVES_CALL = {
    "HAS_ITEM", "CHECK_QUEST_ITEM", "CHECK_DUNGEON_ITEM", "CHECK_WEEKEVENTREG", "Flags_GetRandoInf",
    "GET_CUR_EQUIP_VALUE", "CUR_UPG_VALUE", "GET_CUR_UPG_VALUE", "CHECK_MAX_HP", "HAS_BOTTLE_ITEM",
    "CAN_PLAY_SONG", "canPlaySong", "CAN_ACCESS", "CAN_USE_MAGIC_ARROW", "CAN_USE_ABILITY", "KEY_COUNT",
    "CAN_AFFORD", "CAN_OWL_WARP", "HAS_ENOUGH_STRAY_FAIRIES", "HAS_ENOUGH_SKULLTULA_TOKENS",
    "CanAccessDungeon", "MoonMaskCount", "RemainsCount", "MeetsMoonRequirements", "CanKillEnemy",
    "HaveEnemySoul", "FoundOcarinaButtons", "ClockCount", "Flags_GetSceneSwitch",
    "CanGetPastBigOcto", "CanGetPastBigOctoWithoutBoat",
    # time (TimeContext)
    "AT", "BEFORE", "AFTER", "BETWEEN", "IS_DAY1", "IS_NIGHT1", "IS_DAY2", "IS_NIGHT2", "IS_DAY3", "IS_NIGHT3",
    "IS_DAY", "IS_NIGHT", "FIRST_DAY", "SECOND_DAY", "FINAL_DAY", "CLOCK_DAY1", "CLOCK_NIGHT1", "CLOCK_DAY2",
    "CLOCK_NIGHT2", "CLOCK_DAY3", "CLOCK_NIGHT3", "MIDNIGHT",
}
PRIMITIVES_BARE = {"IS_DEKU", "IS_ZORA", "IS_DEITY", "IS_GORON", "IS_HUMAN", "HAS_MAGIC", "HAS_BOTTLE"}
DSL_MACROS = {"EVENT", "EXIT", "CONNECTION", "CHECK", "STAY", "ONE_WAY_EXIT", "SET_OWL_WARP", "CLEAR_OWL_WARP"}
# constants from game headers outside Types.h that appear in logic expressions
EXTRA_CONSTS = {
    "EQUIP_VALUE_SWORD_NONE": 0, "EQUIP_VALUE_SWORD_KOKIRI": 1, "EQUIP_VALUE_SWORD_RAZOR": 2,
    "EQUIP_VALUE_SWORD_GILDED": 3, "EQUIP_VALUE_SWORD_DEITY": 4,
    "EQUIP_VALUE_SHIELD_NONE": 0, "EQUIP_VALUE_SHIELD_HERO": 1, "EQUIP_VALUE_SHIELD_MIRROR": 2,
}


# ------------------------------------------------------------------------------------------------
# source helpers
# ------------------------------------------------------------------------------------------------
def strip_comments(src: str) -> str:
    """Replace comments by spaces, keeping newlines and offsets."""
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if c == '"':
            j = i + 1
            while j < n and src[j] != '"':
                j += 2 if src[j] == "\\" else 1
            out.append(src[i:j + 1])
            i = j + 1
        elif src.startswith("//", i):
            j = src.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif src.startswith("/*", i):
            j = src.find("*/", i + 2) + 2
            out.append(re.sub(r"[^\n]", " ", src[i:j]))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def match_brace(s: str, i: int, open_c: str = "{", close_c: str = "}") -> int:
    """s[i] == open_c; returns index of matching close."""
    depth = 0
    in_str = False
    for j in range(i, len(s)):
        c = s[j]
        if in_str:
            if c == "\\":
                continue
            if c == '"':
                in_str = False
            continue
        if c == '"':
            in_str = True
        elif c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced")


def split_top(s: str, sep: str = ",") -> list[tuple[int, str]]:
    """Split at top-level separators; returns (offset, piece) with offsets relative to s."""
    parts, depth, start, in_str = [], 0, 0, False
    for j, c in enumerate(s):
        if in_str:
            if c == '"':
                in_str = False
            continue
        if c == '"':
            in_str = True
        elif c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        elif c == sep and depth == 0:
            parts.append((start, s[start:j]))
            start = j + 1
    parts.append((start, s[start:]))
    return [(o + (len(p) - len(p.lstrip())), p.strip()) for o, p in parts if p.strip()]


def parse_enums(types_h: str) -> dict[str, dict[str, int]]:
    enums = {}
    for m in re.finditer(r"typedef enum\s*\{(.*?)\}\s*(\w+)\s*;", types_h, re.S):
        body, name = m.group(1), m.group(2)
        vals, cur = {}, -1
        for entry in body.split(","):
            entry = entry.strip()
            if not entry:
                continue
            if "=" in entry:
                k, v = (x.strip() for x in entry.split("=", 1))
                cur = int(v, 0) if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", v) else vals[v]
            else:
                k = entry
                cur += 1
            vals[k] = cur
        enums[name] = vals
    return enums


# ------------------------------------------------------------------------------------------------
# expression translator
# ------------------------------------------------------------------------------------------------
TOK = re.compile(r"\s*(?:(0[xX][0-9a-fA-F]+|\d+)|([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)|(&&|\|\||==|!=|>=|<=|[!<>()\[\],+\-*/]))")
BIN = {"||": 1, "&&": 2, "==": 5, "!=": 5, "<": 6, ">": 6, "<=": 6, ">=": 6, "+": 8, "-": 8, "*": 9, "/": 9}


class ParseError(Exception):
    pass


def tokenize(s: str):
    toks, i = [], 0
    s = s.rstrip()
    while i < len(s):
        m = TOK.match(s, i)
        if not m or m.end() == i:
            raise ParseError(f"bad token at {s[i:i + 20]!r}")
        if m.group(1):
            toks.append(("num", int(m.group(1), 0)))
        elif m.group(2):
            toks.append(("id", m.group(2).split("::")[-1]))
        elif m.group(3):
            toks.append(("op", m.group(3)))
        i = m.end()
    return toks


class Parser:
    def __init__(self, toks):
        self.t, self.i = toks, 0

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else ("eof", None)

    def take(self, op=None):
        tok = self.peek()
        if op is not None and tok != ("op", op):
            raise ParseError(f"expected {op}, got {tok}")
        self.i += 1
        return tok

    def expr(self, minp=0):
        left = self.unary()
        while True:
            k, v = self.peek()
            if k != "op" or v not in BIN or BIN[v] < minp:
                return left
            self.take()
            left = ("bin", v, left, self.expr(BIN[v] + 1))

    def unary(self):
        k, v = self.peek()
        if k == "op" and v in ("!", "-"):
            self.take()
            return ("un", v, self.unary())
        return self.postfix(self.atom())

    def atom(self):
        k, v = self.take()
        if k == "num":
            return ("num", v)
        if k == "id":
            return ("id", v)
        if (k, v) == ("op", "("):
            e = self.expr()
            self.take(")")
            return e
        raise ParseError(f"unexpected {k} {v}")

    def postfix(self, node):
        while True:
            k, v = self.peek()
            if (k, v) == ("op", "(") and node[0] == "id":
                self.take()
                args = []
                if self.peek() != ("op", ")"):
                    args.append(self.expr())
                    while self.peek() == ("op", ","):
                        self.take()
                        args.append(self.expr())
                self.take(")")
                node = ("call", node[1], args)
            elif (k, v) == ("op", "[") and node[0] == "id":
                self.take()
                idx = self.expr()
                self.take("]")
                node = ("index", node[1], idx)
            else:
                return node


def parse_expr(s: str):
    p = Parser(tokenize(s))
    e = p.expr()
    if p.peek()[0] != "eof":
        raise ParseError(f"trailing tokens {p.t[p.i:p.i + 3]}")
    return e


class Translator:
    def __init__(self, consts: dict[str, int], macros0: set[str], macrosN: set[str], prefix: str):
        self.consts, self.m0, self.mN, self.p = consts, macros0 | PRIMITIVES_BARE, macrosN | PRIMITIVES_CALL, prefix
        self.unknown: list[str] = []
        self.params: set[str] = set()
        self.usage: dict[str, set] = defaultdict(set)

    def arg(self, node) -> str:
        if node[0] == "id" and node[1] not in self.m0 and node[1] not in self.params and node[1] not in ("true", "false"):
            return repr(node[1])
        return self.emit(node)

    def emit(self, node) -> str:
        kind = node[0]
        if kind == "num":
            return str(node[1])
        if kind == "id":
            name = node[1]
            if name == "true":
                return "True"
            if name == "false":
                return "False"
            if name in self.params:
                return name
            if name in self.m0:
                self.usage[name].add(())
                return f"{self.p}{name}(s)"
            if name in self.consts:
                return str(self.consts[name])
            self.unknown.append(f"identifier `{name}`")
            return f"{self.p}UNTRANSLATED({name!r})"
        if kind == "call":
            name, args = node[1], node[2]
            if name in self.mN:
                self.usage[name].add(tuple(a[1] if a[0] in ("id", "num") else "<expr>" for a in args))
                return f"{self.p}{name}(s{''.join(', ' + self.arg(a) for a in args)})"
            self.unknown.append(f"call `{name}(...)`")
            return f"{self.p}UNTRANSLATED({name!r})"
        if kind == "index":
            name, idx = node[1], node[2]
            if name == "RANDO_EVENTS" and idx[0] == "id":
                return f"s.ev({idx[1]!r})"
            if name == "RANDO_SAVE_OPTIONS" and idx[0] == "id":
                self.usage["RANDO_SAVE_OPTIONS"].add((idx[1],))
                return f"s.opt({idx[1]!r})"
            self.unknown.append(f"index `{name}[...]`")
            return f"{self.p}UNTRANSLATED({name!r})"
        if kind == "un":
            e = self.emit(node[2])
            return f"(not {e})" if node[1] == "!" else f"(-{e})"
        if kind == "bin":
            op = {"&&": "and", "||": "or"}.get(node[1], node[1])
            return f"({self.emit(node[2])} {op} {self.emit(node[3])})"
        raise AssertionError(node)

    def translate(self, src: str) -> tuple[str, list[str]]:
        self.unknown = []
        try:
            code = self.emit(parse_expr(src))
        except ParseError as e:
            return f"{self.p}UNTRANSLATED('parse')", [f"parse error: {e}"]
        return code, list(self.unknown)


# ------------------------------------------------------------------------------------------------
# Logic.h #defines and CanKillEnemy
# ------------------------------------------------------------------------------------------------
DEFINE = re.compile(r"^[ \t]*#define[ \t]+(\w+)(\(([^)]*)\))?[ \t]*((?:[^\n]*\\\n)*[^\n]*)", re.M)


def parse_defines(src: str):
    out = []
    for m in DEFINE.finditer(src):
        name, params, body = m.group(1), m.group(3), m.group(4).replace("\\\n", " ").strip()
        line = src.count("\n", 0, m.start()) + 1
        out.append((name, [p.strip() for p in params.split(",")] if params is not None else None, body, line))
    return out


def parse_switch_returns(src: str, func: str):
    """case A: case B: return expr;  ->  [([A, B], expr, line)]"""
    i = src.index(func)
    j = src.index("switch", i)
    k = src.index("{", j)
    body = src[k:match_brace(src, k) + 1]
    base = src.count("\n", 0, k)
    out, pending = [], []
    for m in re.finditer(r"case\s+(\w+)\s*:|default\s*:|return\s+(.*?);", body, re.S):
        if m.group(1):
            pending.append(m.group(1))
        elif m.group(0).startswith("default"):
            pending = []
        elif pending:
            out.append((pending, m.group(2), base + body.count("\n", 0, m.start()) + 1))
            pending = []
    return out


# ------------------------------------------------------------------------------------------------
# region DSL
# ------------------------------------------------------------------------------------------------
def entrance_key(s: str) -> str:
    m = re.fullmatch(r"ENTRANCE\(\s*(\w+)\s*,\s*(\d+)\s*\)", s.strip())
    if not m:
        raise ParseError(f"bad entrance {s!r}")
    return f"{m.group(1)}:{m.group(2)}"


def macro_args(item: str, name: str) -> list[tuple[int, str]]:
    m = re.match(rf"{name}\s*\(", item)
    if not m:
        raise ParseError(f"expected {name}(...): {item[:60]!r}")
    a = m.end() - 1
    b = match_brace(item, a, "(", ")")
    return [(a + 1 + o, p) for o, p in split_top(item[a + 1:b])]


def parse_regions(path: pathlib.Path):
    raw = path.read_text(encoding="utf-8")
    src = strip_comments(raw)
    regions, local_defines = [], []
    for name, params, body, line in parse_defines(src):
        local_defines.append((name, params, body, f"{path.name}:{line}"))
    local_funcs = re.findall(r"^inline\s+bool\s+(\w+)\s*\(", src, re.M)
    lineof = lambda off: src.count("\n", 0, off) + 1  # noqa: E731
    for m in re.finditer(r"Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{", src):
        start = m.end() - 1
        end = match_brace(src, start)
        body = src[start + 1:end]
        reg = {"id": m.group(1), "name": "", "scene": "", "checks": [], "connections": [], "exits": [],
               "events": [], "stay": [], "one_way": [], "can_stay": None, "loc": f"{path.name}:{lineof(m.start())}"}
        for off, field in split_top(body):
            fm = re.match(r"\.(\w+)\s*=\s*(.*)$", field, re.S)
            if not fm:
                raise ParseError(f"{path.name}:{lineof(start + 1 + off)} bad field {field[:40]!r}")
            key, val = fm.group(1), fm.group(2).strip()
            voff = start + 1 + off + fm.start(2)
            if key == "name":
                reg["name"] = val.strip('"')
            elif key == "sceneId":
                reg["scene"] = val
            elif key == "canStayOverTime":
                reg["can_stay"] = val
            elif key in ("checks", "connections", "exits", "events", "timeStayRestrictions", "oneWayEntrances"):
                inner = val[1:val.rindex("}")]
                ioff = voff + 1
                for o, item in split_top(inner):
                    at = ioff + o
                    loc = f"{path.name}:{lineof(at)}"
                    if key == "oneWayEntrances":
                        reg["one_way"].append(entrance_key(item))
                        continue
                    macro = {"checks": "CHECK", "connections": "CONNECTION", "exits": "EXIT", "events": "EVENT",
                             "timeStayRestrictions": "STAY"}[key]
                    args = [a for _, a in macro_args(item, macro)]
                    if macro == "EXIT":
                        to, frm, cond = args[0], args[1], ",".join(args[2:])
                        reg["exits"].append((entrance_key(to), None if frm == "ONE_WAY_EXIT" else entrance_key(frm), cond, loc))
                    else:
                        tgt, cond = args[0], ",".join(args[1:])
                        reg[{"CHECK": "checks", "CONNECTION": "connections", "EVENT": "events", "STAY": "stay"}[macro]].append((tgt, cond, loc))
            else:
                raise ParseError(f"{path.name}:{lineof(voff)} unknown field .{key}")
        regions.append(reg)
    return regions, local_defines, local_funcs


# ------------------------------------------------------------------------------------------------
# static data
# ------------------------------------------------------------------------------------------------
def parse_rc_table(src: str):
    out = {}
    for m in re.finditer(r"^\s*RC\((RC_\w+),\s*(RCTYPE_\w+),\s*(SCENE_\w+),\s*(\w+),\s*([\w]+),\s*(RI_\w+)\)", src, re.M):
        flag = int(m.group(5), 0) if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", m.group(5)) else m.group(5)
        out[m.group(1)] = {"type": m.group(2), "scene": m.group(3), "vanilla": m.group(6), "flag": flag}
    return out



def check_grottos(rows: list[dict], grotto_of: dict, rc_table: dict, out_regions: list, rando) -> None:
    """A grotto is the pair (real entrance, grotto data). Where grottos share a real entrance, prove each pair from
    the client's own lookups: chest flag = data & 0x1F (z_en_torch.c), ObjGrass.cpp chestGrottoMap/cowGrottoMap."""
    if set(grotto_of) != {r["into"] for r in rows if r["grotto"]}:
        raise SystemExit("grottoEntrances: rows without a shuffleEntrances grotto row")
    checks_of = {rid: {c for c, _ in checks} for rid, _, _, checks, *_ in out_regions}
    grass = strip_comments((rando / "ActorBehavior" / "ObjGrass.cpp").read_text(encoding="utf-8"))
    maps = {}
    for name in ("chestGrottoMap", "cowGrottoMap"):
        body = re.search(name + r"\s*=\s*\{(.*?)\n\};", grass, re.S).group(1)
        maps[name] = {int(k) & 0xFF: rc for k, rc in re.findall(r"\{\s*(-?\d+)\s*,\s*(?:\{\s*\d+\s*,\s*)?(RC_\w+)", body)}
    by_game = defaultdict(list)
    for r in rows:
        if r["grotto"]:
            by_game[r["grotto"]["game_entrance"]].append(r)
    for game, shared in by_game.items():
        if len(shared) == 1:
            continue
        datas = [r["grotto"]["data"] for r in shared]
        if None in datas or len(set(datas)) != len(datas):
            raise SystemExit(f"grottoEntrances: grottos sharing {game} need distinct data, have {datas}")
        lookup = {"GROTTOS:4": "chestGrottoMap", "GROTTOS:10": "cowGrottoMap"}.get(game)
        if lookup is None:
            raise SystemExit(f"grottoEntrances: no client lookup known for the shared entrance {game}")
        for r in shared:
            data, region = r["grotto"]["data"], r["inside_region"]
            if maps[lookup].get(data) not in checks_of[region]:
                raise SystemExit(f"grottoEntrances {r['into']}: {lookup}[{data:#x}] = {maps[lookup].get(data)} "
                                 f"is not in {region}")
            if lookup == "chestGrottoMap":
                chests = [c for c in checks_of[region] if rc_table.get(c, {}).get("type") == "RCTYPE_CHEST"]
                if len(chests) != 1 or rc_table[chests[0]]["flag"] != data & 0x1F:
                    raise SystemExit(f"grottoEntrances {r['into']}: chest flag of {chests} is not {data & 0x1F:#x}")


def check_interiors(rows: list[dict], hours_of: dict, out_regions: list) -> None:
    """Interiors are dead ends behind knob doors: every way out of an interior scene to another scene is the way back
    of one of its interior rows, and no interior door stands inside another interior (no nesting)."""
    interiors = [r for r in rows if r["hours"]]
    if set(hours_of) != {r["into"] for r in interiors}:
        raise SystemExit("interiorEntrances: rows without a shuffleEntrances interior row")
    scene_of = {rid: scene for rid, _, scene, *_ in out_regions}
    inside_scenes = {scene_of[r["inside_region"]] for r in interiors}
    outs = {r["out"] for r in interiors}
    for r in interiors:
        if scene_of[r["door_region"]] in inside_scenes:
            raise SystemExit(f"interiorEntrances: {r['into']} is a door inside another interior")
    for rid, _, scene, _, _, exits, _, _ in out_regions:
        if scene not in inside_scenes:
            continue
        for to, tgt, _ in exits:
            if scene_of[tgt] != scene and to not in outs:
                raise SystemExit(f"interiorEntrances: {rid} in {scene} leaves through {to}, no interior's way back")
    for r in rows:
        if not r["hours"] and scene_of[r["inside_region"]] in inside_scenes:
            raise SystemExit(f"shuffleEntrances: {r['into']} leads into an interior scene but is no interior row")


# Scene variants the game picks in Play_Init (z_play.c:2264-2299) or by inversion; an overworld row names the place,
# so its entrances and scenes must be the base ones (Play_GetOriginalSceneId, z_play.c:1975).
VARIANT_ENTRANCES = ("MOUNTAIN_VILLAGE_SPRING", "GORON_VILLAGE_SPRING", "PATH_TO_GORON_VILLAGE_SPRING",
                     "SOUTHERN_SWAMP_CLEARED", "STONE_TOWER_INVERTED")
VARIANT_SCENES = ("SCENE_20SICHITAI2", "SCENE_10YUKIYAMANOMURA2", "SCENE_11GORONNOSATO2", "SCENE_17SETUGEN2",
                  "SCENE_F41", "SCENE_INISIE_R")


def check_overworld(rows: list[dict], es_rows: list, out_regions: list) -> None:
    """Overworld rows are single exits, two per transition: row i and the row whose "into" is i's "out" are each other's
    way back. Every row names a place (no scene variant), leaves its scene for another, and shares no entrance with a
    row of another option, so the client's redirects never overlap."""
    over = [r for r in rows if r["option"] == "RO_SHUFFLE_OVERWORLD"]
    if not over:
        return
    by_into = {r["into"]: r for r in over}
    if len(by_into) != len(over):
        raise SystemExit("shuffleEntrances: an overworld entrance has two rows")
    scenes = {into: (ins, outs) for opt, into, _, ins, outs, *_ in es_rows if opt == "RO_SHUFFLE_OVERWORLD"}
    # A grotto hole's way back is the hole it stored (respawn UNK_3, z_player.c:6496), not its logic "out".
    others = {e for r in rows if r["option"] != "RO_SHUFFLE_OVERWORLD"
              for e in (r["into"], r["out"]) if not (e == r["out"] and (r["grotto"] or {}).get("kind") == "HOLE")}
    scene_of = {rid: scene for rid, _, scene, *_ in out_regions}
    for r in over:
        back = by_into.get(r["out"])
        if back is None or back["out"] != r["into"] or back["door_region"] != r["inside_region"] \
                or back["inside_region"] != r["door_region"]:
            raise SystemExit(f"shuffleEntrances: overworld {r['into']} has no opposite row {r['out']}")
        inside, outside = scenes[r["into"]]
        if inside == outside or r["into"].split(":")[0] in VARIANT_ENTRANCES or {inside, outside} & set(VARIANT_SCENES):
            raise SystemExit(f"shuffleEntrances: overworld {r['into']} must name a place in another scene, "
                             f"not a variant ({outside} -> {inside})")
        if scene_of[r["inside_region"]] != inside or scene_of[r["door_region"]] != outside:
            raise SystemExit(f"shuffleEntrances: overworld {r['into']} regions are not in {outside} -> {inside}")
        if r["into"] in others or r["out"] in others:
            raise SystemExit(f"shuffleEntrances: overworld {r['into']} shares an entrance with another option's row")
        if r["boss_warps"] or r["temple_exit"] is not None:
            raise SystemExit(f"shuffleEntrances: overworld {r['into']} cannot have boss warps")


def parse_ri_table(src: str):
    out = {}
    for m in re.finditer(r'^\s*RI\((RI_\w+),\s*"([^"]*)",\s*"([^"]*)",\s*(RITYPE_\w+),\s*(\w+),', src, re.M):
        out[m.group(1)] = {"name": m.group(3), "type": m.group(4), "item": m.group(5)}
    return out


def readable(rc: str) -> str:
    words = rc[3:].split("_") if rc.startswith("RC_") else rc.split("_")
    return " ".join("HP" if w.lower() == "hp" else w.lower().capitalize() for w in words)


# ------------------------------------------------------------------------------------------------
def main() -> int:
    root = pathlib.Path(sys.argv[1]).resolve()
    rando = root / "mm" / "2s2h" / "Rando"
    types_h = strip_comments((rando / "Types.h").read_text(encoding="utf-8"))
    enums = parse_enums(types_h)
    E = lambda n: enums[n]  # noqa: E731
    consts = {}
    for vals in enums.values():
        consts.update(vals)
    consts.update(EXTRA_CONSTS)
    untranslated: list[tuple[str, str, str]] = []  # (where, what, source)
    justified: list[tuple[str, str, str]] = []

    # ---- static data ----
    rc_table = parse_rc_table(strip_comments((rando / "StaticData" / "Checks.cpp").read_text(encoding="utf-8")))
    ri_table = parse_ri_table(strip_comments((rando / "StaticData" / "Items.cpp").read_text(encoding="utf-8")))
    souls_src = strip_comments((rando / "ActorBehavior" / "Souls.cpp").read_text(encoding="utf-8"))
    souls = dict(re.findall(r"\{\s*(ACTOR_\w+)\s*,\s*(RI_SOUL_\w+)\s*\}", souls_src))
    # NPC souls: { RI_SOUL_NPC_X, { actors }, { checks }, { events } } (Souls.cpp npcSouls)
    names_in = lambda s: [x.strip() for x in s.split(",") if x.strip()]  # noqa: E731
    npc_souls = {ri: (names_in(actors), names_in(checks), names_in(events)) for ri, actors, checks, events in re.findall(
        r"\{\s*(RI_SOUL_NPC_\w+)\s*,\s*\{([^{}]*)\}\s*,\s*\{([^{}]*)\}\s*,\s*\{([^{}]*)\}\s*,?\s*\}", souls_src)}
    npc_enum = [ri for ri in E("RandoItemId") if ri.startswith("RI_SOUL_NPC_")]
    if sorted(npc_souls) != sorted(npc_enum):
        raise SystemExit(f"npcSouls table and RI_SOUL_NPC_* enum differ: {set(npc_souls) ^ set(npc_enum)}")
    for ri, (_, checks, events) in npc_souls.items():
        unknown = [c for c in checks if c not in E("RandoCheckId")] + [e for e in events if e not in E("RandoEvent")]
        if unknown or f"RANDO_INF_OBTAINED_SOUL_OF_NPC_{ri[len('RI_SOUL_NPC_'):]}" not in E("RandoInf"):
            raise SystemExit(f"npcSouls {ri}: unknown checks/events {unknown} or missing RANDO_INF")

    # Entrance shuffle: { RO_X, ENTRANCE(into), ENTRANCE(outOf), inside scene, outside scene, boss scene,
    # { boss warp targets } } (MiscBehavior/EntranceShuffle.cpp shuffleEntrances); checked against the regions below
    es_src = strip_comments((rando / "MiscBehavior" / "EntranceShuffle.cpp").read_text(encoding="utf-8"))
    es_body = re.search(r"shuffleEntrances\s*=\s*\{(.*?)\n\};", es_src, re.S).group(1)
    ent = r"ENTRANCE\(\s*\w+\s*,\s*\d+\s*\)"
    es_rows = [(opt, entrance_key(a), entrance_key(b), ins, outs, boss, [entrance_key(w) for w in re.findall(ent, warps)],
                entrance_key(temple) if temple else None)
               for opt, a, b, ins, outs, boss, warps, temple in re.findall(
                   rf"\{{\s*(RO_\w+)\s*,\s*({ent})\s*,\s*({ent})\s*,\s*(SCENE_\w+)\s*,\s*(SCENE_\w+)\s*,\s*(SCENE_\w+)\s*,"
                   rf"\s*\{{([^{{}}]*)\}}\s*(?:,\s*({ent}))?\s*\}}", es_body)]
    if len(es_rows) != es_body.count("RO_") or not es_rows:
        raise SystemExit("shuffleEntrances: a row does not match the expected format")
    entr_scene = parse_enums(strip_comments((root / "mm" / "include" / "z64scene.h").read_text(encoding="utf-8")))["EntranceSceneId"]
    # The save keeps one u16 per row (z64save.h RandoSaveInfo::entrancePairs); rows past its end could not be stored.
    save_h = strip_comments((root / "mm" / "include" / "z64save.h").read_text(encoding="utf-8"))
    pair_slots = int(re.search(r"\bu16\s+entrancePairs\[(\d+)\]", save_h).group(1))
    if len(es_rows) > pair_slots:
        raise SystemExit(f"shuffleEntrances: {len(es_rows)} rows do not fit entrancePairs[{pair_slots}]")
    # Grottos: { ENTRANCE(into), ENTRANCE(game entrance), data, GROTTO_KIND } (EntranceShuffle.cpp grottoEntrances)
    ge_body = re.search(r"grottoEntrances\s*=\s*\{(.*?)\n\};", es_src, re.S).group(1)
    ge_rows = re.findall(rf"\{{\s*({ent})\s*,\s*({ent})\s*,\s*(-1|0x[0-9A-Fa-f]+)\s*,\s*(GROTTO_HOLE|GROTTO_SCENE_EXIT)\s*\}}",
                         ge_body)
    if len(ge_rows) != ge_body.count("GROTTO_") or not ge_rows:
        raise SystemExit("grottoEntrances: a row does not match the expected format")
    grotto_of = {}
    for a, game, data, kind in ge_rows:
        if entrance_key(a) in grotto_of:
            raise SystemExit(f"grottoEntrances: {entrance_key(a)} twice")
        grotto_of[entrance_key(a)] = {"game_entrance": entrance_key(game), "data": None if data == "-1" else int(data, 16),
                                      "kind": kind[len("GROTTO_"):]}

    # Interiors: { ENTRANCE(into), INTERIOR_HOURS } (EntranceShuffle.cpp interiorEntrances); rows pair only within one
    # hours class, OWN_HOURS rows never move (test_interior_shuffle.py proves the classes against the door rules)
    ie_body = re.search(r"interiorEntrances\s*=\s*\{(.*?)\n\};", es_src, re.S).group(1)
    ie_rows = re.findall(rf"\{{\s*({ent})\s*,\s*INTERIOR_(ALWAYS_OPEN|DAYTIME|OWN_HOURS)\s*\}}", ie_body)
    if len(ie_rows) != ie_body.count("INTERIOR_") or not ie_rows:
        raise SystemExit("interiorEntrances: a row does not match the expected format")
    hours_of = {}
    for a, hours in ie_rows:
        if entrance_key(a) in hours_of:
            raise SystemExit(f"interiorEntrances: {entrance_key(a)} twice")
        hours_of[entrance_key(a)] = hours

    def entrance_value(key: str) -> int:  # z64scene.h ENTRANCE(scene, spawn)
        scene, spawn = key.split(":")
        return ((entr_scene[f"ENTR_SCENE_{scene}"] & 0x7F) << 9) | ((int(spawn) & 0x1F) << 4)

    # ---- macros from Logic.h ----
    logic_h = strip_comments((rando / "Logic" / "Logic.h").read_text(encoding="utf-8"))
    defines = [d for d in parse_defines(logic_h) if d[0] not in DSL_MACROS and not d[0].endswith("_H")]
    all_regions, region_defines, region_funcs = [], [], []
    for f in sorted((rando / "Logic" / "Regions").glob("*.cpp")):
        regs, defs, funcs = parse_regions(f)
        all_regions += regs
        region_defines += defs
        region_funcs += [(fn, f.name) for fn in funcs]
    for fn, fname in region_funcs:
        if fn not in PRIMITIVES_CALL:
            untranslated.append((fname, f"inline function `{fn}` needs a hand port in primitives.py", ""))

    gen_macros = [d for d in defines if d[0] not in PRIMITIVES_CALL and d[0] not in PRIMITIVES_BARE]
    gen_macros += [(n, p, b, l) for n, p, b, l in region_defines]
    macros0 = {n for n, p, _, _ in gen_macros if p is None}
    macrosN = {n for n, p, _, _ in gen_macros if p is not None}

    mt = Translator(consts, macros0, macrosN, prefix="")
    lines = ['"""AUTO-GENERATED by tools/gen_from_2s2h.py from 2S2H Logic.h. Do not edit."""',
             "# flake8: noqa", "from .primitives import *  # noqa: F401,F403", ""]
    for name, params, body, line in gen_macros:
        where = line if isinstance(line, str) else f"Logic.h:{line}"
        if "##" in body:
            untranslated.append((where, f"macro `{name}` uses ## token pasting, needs a hand port", body))
            continue
        mt.params = set(params or [])
        code, unk = mt.translate(body)
        for u in unk:
            untranslated.append((where, f"macro `{name}`: {u}", body))
        sig = ", ".join(["s"] + (params or []))
        lines.append(f"def {name}({sig}):  # {where}\n    return {code}\n")
    mt.params = set()
    # CanKillEnemy switch
    kills = parse_switch_returns(logic_h, "inline bool CanKillEnemy(")
    lines.append("KILL_RULES = {")
    for cases, expr, line in kills:
        code, unk = mt.translate(expr)
        for u in unk:
            untranslated.append((f"Logic.h:{line}", f"CanKillEnemy {cases[0]}: {u}", expr))
        for c in cases:
            lines.append(f"    {c!r}: lambda s: {code},")
    lines.append("}\n")
    (OUT / "macros_generated.py").write_text("\n".join(lines), encoding="utf-8")

    # ---- regions ----
    rt = Translator(consts, macros0, macrosN, prefix="h.")
    rules: dict[str, str] = {}
    rule_lines = []

    def rule(cond: str, where: str):
        cond = " ".join(cond.split())
        if cond == "true":
            return None
        if cond not in rules:
            code, unk = rt.translate(cond)
            for u in unk:
                untranslated.append((where, u, cond))
            fname = f"r{len(rules)}"
            rules[cond] = fname
            rule_lines.append(f"def {fname}(s):\n    return {code}\n")
        return rules[cond]

    # entrance -> region
    # GetRegionIdFromEntrance (Logic.cpp): iterate std::map Regions in RandoRegionId order, later regions
    # overwrite earlier ones when several claim the same return entrance (e.g. GREAT_BAY_COAST:0).
    rr_order = E("RandoRegionId")
    ent2reg, ent_claims = {}, defaultdict(list)
    for r in sorted(all_regions, key=lambda r: rr_order[r["id"]]):
        for _, frm, _, _ in r["exits"]:
            if frm:
                ent2reg[frm] = r["id"]
                ent_claims[frm].append(r["id"])
        for e in r["one_way"]:
            ent2reg[e] = r["id"]
            ent_claims[e].append(r["id"])
    shared_entrances = {e: rs for e, rs in ent_claims.items() if len(set(rs)) > 1}
    # RR_MAX lives in Logic.cpp
    logic_cpp = strip_comments((rando / "Logic" / "Logic.cpp").read_text(encoding="utf-8"))
    tmp = pathlib.Path(OUT / "_rrmax.tmp")
    m = re.search(r"Regions\[RR_MAX\].*?\n    \};", logic_cpp, re.S)
    tmp.write_text(m.group(0), encoding="utf-8")
    rrmax, _, _ = parse_regions(tmp)
    tmp.unlink()
    rrmax[0]["loc"] = "Logic.cpp"
    for i, (to, frm, cond, loc) in enumerate(rrmax[0]["exits"]):
        rrmax[0]["exits"][i] = (to, frm, cond, loc.replace("_rrmax.tmp", "Logic.cpp(RR_MAX)"))
    all_regions = rrmax + all_regions

    out_regions = []
    check_regions = defaultdict(list)
    for r in all_regions:
        checks = [(c, rule(cond, loc)) for c, cond, loc in r["checks"]]
        for c, _ in checks:
            check_regions[c].append(r["id"])
            if c not in rc_table:
                untranslated.append((r["loc"], f"check `{c}` missing from Checks.cpp", ""))
        conns = [(t, rule(cond, loc)) for t, cond, loc in r["connections"]]
        exits = []
        for to, frm, cond, loc in r["exits"]:
            tgt = ent2reg.get(to)
            if tgt is None:
                # GetRegionIdFromEntrance() returns RR_MAX for unknown entrances; mirror that (upstream gap)
                justified.append((loc, f"exit to `{to}` has no region in 2S2H; like C++ it resolves to RR_MAX", cond))
                tgt = "RR_MAX"
            elif to in shared_entrances:
                justified.append((loc, f"entrance `{to}` is claimed by {sorted(set(shared_entrances[to]))}; like C++ it "
                                       f"resolves to the last in RandoRegionId order, {tgt}", ""))
            exits.append((to, tgt, rule(cond, loc)))
        events = [(e, rule(cond, loc)) for e, cond, loc in r["events"]]
        stay = [(s, rule(cond, loc)) for s, cond, loc in r["stay"]]
        if r["can_stay"] is not None:
            untranslated.append((r["loc"], "canStayOverTime set: time-logic closed form invalid (docs/time-logic.md)", r["can_stay"]))
        out_regions.append((r["id"], r["name"], r["scene"], checks, conns, exits, events, stay))

    # ---- entrance shuffle rows against the region graph ----
    exits_of = defaultdict(list)  # entrance -> [(source region, its scene, target region)]
    for rid, _, scene, _, _, exits, _, _ in out_regions:
        for to, tgt, _ in exits:
            exits_of[to].append((rid, scene, tgt))
    entrance_shuffle = []
    for opt, into, out, inside, outside, boss, warps, temple_exit in es_rows:
        if opt not in E("RandoOptionId"):
            raise SystemExit(f"shuffleEntrances: unknown option {opt}")
        grotto = opt == "RO_SHUFFLE_GROTTOS"
        overworld = opt == "RO_SHUFFLE_OVERWORLD"
        # Overworld places are also the logic's way back out of grottos (TERMINA_FIELD:0, GREAT_BAY_COAST:0, ...):
        # an overworld row is the exit in its own scene.
        doors = [x for x in exits_of[into] if not overworld or x[1] == outside]
        if len(doors) != 1 or doors[0][1] != outside:
            raise SystemExit(f"shuffleEntrances: {into} must be the exit of exactly one region in {outside}, "
                             f"found {exits_of[into]}")
        door, _, behind = doors[0]
        # A grotto's way back is not unique (11 grottos return to TERMINA_FIELD:0): take the one of its own region.
        outs = [x for x in exits_of[out] if (not grotto and not overworld) or x[0] == behind]
        if len(outs) != 1 or outs[0][1] != inside:
            raise SystemExit(f"shuffleEntrances: {out} must be the exit of exactly one region in {inside}, found {outs}")
        inner, _, back = outs[0]
        # Grotto 27's way back GREAT_BAY_COAST:0 is ambiguous and resolves to RR_GREAT_BAY_COAST (UNTRANSLATED.md),
        # while its hole is on the cliffside. Shuffled, a grotto returns to its hole's region, as in game.
        back_scene = next(scene for rid, _, scene, *_ in out_regions if rid == back)
        if behind != inner or (temple_exit is None and back != door and not (grotto and back_scene == outside)):
            raise SystemExit(f"shuffleEntrances: {into} ({door} -> {behind}) and {out} ({inner} -> {back}) "
                             f"are not each other's way back")
        if grotto != (into in grotto_of):
            raise SystemExit(f"shuffleEntrances: {into} must have a grottoEntrances row exactly if it is a grotto")
        interior = opt == "RO_SHUFFLE_INTERIORS"
        if interior != (into in hours_of):
            raise SystemExit(f"shuffleEntrances: {into} must have an interiorEntrances row exactly if it is an interior")
        if interior and (behind == door or back != door or boss != "SCENE_MAX" or warps or temple_exit is not None):
            raise SystemExit(f"shuffleEntrances: interior {into} must be a plain door into a dead end and back")
        boss_warps = [(rid, w) for w in warps for rid, scene, _ in exits_of[w] if scene == boss]
        if (boss == "SCENE_MAX") != (not boss_warps):
            raise SystemExit(f"shuffleEntrances: boss warps {warps} of {boss} are no exit of a {boss} region")
        exterior_region = None
        if temple_exit is not None:
            if inside != boss or out not in warps or len(exits_of[temple_exit]) != 1 or exits_of[temple_exit][0][1] != outside:
                raise SystemExit(f"shuffleEntrances: invalid boss door/temple exit: {into}, {temple_exit}")
            exterior_region = exits_of[temple_exit][0][2]
        entrance_shuffle.append({"option": opt, "into": into, "out": out, "into_value": entrance_value(into),
                                 "door_region": door, "inside_region": inner, "boss_warps": boss_warps,
                                 "temple_exit": temple_exit, "exterior_region": exterior_region,
                                 "grotto": grotto_of.get(into), "hours": hours_of.get(into)})
    check_grottos(entrance_shuffle, grotto_of, rc_table, out_regions, rando)
    check_interiors(entrance_shuffle, hours_of, out_regions)
    check_overworld(entrance_shuffle, es_rows, out_regions)

    # Door keys: { RI_DOOR_KEY_X, "Building", { ENTRANCE(into), ... } } (MiscBehavior/DoorKeys.cpp doorKeys). Every
    # "into" must be the exit of exactly one region and lead into another scene; one key per building, in enum order.
    dk_src = strip_comments((rando / "MiscBehavior" / "DoorKeys.cpp").read_text(encoding="utf-8"))
    dk_body = re.search(r"doorKeys\s*=\s*\{(.*?)\n\};", dk_src, re.S).group(1)
    dk_rows = re.findall(r'\{\s*(RI_DOOR_KEY_\w+)\s*,\s*"([^"]+)"\s*,\s*\{([^{}]*)\}\s*\}', dk_body)
    if len(dk_rows) != dk_body.count("RI_DOOR_KEY_") or not dk_rows:
        raise SystemExit("doorKeys: a row does not match the expected format")
    dk_enum = [ri for ri in E("RandoItemId") if ri.startswith("RI_DOOR_KEY_")]
    if [ri for ri, _, _ in dk_rows] != dk_enum:
        raise SystemExit(f"doorKeys rows and RI_DOOR_KEY_* enum differ (order matters): {[r for r, _, _ in dk_rows]}")
    door_keys, door_key_of = [], {}
    for ri, building, intos in dk_rows:
        flag = f"RANDO_INF_OBTAINED_DOOR_KEY_{ri[len('RI_DOOR_KEY_'):]}"
        if flag not in E("RandoInf") or E("RandoInf")[flag] - E("RandoInf")["RANDO_INF_OBTAINED_DOOR_KEY_TRADING_POST"] \
                != E("RandoItemId")[ri] - E("RandoItemId")["RI_DOOR_KEY_TRADING_POST"]:
            raise SystemExit(f"doorKeys {ri}: {flag} missing or out of order (DOOR_KEY_RI_TO_RANDO_INF)")
        if ri_table.get(ri, {}).get("name") != f"{building} Key":
            raise SystemExit(f"doorKeys {ri}: Items.cpp name must be '{building} Key', is {ri_table.get(ri)}")
        keys = [entrance_key(x) for x in re.findall(ent, intos)]
        if not keys or len(keys) != intos.count("ENTRANCE"):
            raise SystemExit(f"doorKeys {ri}: bad entrance list {intos!r}")
        doors, insides = [], []
        for key in keys:
            found = exits_of[key]
            if len(found) != 1 or key in door_key_of:
                raise SystemExit(f"doorKeys {ri}: {key} must be the exit of exactly one region and of one building, "
                                 f"found {found}, already {door_key_of.get(key)}")
            door, door_scene, inside = found[0]
            inside_scene = next(scene for rid, _, scene, *_ in out_regions if rid == inside)
            if inside_scene == door_scene:
                raise SystemExit(f"doorKeys {ri}: {key} ({door} -> {inside}) does not lead into another scene")
            door_key_of[key] = ri
            doors.append(door)
            insides.append(inside)
        door_keys.append({"key": ri, "building": building, "into": keys, "into_values": [entrance_value(k) for k in keys],
                          "door_regions": doors, "inside_regions": insides})

    # Area rules: { RO_X, ENTRANCE(into), [] { return condition; } } (EntranceShuffle.cpp shuffleAreaRules), translated
    # like every CONNECTION rule; the apworld adds them to a door with another row's area behind it
    ar_body = re.search(r"shuffleAreaRules\s*=\s*\{(.*?)\n\};", es_src, re.S).group(1)
    ar_rows = re.findall(rf"\{{\s*(RO_\w+)\s*,\s*({ent})\s*,\s*\[\]\s*\{{\s*return\s+(.*?);\s*\}}\s*\}}", ar_body, re.S)
    if len(ar_rows) != ar_body.count("RO_"):
        raise SystemExit("shuffleAreaRules: a row does not match the expected format")
    es_options = {row["into"]: row["option"] for row in entrance_shuffle}
    area_rules = {}
    for opt, into, cond in ar_rows:
        into = entrance_key(into)
        if es_options.get(into) != opt or into in area_rules:
            raise SystemExit(f"shuffleAreaRules: {opt} {into} is no shuffleEntrances row (or a duplicate)")
        before = len(untranslated)
        area_rules[into] = rule(cond, "EntranceShuffle.cpp(shuffleAreaRules)")
        if area_rules[into] is None or len(untranslated) != before:
            raise SystemExit(f"shuffleAreaRules: {into}: rule `{cond}` is empty or untranslatable")

    # ---- time-logic guard (docs/time-logic.md) ----
    time_ops = {"AT", "BEFORE", "AFTER", "BETWEEN", "IS_DAY1", "IS_NIGHT1", "IS_DAY2", "IS_NIGHT2", "IS_DAY3",
                "IS_NIGHT3", "IS_DAY", "IS_NIGHT", "FIRST_DAY", "SECOND_DAY", "FINAL_DAY", "MIDNIGHT"}
    rrmax_r = all_regions[0]
    for cond in [c for _, c, _ in rrmax_r["checks"]] + [c for _, _, c, _ in rrmax_r["exits"]]:
        if any(re.search(rf"\b{t}\b", cond) for t in time_ops):
            untranslated.append(("Logic.cpp", "RR_MAX condition uses a time operator: closed form invalid", cond))
    stay_regions = {r["id"] for r in all_regions if r["stay"]}
    for to, _, _, _ in rrmax_r["exits"]:
        if ent2reg.get(to) in stay_regions:
            untranslated.append(("Logic.cpp", f"RR_MAX exit target {ent2reg[to]} has STAY: closed form invalid", ""))
    for r in all_regions:
        for s_, cond, loc in r["stay"]:
            if any(re.search(rf"\b{t}\b", cond) for t in time_ops):
                untranslated.append((loc, "STAY condition uses a time operator: closed form invalid", cond))

    lines = ['"""AUTO-GENERATED by tools/gen_from_2s2h.py from 2S2H Logic/Regions. Do not edit."""',
             "# flake8: noqa", "from . import macros_generated as h", ""]
    lines += rule_lines
    lines.append("# id: (name, scene, checks[(RC, rule)], connections[(RR, rule)], exits[(entrance, RR, rule)],")
    lines.append("#      events[(RE, rule)], stay[(TIME_SLICE, rule)])")
    lines.append("REGIONS = {")
    for rid, name, scene, checks, conns, exits, events, stay in out_regions:
        f = lambda xs: "[" + ", ".join("(" + ", ".join(repr(a) if not (isinstance(a, str) and a.startswith("r") and a[1:].isdigit()) else a for a in x) + ")" for x in xs) + "]"  # noqa: E731
        lines.append(f"    {rid!r}: ({name!r}, {scene!r}, {f(checks)}, {f(conns)}, {f(exits)}, {f(events)}, {f(stay)}),")
    lines.append("}")
    lines.append("# shuffleAreaRules (EntranceShuffle.cpp): door entrance `into` -> rule of the area behind it")
    lines.append("ENTRANCE_AREA_RULES = {" + ", ".join(f"{k!r}: {v}" for k, v in area_rules.items()) + "}")
    (OUT / "regions_generated.py").write_text("\n".join(lines) + "\n", encoding="utf-8")

    # ---- enums / items / locations ----
    rc_ids, ri_ids, rr_ids, re_ids, ro_ids = E("RandoCheckId"), E("RandoItemId"), E("RandoRegionId"), E("RandoEvent"), E("RandoOptionId")
    with open(OUT / "enums_generated.py", "w", encoding="utf-8") as fh:
        fh.write('"""AUTO-GENERATED by tools/gen_from_2s2h.py from 2S2H Types.h. Do not edit."""\n# flake8: noqa\n')
        for ename in ("RandoCheckType", "RandoItemType", "RandoOptionId", "RandoInf", "RandoEvent"):
            fh.write(f"{ename} = {dict(E(ename))!r}\n")
        option_enums = {k: v for k, v in enums.items() if k.startswith("RandoOption") or k == "RandoClockShuffleOptions"}
        fh.write(f"OPTION_ENUMS = {option_enums!r}\n")
        fh.write(f"RC_ID = {dict(rc_ids)!r}\nRI_ID = {dict(ri_ids)!r}\n")
        fh.write(f"ENEMY_SOULS = {souls!r}\n")
        fh.write("# RI: ([actors], [checks that need the soul], [events that need the soul])\n")
        fh.write(f"NPC_SOULS = {npc_souls!r}\n")
        fh.write("# shuffleable doors (EntranceShuffle.cpp), in row order: exits `into` (door_region -> inside_region)\n"
                 "# and `out` (back), `into_value` = game entrance, boss_warps = [(boss region, warp exit)]\n")
        fh.write(f"ENTRANCE_SHUFFLE = {entrance_shuffle!r}\n")
        fh.write("# buildings locked by Shuffle Door Keys (DoorKeys.cpp), in enum order: exits `into` (door_regions ->\n"
                 "# inside_regions) need the key; `into_values` = game entrances\n")
        fh.write(f"DOOR_KEYS = {door_keys!r}\n")
        ts = re.search(r"enum TimeSlice\s*\{(.*?)\}", logic_h, re.S).group(1)
        names = [x.split("=")[0].strip() for x in ts.split(",") if x.strip()]
        fh.write(f"TIME_SLICE = {dict((n, i) for i, n in enumerate(names))!r}\n")
        # option defaults (StaticData/Options.cpp); symbolic defaults from headers
        header_defs = {}
        for hdr in [rando / "StaticData" / "StaticData.h", root / "mm" / "include" / "z64save.h",
                    root / "mm" / "src" / "overlays" / "actors" / "ovl_En_Si" / "z_en_si.h"] + \
                list((rando).glob("**/*.h")):
            for name_, val in re.findall(r"#define\s+(\w+)\s+(\d+)\b", hdr.read_text(encoding="utf-8", errors="ignore")):
                header_defs.setdefault(name_, int(val))
        opt_src = strip_comments((rando / "StaticData" / "Options.cpp").read_text(encoding="utf-8"))
        defaults = {}
        for oid, dv in re.findall(r"RO\((RO_\w+),\s*(\w+)\)", opt_src):
            if dv.isdigit():
                defaults[oid] = int(dv)
            elif dv in consts:
                defaults[oid] = consts[dv]
            elif dv in header_defs:
                defaults[oid] = header_defs[dv]
            else:
                untranslated.append(("Options.cpp", f"default of {oid} unknown: {dv}", ""))
        fh.write(f"OPTION_DEFAULTS = {defaults!r}\n")

    names_seen = {}
    with open(OUT / "items_generated.py", "w", encoding="utf-8") as fh:
        fh.write('"""AUTO-GENERATED by tools/gen_from_2s2h.py from 2S2H Items.cpp. Do not edit."""\n# flake8: noqa\n')
        fh.write("# RI: (ap_name, RITYPE, vanilla ITEM_ id)\nITEMS = {\n")
        for ri, d in ri_table.items():
            if ri == "RI_ARCHIPELAGO_ITEM":  # client-side placeholder for other worlds' items
                continue
            name = d["name"]
            if name in names_seen:
                name = f"{name} ({ri[3:].replace('_', ' ').title()})"
            names_seen[name] = ri
            fh.write(f"    {ri!r}: ({name!r}, {d['type']!r}, {d['item']!r}),\n")
        fh.write("}\n")

    loc_names = {}
    with open(OUT / "locations_generated.py", "w", encoding="utf-8") as fh:
        fh.write('"""AUTO-GENERATED by tools/gen_from_2s2h.py from 2S2H Checks.cpp and Logic/Regions. Do not edit."""\n# flake8: noqa\n')
        fh.write("# RC: (ap_name, RCTYPE, scene, vanilla RI, [regions])\nLOCATIONS = {\n")
        for rc, regs in check_regions.items():
            d = rc_table.get(rc, {"type": "RCTYPE_UNKNOWN", "scene": "", "vanilla": "RI_UNKNOWN"})
            name = readable(rc)
            assert name not in loc_names, (name, rc)
            loc_names[name] = rc
            fh.write(f"    {rc!r}: ({name!r}, {d['type']!r}, {d['scene']!r}, {d['vanilla']!r}, {regs!r}),\n")
        fh.write("}\n")

    # ---- report ----
    with open(OUT / "UNTRANSLATED.md", "w", encoding="utf-8") as fh:
        fh.write("# Untranslated logic\n\nAUTO-GENERATED by tools/gen_from_2s2h.py. Every entry compiles to "
                 "`h.UNTRANSLATED(...)` and must be resolved or justified here.\n\n")
        if not untranslated:
            fh.write("_Nothing. All conditions were translated._\n")
        for where, what, srcx in untranslated:
            fh.write(f"- `{where}`: {what}" + (f"\n  `{' '.join(srcx.split())[:200]}`" if srcx else "") + "\n")
        fh.write("\n## Justified (mirrors 2S2H behaviour, no action needed)\n\n")
        for where, what, srcx in justified:
            fh.write(f"- `{where}`: {what}" + (f"\n  `{' '.join(srcx.split())[:200]}`" if srcx else "") + "\n")
        fh.write("\nNot modelled on purpose: `timeStayRestrictions` (STAY) are kept in REGIONS but unused, because the "
                 "time mask is uniform in 5.0.1 (docs/time-logic.md); the generator guards that assumption.\n")
    with open(HERE / "primitive_usage.txt", "w", encoding="utf-8") as fh:
        usage = defaultdict(set)
        for t in (mt, rt):
            for k, v in t.usage.items():
                usage[k] |= v
        for k in sorted(usage):
            if k in PRIMITIVES_CALL or k in PRIMITIVES_BARE or k == "RANDO_SAVE_OPTIONS":
                fh.write(f"{k}: {sorted(usage[k], key=str)}\n")

    print(f"regions: {len(out_regions)}  locations: {len(check_regions)} "
          f"(multi-region: {sum(1 for v in check_regions.values() if len(v) > 1)})  "
          f"rules: {len(rules)}  macros: {len(gen_macros)}  kill rules: {sum(len(c) for c, _, _ in kills)}  "
          f"events: {sum(len(r[6]) for r in out_regions)}  items: {len(ri_table)}  untranslated: {len(untranslated)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
