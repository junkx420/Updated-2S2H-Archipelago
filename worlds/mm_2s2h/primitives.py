"""Hand-written primitive layer for the generated 2S2H logic.

2S2H evaluates its logic against a *copied save file*: every item placed so far is given with
GiveItem(ConvertItem(ri)) and the macros in Logic.h read inventory, quest items, flags, etc.
`SaveView` reproduces the parts of that save the logic reads, derived from the RI item counts in an
AP CollectionState (port of Rando/GiveItem.cpp, ConvertItem.cpp and IsItemObtainable). Every primitive
used by the generated code (tools/primitive_usage.txt) is defined here, everything else is generated.
"""
from __future__ import annotations

from collections import Counter

from .enums_generated import ENEMY_SOULS, RandoInf, RI_ID
from .time_logic import TimeContext, TIME_SLICE

# ---------------------------------------------------------------------------------------------
# item -> save effects (GiveItem.cpp / Items.cpp itemId column)
# ---------------------------------------------------------------------------------------------
# RI that put an ITEM_* into the inventory (HAS_ITEM). Bow/bomb/bombchu are derived from upgrades.
INVENTORY = {
    "RI_ARROW_FIRE": "ITEM_ARROW_FIRE", "RI_ARROW_ICE": "ITEM_ARROW_ICE", "RI_ARROW_LIGHT": "ITEM_ARROW_LIGHT",
    "RI_HOOKSHOT": "ITEM_HOOKSHOT", "RI_LENS": "ITEM_LENS_OF_TRUTH", "RI_OCARINA": "ITEM_OCARINA_OF_TIME",
    "RI_PICTOGRAPH_BOX": "ITEM_PICTOGRAPH_BOX", "RI_POWDER_KEG": "ITEM_POWDER_KEG", "RI_ROOM_KEY": "ITEM_ROOM_KEY",
    "RI_GREAT_FAIRY_SWORD": "ITEM_SWORD_GREAT_FAIRY", "RI_MAGIC_BEAN": "ITEM_MAGIC_BEANS",
    "RI_DEKU_NUT": "ITEM_DEKU_NUT", "RI_DEKU_NUTS_5": "ITEM_DEKU_NUT", "RI_DEKU_NUTS_10": "ITEM_DEKU_NUT",
    "RI_DEKU_STICK": "ITEM_DEKU_STICK", "RI_DEKU_STICKS_5": "ITEM_DEKU_STICK",
    "RI_MOONS_TEAR": "ITEM_MOONS_TEAR", "RI_DEED_LAND": "ITEM_DEED_LAND", "RI_DEED_SWAMP": "ITEM_DEED_SWAMP",
    "RI_DEED_MOUNTAIN": "ITEM_DEED_MOUNTAIN", "RI_DEED_OCEAN": "ITEM_DEED_OCEAN",
    "RI_LETTER_TO_KAFEI": "ITEM_LETTER_TO_KAFEI", "RI_LETTER_TO_MAMA": "ITEM_LETTER_MAMA",
    "RI_PENDANT_OF_MEMORIES": "ITEM_PENDANT_OF_MEMORIES",
}
MASKS = [
    "DEKU", "GORON", "ZORA", "FIERCE_DEITY", "TRUTH", "KAFEIS_MASK", "ALL_NIGHT", "BUNNY", "KEATON", "GARO",
    "ROMANI", "CIRCUS_LEADER", "POSTMAN", "COUPLE", "GREAT_FAIRY", "GIBDO", "DON_GERO", "KAMARO", "CAPTAIN",
    "STONE", "BREMEN", "BLAST", "SCENTS", "GIANT",
]
for _m in MASKS:
    INVENTORY[f"RI_MASK_{_m}"] = f"ITEM_MASK_{_m}"
# ITEM_MASK_TRUTH .. ITEM_MASK_GIANT in z64item.h order (MoonMaskCount)
MOON_MASKS = [f"ITEM_MASK_{m}" for m in MASKS[4:]]

QUEST_ORDER = ["QUEST_SONG_SONATA", "QUEST_SONG_LULLABY", "QUEST_SONG_BOSSA_NOVA", "QUEST_SONG_ELEGY",
               "QUEST_SONG_OATH", "QUEST_SONG_SARIA", "QUEST_SONG_TIME", "QUEST_SONG_HEALING", "QUEST_SONG_EPONA",
               "QUEST_SONG_SOARING", "QUEST_SONG_STORMS", "QUEST_SONG_SUN"]
QUEST_INDEX = {q: 6 + i for i, q in enumerate(QUEST_ORDER)}
QUEST_INDEX["QUEST_SONG_LULLABY_INTRO"] = 0x18
OCARINA_SONGS = ["SONATA", "GORON_LULLABY", "NEW_WAVE", "ELEGY", "OATH", "SARIAS", "TIME", "HEALING", "EPONAS",
                 "SOARING", "STORMS", "SUNS", "INVERTED_TIME", "DOUBLE_TIME", "GORON_LULLABY_INTRO",
                 "WIND_FISH_HUMAN", "WIND_FISH_GORON", "WIND_FISH_ZORA", "WIND_FISH_DEKU", "EVAN_PART1",
                 "EVAN_PART2", "ZELDAS_LULLABY", "SCARECROW_SPAWN", "TERMINA_WALL"]
QUEST = {
    "RI_SONG_SONATA": "QUEST_SONG_SONATA", "RI_SONG_LULLABY": "QUEST_SONG_LULLABY",
    "RI_SONG_LULLABY_INTRO": "QUEST_SONG_LULLABY_INTRO", "RI_SONG_NOVA": "QUEST_SONG_BOSSA_NOVA",
    "RI_SONG_ELEGY": "QUEST_SONG_ELEGY", "RI_SONG_OATH": "QUEST_SONG_OATH", "RI_SONG_SARIA": "QUEST_SONG_SARIA",
    "RI_SONG_TIME": "QUEST_SONG_TIME", "RI_SONG_HEALING": "QUEST_SONG_HEALING", "RI_SONG_EPONA": "QUEST_SONG_EPONA",
    "RI_SONG_SOARING": "QUEST_SONG_SOARING", "RI_SONG_STORMS": "QUEST_SONG_STORMS", "RI_SONG_SUN": "QUEST_SONG_SUN",
    "RI_REMAINS_ODOLWA": "QUEST_REMAINS_ODOLWA", "RI_REMAINS_GOHT": "QUEST_REMAINS_GOHT",
    "RI_REMAINS_GYORG": "QUEST_REMAINS_GYORG", "RI_REMAINS_TWINMOLD": "QUEST_REMAINS_TWINMOLD",
    "RI_BOMBERS_NOTEBOOK": "QUEST_BOMBERS_NOTEBOOK",
}
FLAGS = {
    "RI_MOONS_TEAR": "RANDO_INF_OBTAINED_MOONS_TEAR", "RI_DEED_LAND": "RANDO_INF_OBTAINED_DEED_LAND",
    "RI_DEED_SWAMP": "RANDO_INF_OBTAINED_DEED_SWAMP", "RI_DEED_MOUNTAIN": "RANDO_INF_OBTAINED_DEED_MOUNTAIN",
    "RI_DEED_OCEAN": "RANDO_INF_OBTAINED_DEED_OCEAN", "RI_ROOM_KEY": "RANDO_INF_OBTAINED_ROOM_KEY",
    "RI_LETTER_TO_MAMA": "RANDO_INF_OBTAINED_LETTER_TO_MAMA", "RI_LETTER_TO_KAFEI": "RANDO_INF_OBTAINED_LETTER_TO_KAFEI",
    "RI_PENDANT_OF_MEMORIES": "RANDO_INF_OBTAINED_PENDANT_OF_MEMORIES", "RI_ABILITY_SWIM": "RANDO_INF_OBTAINED_SWIM",
    "RI_SONG_DOUBLE_TIME": "RANDO_INF_OBTAINED_SONG_DOUBLE_TIME",
    "RI_SONG_INVERTED_TIME": "RANDO_INF_OBTAINED_SONG_INVERTED_TIME",
}
_inf_by_value = {v: k for k, v in RandoInf.items()}
for _ri, _v in RI_ID.items():  # souls: SOUL_RI_TO_RANDO_INF / NPC_SOUL_RI_TO_RANDO_INF
    if _ri.startswith("RI_SOUL_NPC_"):
        FLAGS[_ri] = "RANDO_INF_OBTAINED_SOUL_OF_NPC_" + _ri[len("RI_SOUL_NPC_"):]
        assert FLAGS[_ri] in RandoInf
    elif _ri.startswith("RI_SOUL_"):
        FLAGS[_ri] = _inf_by_value[_v - RI_ID["RI_SOUL_BOSS_GOHT"] + RandoInf["RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT"]]
for _i, _b in enumerate(["A", "C_DOWN", "C_RIGHT", "C_LEFT", "C_UP"]):
    _ri = f"RI_OCARINA_BUTTON_{_b}"
    FLAGS[_ri] = _inf_by_value[RandoInf["RANDO_INF_OBTAINED_OCARINA_BUTTON_A"] + RI_ID[_ri] - RI_ID["RI_OCARINA_BUTTON_A"]]
WEEKREG = {
    "RI_CLOCK_TOWN_STRAY_FAIRY": "WEEKEVENTREG_08_80", "RI_GREAT_SPIN_ATTACK": "WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK",
    "RI_FROG_BLUE": "WEEKEVENTREG_33_01", "RI_FROG_CYAN": "WEEKEVENTREG_32_40", "RI_FROG_PINK": "WEEKEVENTREG_32_80",
    "RI_FROG_WHITE": "WEEKEVENTREG_33_02", "RI_POWDER_KEG": "WEEKEVENTREG_HAS_POWDERKEG_PRIVILEGES",
    "RI_SINGLE_MAGIC": "WEEKEVENTREG_12_80", "RI_DOUBLE_MAGIC": "WEEKEVENTREG_12_80",
}
OWLS = {"RI_OWL_CLOCK_TOWN_SOUTH": "OWL_WARP_CLOCK_TOWN", "RI_OWL_GREAT_BAY_COAST": "OWL_WARP_GREAT_BAY_COAST",
        "RI_OWL_IKANA_CANYON": "OWL_WARP_IKANA_CANYON", "RI_OWL_MILK_ROAD": "OWL_WARP_MILK_ROAD",
        "RI_OWL_MOUNTAIN_VILLAGE": "OWL_WARP_MOUNTAIN_VILLAGE", "RI_OWL_SNOWHEAD": "OWL_WARP_SNOWHEAD",
        "RI_OWL_SOUTHERN_SWAMP": "OWL_WARP_SOUTHERN_SWAMP", "RI_OWL_STONE_TOWER": "OWL_WARP_STONE_TOWER",
        "RI_OWL_WOODFALL": "OWL_WARP_WOODFALL", "RI_OWL_ZORA_CAPE": "OWL_WARP_ZORA_CAPE"}
DUNGEONS = {"WOODFALL": "WOODFALL_TEMPLE", "SNOWHEAD": "SNOWHEAD_TEMPLE", "GREAT_BAY": "GREAT_BAY_TEMPLE",
            "STONE_TOWER": "STONE_TOWER_TEMPLE"}
SKELETON_KEYS = {"WOODFALL_TEMPLE": 1, "SNOWHEAD_TEMPLE": 3, "GREAT_BAY_TEMPLE": 1, "STONE_TOWER_TEMPLE": 4}
BOTTLES = {"RI_BOTTLE_EMPTY", "RI_BOTTLE_CHATEAU_ROMANI", "RI_BOTTLE_GOLD_DUST", "RI_BOTTLE_MILK", "RI_BOTTLE_RED_POTION"}
CLOCKS = ["RI_TIME_DAY_1", "RI_TIME_NIGHT_1", "RI_TIME_DAY_2", "RI_TIME_NIGHT_2", "RI_TIME_DAY_3", "RI_TIME_NIGHT_3"]

# every RI that changes something the logic reads: these are progression in AP
LOGIC_ITEMS = (set(INVENTORY) | set(QUEST) | set(FLAGS) | set(WEEKREG) | set(OWLS) | BOTTLES | set(CLOCKS) | {
    "RI_TIME_PROGRESSIVE", "RI_PROGRESSIVE_SWORD", "RI_SWORD_KOKIRI", "RI_SWORD_RAZOR", "RI_SWORD_GILDED",
    "RI_SHIELD_HERO", "RI_SHIELD_MIRROR", "RI_PROGRESSIVE_BOW", "RI_BOW", "RI_QUIVER_40", "RI_QUIVER_50",
    "RI_PROGRESSIVE_BOMB_BAG", "RI_BOMB_BAG_20", "RI_BOMB_BAG_30", "RI_BOMB_BAG_40", "RI_PROGRESSIVE_WALLET",
    "RI_WALLET_ADULT", "RI_WALLET_GIANT", "RI_WALLET_TYCOON", "RI_PROGRESSIVE_MAGIC", "RI_PROGRESSIVE_LULLABY",
    "RI_HEART_PIECE", "RI_HEART_CONTAINER", "RI_SKELETON_KEY", "RI_GS_TOKEN_SWAMP", "RI_GS_TOKEN_OCEAN",
    "RI_TRIFORCE_PIECE",
} | {f"RI_{d}_{k}" for d in DUNGEONS for k in ("SMALL_KEY", "BOSS_KEY", "STRAY_FAIRY")})


class SaveView:
    """The slice of a 2S2H save file that Logic.h reads, for one CollectionState."""

    def __init__(self, ri: Counter, events: Counter, world):
        self.ri, self.events, self.w = ri, events, world
        opt = world.ro
        self.opts = opt
        g = ri.get
        self.inv = {INVENTORY[k] for k in ri if k in INVENTORY}
        self.quest = {QUEST[k] for k in ri if k in QUEST}
        lull = g("RI_PROGRESSIVE_LULLABY", 0)
        if lull >= 1:
            self.quest.add("QUEST_SONG_LULLABY_INTRO")
        if lull >= 2:
            self.quest.add("QUEST_SONG_LULLABY")
        self.flags = {FLAGS[k] for k in ri if k in FLAGS}
        self.weekreg = {WEEKREG[k] for k in ri if k in WEEKREG}
        self.owls = {OWLS[k] for k in ri if k in OWLS}

        def level(prog: str, specific: list[str], cap: int) -> int:
            lv = min(cap, g(prog, 0))
            for i, k in enumerate(specific, 1):
                if g(k, 0):
                    lv = max(lv, i)
            return lv

        self.sword = level("RI_PROGRESSIVE_SWORD", ["RI_SWORD_KOKIRI", "RI_SWORD_RAZOR", "RI_SWORD_GILDED"], 3)
        self.shield = 2 if g("RI_SHIELD_MIRROR", 0) else (1 if g("RI_SHIELD_HERO", 0) else 0)
        self.quiver = level("RI_PROGRESSIVE_BOW", ["RI_BOW", "RI_QUIVER_40", "RI_QUIVER_50"], 3)
        if g("RI_BOW", 0) or g("RI_PROGRESSIVE_BOW", 0):
            self.inv.add("ITEM_BOW")
        self.bombbag = level("RI_PROGRESSIVE_BOMB_BAG", ["RI_BOMB_BAG_20", "RI_BOMB_BAG_30", "RI_BOMB_BAG_40"], 3)
        if self.bombbag:
            self.inv |= {"ITEM_BOMB", "ITEM_BOMBCHU"}
        self.wallet = level("RI_PROGRESSIVE_WALLET", ["RI_WALLET_ADULT", "RI_WALLET_GIANT", "RI_WALLET_TYCOON"], 3)
        self.magic = bool(g("RI_PROGRESSIVE_MAGIC", 0) or g("RI_SINGLE_MAGIC", 0) or g("RI_DOUBLE_MAGIC", 0))
        if self.magic:
            self.weekreg.add("WEEKEVENTREG_12_80")
        hearts = opt.get("RO_STARTING_HEALTH", 3)
        self.health = 16 * hearts + 16 * g("RI_HEART_CONTAINER", 0) + 4 * g("RI_HEART_PIECE", 0)
        self.bottle = any(g(b, 0) for b in BOTTLES)
        self.keys, self.fairies, self.dungeon_items = {}, {}, set()
        skeleton = g("RI_SKELETON_KEY", 0) > 0
        for d, scene in DUNGEONS.items():
            k = g(f"RI_{d}_SMALL_KEY", 0)
            self.keys[scene] = max(k, SKELETON_KEYS[scene]) if skeleton else k
            self.fairies[f"DUNGEON_SCENE_INDEX_{scene}"] = g(f"RI_{d}_STRAY_FAIRY", 0)
            if g(f"RI_{d}_BOSS_KEY", 0):
                self.dungeon_items.add(("DUNGEON_BOSS_KEY", f"DUNGEON_SCENE_INDEX_{scene}"))
        self.tokens = {"SCENE_KINSTA1": g("RI_GS_TOKEN_SWAMP", 0), "SCENE_KINDAN2": g("RI_GS_TOKEN_OCEAN", 0)}
        # clocks: RI_TIME_PROGRESSIVE resolves in mode order (ConvertItem.cpp)
        clock_flags = 0
        for i, c in enumerate(CLOCKS):
            if g(c, 0):
                clock_flags |= 1 << i
        prog = g("RI_TIME_PROGRESSIVE", 0)
        order = range(5, -1, -1) if opt.get("RO_CLOCK_SHUFFLE_PROGRESSIVE") == 2 else range(6)
        for i in order:
            if prog <= 0:
                break
            if not clock_flags >> i & 1:
                clock_flags |= 1 << i
                prog -= 1
        for i in range(6):
            if clock_flags >> i & 1:
                self.flags.add(f"RANDO_INF_OBTAINED_CLOCK_{['DAY_1', 'NIGHT_1', 'DAY_2', 'NIGHT_2', 'DAY_3', 'NIGHT_3'][i]}")
        self.time = world.time_context(clock_flags)

    # used by generated code
    def ev(self, name: str) -> int:
        return self.events.get(name, 0)

    def opt(self, name: str) -> int:
        return self.opts.get(name, 0)


# ---------------------------------------------------------------------------------------------
# primitives (names exactly as in Logic.h)
# ---------------------------------------------------------------------------------------------
def UNTRANSLATED(what):
    raise RuntimeError(f"mm_2s2h: untranslated logic reached: {what}")


def IS_HUMAN(s): return True  # logic always runs as human Link
def IS_DEKU(s): return False
def IS_ZORA(s): return False
def IS_GORON(s): return False
def IS_DEITY(s): return False
def HAS_MAGIC(s): return s.magic
def HAS_BOTTLE(s): return s.bottle
def HAS_ITEM(s, item): return item in s.inv
def HAS_BOTTLE_ITEM(s, item): raise RuntimeError("HAS_BOTTLE_ITEM is not used by 5.0.1 logic")
def CHECK_QUEST_ITEM(s, q): return q in s.quest
def CHECK_DUNGEON_ITEM(s, item, dungeon): return (item, dungeon) in s.dungeon_items
def CHECK_WEEKEVENTREG(s, reg): return reg in s.weekreg
def Flags_GetRandoInf(s, flag): return flag in s.flags
def Flags_GetSceneSwitch(s, *a): raise RuntimeError("Flags_GetSceneSwitch is not used by 5.0.1 logic")
def CAN_OWL_WARP(s, owl): return owl in s.owls
def CHECK_MAX_HP(s, hp): return hp * 16 <= s.health
def KEY_COUNT(s, dungeon): return s.keys[dungeon]
def HAS_ENOUGH_STRAY_FAIRIES(s, d): return s.fairies[d] >= s.opt("RO_STRAY_FAIRIES_REQUIRED")
def HAS_ENOUGH_SKULLTULA_TOKENS(s, scene): return s.tokens[scene] >= s.opt("RO_SKULLTULA_TOKENS_REQUIRED")
def CAN_USE_ABILITY(s, ability): return f"RANDO_INF_OBTAINED_{ability}" in s.flags
def CAN_ACCESS(s, access): return s.ev(f"RE_ACCESS_{access}")
def CAN_USE_MAGIC_ARROW(s, t): return "ITEM_BOW" in s.inv and f"ITEM_ARROW_{t}" in s.inv and s.magic


def GET_CUR_EQUIP_VALUE(s, t):
    return s.sword if t == "EQUIP_TYPE_SWORD" else s.shield


def CUR_UPG_VALUE(s, upg):
    return {"UPG_WALLET": s.wallet, "UPG_BOMB_BAG": s.bombbag, "UPG_QUIVER": s.quiver}[upg]


GET_CUR_UPG_VALUE = CUR_UPG_VALUE


def CAN_AFFORD(s, rc):
    price = s.w.prices.get(rc, 0)
    return price < 100 or (price <= 200 and s.wallet >= 1) or s.wallet >= 2


_BUTTONS = {
    "SONATA": ("C_UP", "C_LEFT", "A", "C_RIGHT"), "GORON_LULLABY": ("A", "C_RIGHT", "C_LEFT"),
    "GORON_LULLABY_INTRO": ("A", "C_RIGHT", "C_LEFT"), "NEW_WAVE": ("C_LEFT", "C_UP", "C_RIGHT", "C_DOWN"),
    "ELEGY": ("C_LEFT", "C_UP", "C_RIGHT", "C_DOWN"), "OATH": ("C_RIGHT", "C_LEFT", "C_DOWN", "A", "C_UP"),
    "WIND_FISH_ZORA": ("C_RIGHT", "C_LEFT", "C_DOWN", "A", "C_UP"),
    "TIME": ("C_RIGHT", "A", "C_DOWN"), "INVERTED_TIME": ("C_RIGHT", "A", "C_DOWN"),
    "DOUBLE_TIME": ("C_RIGHT", "A", "C_DOWN"), "WIND_FISH_GORON": ("C_RIGHT", "A", "C_DOWN"),
    "EVAN_PART1": ("C_RIGHT", "A", "C_DOWN"), "HEALING": ("C_LEFT", "C_RIGHT", "C_DOWN"),
    "SARIAS": ("C_LEFT", "C_RIGHT", "C_DOWN"), "EVAN_PART2": ("C_LEFT", "C_RIGHT", "C_DOWN"),
    "EPONAS": ("C_UP", "C_LEFT", "C_RIGHT"), "WIND_FISH_HUMAN": ("C_UP", "C_LEFT", "C_RIGHT"),
    "SOARING": ("C_DOWN", "C_LEFT", "C_UP"), "STORMS": ("A", "C_DOWN", "C_UP"), "SUNS": ("C_RIGHT", "C_DOWN", "C_UP"),
    "WIND_FISH_DEKU": ("C_RIGHT", "A", "C_DOWN", "C_LEFT"),
    "TERMINA_WALL": ("A", "C_DOWN", "C_RIGHT", "C_LEFT", "C_UP"),
}


def FoundOcarinaButtons(s):
    return sum(f"RANDO_INF_OBTAINED_OCARINA_BUTTON_{b}" in s.flags for b in ("A", "C_DOWN", "C_RIGHT", "C_LEFT", "C_UP"))


def canPlaySong(s, song):
    song = song.removeprefix("OCARINA_SONG_")
    if song == "SCARECROW_SPAWN":
        return FoundOcarinaButtons(s) >= 2
    need = _BUTTONS.get(song)
    return need is None or all(f"RANDO_INF_OBTAINED_OCARINA_BUTTON_{b}" in s.flags for b in need)


def CAN_PLAY_SONG(s, song):
    q = f"QUEST_SONG_{song}"
    return ("ITEM_OCARINA_OF_TIME" in s.inv and q in s.quest
            and canPlaySong(s, OCARINA_SONGS[QUEST_INDEX[q] - QUEST_INDEX["QUEST_SONG_SONATA"]]))


def CanAccessDungeon(s, d):
    ocarina = "ITEM_OCARINA_OF_TIME" in s.inv
    from . import macros_generated as m
    song, form = {
        "DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE": (lambda: CAN_PLAY_SONG(s, "SONATA"), lambda: m.CAN_BE_DEKU(s) and ocarina),
        "DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE": (lambda: CAN_PLAY_SONG(s, "LULLABY"), lambda: m.CAN_BE_GORON(s) and ocarina),
        "DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE": (lambda: CAN_PLAY_SONG(s, "BOSSA_NOVA"), lambda: m.CAN_BE_ZORA(s) and ocarina),
    }.get(d, (lambda: False, lambda: False))
    mode = s.opt("RO_ACCESS_DUNGEONS")
    if mode == 1:
        return song() or form()
    if mode == 2:
        return form()
    if mode == 3:
        return song()
    if mode == 4:
        return True
    return song() and form()


def MoonMaskCount(s): return sum(m in s.inv for m in MOON_MASKS)
def RemainsCount(s): return sum(f"QUEST_REMAINS_{b}" in s.quest for b in ("ODOLWA", "GOHT", "GYORG", "TWINMOLD"))


def MeetsMoonRequirements(s):
    return RemainsCount(s) >= s.opt("RO_ACCESS_MOON_REMAINS_COUNT") and MoonMaskCount(s) >= s.opt("RO_ACCESS_MOON_MASKS_COUNT")


def ClockCount(s):
    return sum(f"RANDO_INF_OBTAINED_CLOCK_{c}" in s.flags for c in ("DAY_1", "NIGHT_1", "DAY_2", "NIGHT_2", "DAY_3", "NIGHT_3"))


def HaveEnemySoul(s, actor):
    ri = ENEMY_SOULS.get(actor)
    return ri is None or FLAGS[ri] in s.flags


def CanKillEnemy(s, actor):
    if s.opt("RO_SHUFFLE_ENEMY_SOULS") and not HaveEnemySoul(s, actor):
        return False
    from .macros_generated import KILL_RULES
    return KILL_RULES[actor](s)


# South.cpp inline functions (hand port)
def CanGetPastBigOctoWithoutBoat(s):
    from . import macros_generated as m
    if s.ev("RE_SOUTHERN_SWAMP_KILL_OCTOROK") and m.CAN_BE_DEKU(s):
        return True
    return bool(s.ev("RE_CLEARED_WOODFALL_TEMPLE") and m.CAN_TRAVERSE_WAIST_DEEP_WATER(s))


def CanGetPastBigOcto(s):
    return bool(s.ev("RE_SOUTHERN_SWAMP_RIDE_BOAT")) or CanGetPastBigOctoWithoutBoat(s)


# time (TimeContext, see docs/time-logic.md)
def AT(s, t): return s.time.at(TIME_SLICE[t])
def BEFORE(s, t): return s.time.before(TIME_SLICE[t])
def AFTER(s, t): return s.time.after(TIME_SLICE[t])
def BETWEEN(s, a, b): return s.time.between(TIME_SLICE[a], TIME_SLICE[b])
def IS_DAY1(s): return s.time.is_day1()
def IS_NIGHT1(s): return s.time.is_night1()
def IS_DAY2(s): return s.time.is_day2()
def IS_NIGHT2(s): return s.time.is_night2()
def IS_DAY3(s): return s.time.is_day3()
def IS_NIGHT3(s): return s.time.is_night3()
def IS_DAY(s): return s.time.is_day()
def IS_NIGHT(s): return s.time.is_night()
def FIRST_DAY(s): return s.time.first_day()
def SECOND_DAY(s): return s.time.second_day()
def FINAL_DAY(s): return s.time.final_day()
def CLOCK_DAY1(s): return s.time.clock(0)
def CLOCK_NIGHT1(s): return s.time.clock(1)
def CLOCK_DAY2(s): return s.time.clock(2)
def CLOCK_NIGHT2(s): return s.time.clock(3)
def CLOCK_DAY3(s): return s.time.clock(4)
def CLOCK_NIGHT3(s): return s.time.clock(5)


def MIDNIGHT(s):
    return (BETWEEN(s, "TIME_NIGHT1_AM_12_00", "TIME_DAY2_AM_06_00") or BETWEEN(s, "TIME_NIGHT2_AM_12_00", "TIME_DAY3_AM_06_00")
            or AFTER(s, "TIME_NIGHT3_AM_12_00"))
