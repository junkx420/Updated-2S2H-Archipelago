"""2 Ship 2 Harkinian (Majora's Mask PC port) for Archipelago.

Regions, rules, locations and items are generated from the 2S2H sources by tools/gen_from_2s2h.py;
this module only builds the AP objects and ports the pool construction of Rando/Logic/GeneratePools.cpp
and Rando/StartingItems.cpp.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from typing import Any, ClassVar

from BaseClasses import CollectionState, Entrance, EntranceType, Item, ItemClassification as IC, Location, Region, Tutorial
from entrance_rando import disconnect_entrance_for_randomization, randomize_entrances
from Fill import fill_restrictive
from Options import OptionError
from worlds.AutoWorld import LogicMixin, WebWorld, World

from .enums_generated import ENTRANCE_SHUFFLE, NPC_SOULS, OPTION_DEFAULTS, RC_ID, RI_ID
from .items_generated import ITEMS
from .locations_generated import LOCATIONS
from .macros_generated import CAN_USE_SWORD
from .options import MM2S2HOptions
from .primitives import LOGIC_ITEMS, SaveView
from .regions_generated import ENTRANCE_AREA_RULES, REGIONS
from .time_logic import TimeContext

GAME = "Majora's Mask (2 Ship 2 Harkinian)"
BASE_ID = 0x2D2D0000  # AP id = BASE_ID + RC_* / RI_* enum value (Rando/Types.h)
VERSION_2S2H = "5.0.1"

RI_BY_NAME = {v[0]: k for k, v in ITEMS.items()}
SKIP_BALANCING = {"RI_HEART_PIECE", "RI_HEART_CONTAINER", "RI_DEKU_NUT", "RI_DEKU_NUTS_5", "RI_DEKU_NUTS_10",
                  "RI_DEKU_STICK", "RI_DEKU_STICKS_5", "RI_MAGIC_BEAN", "RI_GS_TOKEN_SWAMP", "RI_GS_TOKEN_OCEAN",
                  "RI_WOODFALL_STRAY_FAIRY", "RI_SNOWHEAD_STRAY_FAIRY", "RI_GREAT_BAY_STRAY_FAIRY",
                  "RI_STONE_TOWER_STRAY_FAIRY", "RI_TRIFORCE_PIECE"}
# RCTYPE -> option that puts it into the check pool (GeneratePools.cpp)
TYPE_OPTION = {
    "RCTYPE_OWL": "RO_SHUFFLE_OWL_STATUES", "RCTYPE_POT": "RO_SHUFFLE_POT_DROPS", "RCTYPE_CRATE": "RO_SHUFFLE_CRATE_DROPS",
    "RCTYPE_BARREL": "RO_SHUFFLE_BARREL_DROPS", "RCTYPE_GRASS": "RO_SHUFFLE_GRASS_DROPS",
    "RCTYPE_BEEHIVE": "RO_SHUFFLE_HIVE_DROPS", "RCTYPE_TREE": "RO_SHUFFLE_TREE_DROPS",
    "RCTYPE_BUTTERFLY": "RO_SHUFFLE_BUTTERFLIES", "RCTYPE_FREESTANDING": "RO_SHUFFLE_FREESTANDING_ITEMS",
    "RCTYPE_SNOWBALL": "RO_SHUFFLE_SNOWBALL_DROPS", "RCTYPE_FROG": "RO_SHUFFLE_FROGS",
    "RCTYPE_REMAINS": "RO_SHUFFLE_BOSS_REMAINS", "RCTYPE_COW": "RO_SHUFFLE_COWS",
    "RCTYPE_ENEMY_DROP": "RO_SHUFFLE_ENEMY_DROPS", "RCTYPE_WONDER_ITEM": "RO_SHUFFLE_WONDER_ITEMS",
    "RCTYPE_TINGLE_SHOP": "RO_SHUFFLE_TINGLE_SHOPS", "RCTYPE_SKULL_TOKEN": "RO_SHUFFLE_GOLD_SKULLTULAS",
}
ALWAYS_SHUFFLED_SHOP = {"RC_CURIOSITY_SHOP_SPECIAL_ITEM", "RC_BOMB_SHOP_ITEM_03", "RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM"}
DUNGEON_SCENES = {"WOODFALL": {"SCENE_MITURIN", "SCENE_MITURIN_BS"}, "SNOWHEAD": {"SCENE_HAKUGIN", "SCENE_HAKUGIN_BS"},
                  "GREAT_BAY": {"SCENE_SEA", "SCENE_SEA_BS"},
                  "STONE_TOWER": {"SCENE_INISIE_N", "SCENE_INISIE_R", "SCENE_INISIE_BS"}}
JUNK_FILL = "RI_RUPEE_BLUE"
LOCATION_TYPE_GROUPS = {
    "RCTYPE_GRASS": "Grass", "RCTYPE_POT": "Pots", "RCTYPE_CRATE": "Crates", "RCTYPE_BARREL": "Barrels",
    "RCTYPE_SNOWBALL": "Snowballs", "RCTYPE_TREE": "Trees", "RCTYPE_BEEHIVE": "Beehives", "RCTYPE_CHEST": "Chests",
    "RCTYPE_FREESTANDING": "Freestanding Items", "RCTYPE_WONDER_ITEM": "Wonder Items", "RCTYPE_NPC": "NPC Rewards",
    "RCTYPE_SKULL_TOKEN": "Gold Skulltulas", "RCTYPE_ENEMY_DROP": "Enemy Drops", "RCTYPE_STRAY_FAIRY": "Stray Fairy Checks",
    "RCTYPE_SHOP": "Shops", "RCTYPE_HEART": "Heart Pieces and Containers", "RCTYPE_BUTTERFLY": "Butterflies",
    "RCTYPE_MINIGAME": "Minigames", "RCTYPE_TINGLE_SHOP": "Tingle", "RCTYPE_COW": "Cows", "RCTYPE_OWL": "Owl Statue Checks",
    "RCTYPE_SONG": "Song Checks", "RCTYPE_FROG": "Frogs", "RCTYPE_REMAINS": "Boss Remains Checks",
}
# Nearly No Logic placement blacklists (port of Logic/NearlyNoLogic.cpp)
_MOON = {"SCENE_LAST_DEKU", "SCENE_LAST_GORON", "SCENE_LAST_ZORA", "SCENE_LAST_LINK", "SCENE_SOUGEN", "SCENE_LAST_BS"}
NNL_SCENE_BLACKLIST = {
    "RI_MASK_DEKU": DUNGEON_SCENES["WOODFALL"] | _MOON, "RI_SONG_SONATA": DUNGEON_SCENES["WOODFALL"] | _MOON,
    "RI_MASK_ZORA": DUNGEON_SCENES["GREAT_BAY"] | _MOON, "RI_SONG_NOVA": DUNGEON_SCENES["GREAT_BAY"] | _MOON,
    "RI_REMAINS_ODOLWA": _MOON, "RI_REMAINS_GOHT": _MOON, "RI_REMAINS_GYORG": _MOON, "RI_REMAINS_TWINMOLD": _MOON,
    "RI_SONG_OATH": _MOON,
}
NNL_TYPE_BLACKLIST = {"RI_SONG_EPONA": "RCTYPE_COW"}
LOGIC_GLITCHLESS, LOGIC_NEARLY_NO_LOGIC = 0, 2


def _door_name(row: dict) -> str:
    """'WOODFALL_TEMPLE:0' -> 'Woodfall Temple'"""
    return row["into"].split(":")[0].replace("_", " ").title()


def region_name(rr: str) -> str:
    return "Menu" if rr == "RR_MAX" else " ".join(w.capitalize() for w in rr[3:].split("_"))


class MM2S2HItem(Item):
    game = GAME


class MM2S2HLocation(Location):
    game = GAME


class MM2S2HLogic(LogicMixin):
    mm2s2h_views: dict[int, SaveView]

    def init_mixin(self, multiworld) -> None:
        self.mm2s2h_views = {}

    def copy_mixin(self, new: CollectionState) -> CollectionState:
        new.mm2s2h_views = {}
        return new


class MM2S2HWeb(WebWorld):
    theme = "grass"
    tutorials = [Tutorial("Multiworld Setup Guide", "Connect 2 Ship 2 Harkinian to Archipelago.", "English",
                          "setup_en.md", "setup/en", ["junkx420"])]


class MM2S2HWorld(World):
    """The Legend of Zelda: Majora's Mask, played through the 2 Ship 2 Harkinian PC port, with every
    option of its own randomizer (grass, pots, crates, souls, clocks, ocarina buttons, ...)."""
    game = GAME
    web = MM2S2HWeb()
    options_dataclass = MM2S2HOptions
    options: MM2S2HOptions
    origin_region_name = "Menu"
    topology_present = True

    item_name_to_id: ClassVar[dict[str, int]] = {v[0]: BASE_ID + RI_ID[k] for k, v in ITEMS.items()
                                                 if k not in ("RI_UNKNOWN", "RI_NONE")}
    location_name_to_id: ClassVar[dict[str, int]] = {v[0]: BASE_ID + RC_ID[k] for k, v in LOCATIONS.items()}
    item_name_groups = {group: {v[0] for k, v in ITEMS.items() if test(k, v)} for group, test in {
        "Souls": lambda k, v: k.startswith("RI_SOUL_"),
        "Boss Souls": lambda k, v: k.startswith("RI_SOUL_BOSS_"),
        "Enemy Souls": lambda k, v: k.startswith("RI_SOUL_ENEMY_"),
        "NPC Souls": lambda k, v: k.startswith("RI_SOUL_NPC_"),
        "Clocks": lambda k, v: k.startswith("RI_TIME_"),
        "Masks": lambda k, v: v[1] == "RITYPE_MASK",
        "Songs": lambda k, v: k.startswith("RI_SONG_") or k == "RI_PROGRESSIVE_LULLABY",
        "Boss Remains": lambda k, v: k.startswith("RI_REMAINS_"),
        "Owl Statues": lambda k, v: k.startswith("RI_OWL_"),
        "Ocarina Buttons": lambda k, v: k.startswith("RI_OCARINA_BUTTON_"),
        "Small Keys": lambda k, v: v[1] == "RITYPE_SMALL_KEY",
        "Boss Keys": lambda k, v: v[1] == "RITYPE_BOSS_KEY",
        "Stray Fairies": lambda k, v: v[1] == "RITYPE_STRAY_FAIRY",
        "Tingle Maps": lambda k, v: k.startswith("RI_TINGLE_MAP_"),
    }.items()}
    location_name_groups = {
        **{group: {v[0] for v in LOCATIONS.values() if v[1] == rctype} for rctype, group in LOCATION_TYPE_GROUPS.items()},
        **{f"{d.replace('_', ' ').title()} Temple": {v[0] for v in LOCATIONS.values() if v[2] in scenes}
           for d, scenes in DUNGEON_SCENES.items()},
    }

    ro: dict[str, int]
    prices: dict[str, int]

    # ------------------------------------------------------------------------------------------
    def generate_early(self) -> None:
        self.ro = dict(OPTION_DEFAULTS)
        for field in self.options_dataclass.__dataclass_fields__:
            opt = getattr(self.options, field)
            if getattr(opt, "ro", ""):
                self.ro[opt.ro] = int(opt.value)
        if self.ro["RO_TRIFORCE_PIECES_REQUIRED"] > self.ro["RO_TRIFORCE_PIECES_MAX"]:
            self.ro["RO_TRIFORCE_PIECES_REQUIRED"] = self.ro["RO_TRIFORCE_PIECES_MAX"]
        if self.ro["RO_STRAY_FAIRIES_REQUIRED"] > self.ro["RO_STRAY_FAIRIES_MAX"]:
            self.ro["RO_STRAY_FAIRIES_REQUIRED"] = self.ro["RO_STRAY_FAIRIES_MAX"]
        self._time_cache: dict[int, TimeContext] = {}
        # prices: GeneratePools rolls Ship_Random(0, 200) for every non-tingle check (logic reads shops only)
        self.prices = {rc: self.random.randrange(0, 200) for rc, d in LOCATIONS.items()
                       if d[1] in ("RCTYPE_SHOP", "RCTYPE_TINGLE_SHOP") or rc == "RC_GORMAN_MILK_PURCHASE"
                       or rc.startswith("RC_MILK_BAR_PURCHASE")}
        # vanilla skulltulas (RO_SKULLTULA_SHUFFLED per Spider House)
        self.vanilla_skulltulas: set[str] = set()
        if self.ro["RO_SHUFFLE_GOLD_SKULLTULAS"]:
            by_scene = defaultdict(list)
            for rc, d in LOCATIONS.items():
                if d[1] == "RCTYPE_SKULL_TOKEN":
                    by_scene[d[2]].append(rc)
            n = self.ro["RO_SKULLTULA_SHUFFLED"]
            for rcs in by_scene.values():
                if n < len(rcs):
                    self.random.shuffle(rcs)
                    self.vanilla_skulltulas.update(rcs[n:])
        self.starting = self._starting_items()
        self.ut_entrance_pairs: list[tuple[int, int]] | None = None
        self.entrance_pairs: list[tuple[int, int]] = []

        # Universal Tracker: rebuild exactly the generated world from slot data (options, random rolls)
        passthrough = getattr(self.multiworld, "re_gen_passthrough", {}).get(self.game)
        if passthrough:
            rc_by_id = {v: k for k, v in RC_ID.items()}
            ri_by_id = {v: k for k, v in RI_ID.items()}
            self.ro = dict(OPTION_DEFAULTS) | {k: int(v) for k, v in passthrough["options"].items()}
            self.prices = {rc_by_id[int(k)]: int(v) for k, v in passthrough["prices"].items()}
            self.vanilla_skulltulas = {rc_by_id[int(i)] for i in passthrough["vanilla_skulltulas"]}
            self.starting = [ri_by_id[int(i)] for i in passthrough["starting_items"]]
            self.ut_entrance_pairs = [tuple(pair) for pair in passthrough.get("entrance_pairs", [])]

    # Universal Tracker
    ut_can_gen_without_yaml = True

    @staticmethod
    def interpret_slot_data(slot_data: dict[str, Any]) -> dict[str, Any]:
        return slot_data

    def _starting_items(self) -> list[str]:
        """Rando::GetStartingItemsFromSave + GetComputedStartingItems."""
        ro = self.ro
        items = [RI_BY_NAME[name] for name, cnt in self.options.starting_items.value.items() for _ in range(cnt)]
        if ro["RO_STARTING_MAPS_AND_COMPASSES"]:
            items += [f"RI_{d}_{k}" for d in ("GREAT_BAY", "SNOWHEAD", "STONE_TOWER", "WOODFALL") for k in ("COMPASS", "MAP")]
            items += [f"RI_TINGLE_MAP_{m}" for m in ("CLOCK_TOWN", "GREAT_BAY", "ROMANI_RANCH", "SNOWHEAD", "STONE_TOWER", "WOODFALL")]
        if ro["RO_PLACEMENT_SMALL_KEYS"] == 2:
            items += ["RI_WOODFALL_SMALL_KEY"] + ["RI_SNOWHEAD_SMALL_KEY"] * 3 + ["RI_GREAT_BAY_SMALL_KEY"] + ["RI_STONE_TOWER_SMALL_KEY"] * 4
        if ro["RO_PLACEMENT_BOSS_KEYS"] == 2:
            items += [f"RI_{d}_BOSS_KEY" for d in ("WOODFALL", "SNOWHEAD", "GREAT_BAY", "STONE_TOWER")]
        if ro["RO_PLACEMENT_STRAY_FAIRIES"] == 2:
            items += [f"RI_{d}_STRAY_FAIRY" for d in ("WOODFALL", "SNOWHEAD", "GREAT_BAY", "STONE_TOWER") for _ in range(15)]
        if not ro["RO_SHUFFLE_SWIM"]:
            items.append("RI_ABILITY_SWIM")
        if not ro["RO_SHUFFLE_ENEMY_SOULS"]:
            items += [k for k in RI_ID if k.startswith("RI_SOUL_ENEMY_")]
        if not ro["RO_SHUFFLE_OCARINA_BUTTONS"]:
            items += [k for k in RI_ID if k.startswith("RI_OCARINA_BUTTON_")]
        if not ro["RO_SHUFFLE_SONG_DOUBLE_TIME"]:
            items.append("RI_SONG_DOUBLE_TIME")
        if not ro["RO_SHUFFLE_SONG_INVERTED_TIME"]:
            items.append("RI_SONG_INVERTED_TIME")
        if ro["RO_CLOCK_SHUFFLE"] and not any(i.startswith("RI_TIME_") for i in items):
            if ro["RO_CLOCK_SHUFFLE_PROGRESSIVE"] == 0:
                items.append(["RI_TIME_DAY_1", "RI_TIME_NIGHT_1", "RI_TIME_DAY_2", "RI_TIME_NIGHT_2",
                              "RI_TIME_DAY_3", "RI_TIME_NIGHT_3"][self.random.randrange(6)])
            else:
                items.append("RI_TIME_PROGRESSIVE")
        return items

    def time_context(self, clock_flags: int) -> TimeContext:
        ctx = self._time_cache.get(clock_flags)
        if ctx is None:
            ctx = self._time_cache[clock_flags] = TimeContext(bool(self.ro["RO_CLOCK_SHUFFLE"]),
                                                              self.ro["RO_CLOCK_SHUFFLE_PROGRESSIVE"], clock_flags)
        return ctx

    # ------------------------------------------------------------------------------------------
    def is_shuffled(self, rc: str) -> bool:
        """GeneratePools.cpp: is this check in the check pool?"""
        _, rctype, scene, vanilla, _ = LOCATIONS[rc]
        if rc == "RC_UNKNOWN" or scene == "SCENE_LAST_BS":
            return False
        if rctype == "RCTYPE_SKULL_TOKEN" and (not self.ro["RO_SHUFFLE_GOLD_SKULLTULAS"] or rc in self.vanilla_skulltulas):
            return False
        opt = TYPE_OPTION.get(rctype)
        if opt and rctype != "RCTYPE_SKULL_TOKEN" and not self.ro[opt]:
            return False
        if rctype == "RCTYPE_SHOP" and not self.ro["RO_SHUFFLE_SHOPS"] and rc not in ALWAYS_SHUFFLED_SHOP:
            return False
        return True

    def view(self, state: CollectionState) -> SaveView:
        v = state.mm2s2h_views.get(self.player)
        if v is None:
            ri, ev = Counter(), Counter()
            for name, cnt in state.prog_items[self.player].items():
                k = RI_BY_NAME.get(name)
                if k is not None:
                    ri[k] = cnt
                elif name.startswith("RE_"):
                    ev[name] = cnt
            v = state.mm2s2h_views[self.player] = SaveView(ri, ev, self)
        return v

    def _rule(self, f):
        if f is None or self.ro["RO_LOGIC"] != LOGIC_GLITCHLESS:
            return None
        return lambda state: bool(f(self.view(state)))

    def _with_souls(self, rule, souls: list[str] | None):
        """`rule`, and every NPC soul in `souls` (item names). Like every access rule, only in Glitchless logic."""
        if not souls or self.ro["RO_LOGIC"] != LOGIC_GLITCHLESS:
            return rule
        p = self.player
        if rule is None:
            return lambda state: state.has_all(souls, p)
        return lambda state: state.has_all(souls, p) and rule(state)

    def create_regions(self) -> None:
        mw, p = self.multiworld, self.player
        # NPC souls: checks and events that need an NPC need its soul (Souls.cpp npcSouls)
        souls_for: dict[str, list[str]] = defaultdict(list)
        if self.ro["RO_SHUFFLE_NPC_SOULS"]:
            for ri, (_, checks, events) in NPC_SOULS.items():
                for key in checks + events:
                    souls_for[key].append(ITEMS[ri][0])
        regions = {rr: Region(region_name(rr), p, mw) for rr in REGIONS}
        mw.regions.extend(regions.values())
        self.regions_by_rr = regions
        self.event_count = 0
        # connections & exits; exits by (region, game entrance) for the entrance shuffle
        self.exit_entrances: dict[tuple[str, str], Entrance] = {}
        for rr, (_, _, _, conns, exits, events, _) in REGIONS.items():
            src = regions[rr]
            seen = Counter()
            for key, tgt, f in [(None, t, f) for t, f in conns] + exits:
                seen[tgt] += 1
                name = f"{src.name} -> {regions[tgt].name}" + (f" #{seen[tgt]}" if seen[tgt] > 1 else "")
                exit_ = src.connect(regions[tgt], name, self._rule(f))
                if key is not None:
                    self.exit_entrances[(rr, key)] = exit_
            for i, (re_id, f) in enumerate(events):
                loc = MM2S2HLocation(p, f"{re_id} @ {src.name} #{i}", None, src)
                loc.access_rule = self._with_souls(self._rule(f), souls_for.get(re_id)) or (lambda state: True)
                loc.place_locked_item(MM2S2HItem(re_id, IC.progression, None, p))
                src.locations.append(loc)
                self.event_count += 1
        # checks
        self.shuffled: list[str] = []
        self.locked_vanilla = 0
        self.confined = self._confined_items()
        self.dungeon_locations: dict[str, list[Location]] = {d: [] for d in DUNGEON_SCENES}
        rules_by_rc = defaultdict(list)
        for rr, (_, _, checks, *_rest) in REGIONS.items():
            for rc, f in checks:
                rules_by_rc[rc].append((rr, f))
        for rc, (name, rctype, scene, vanilla, regs) in LOCATIONS.items():
            shuffled = self.is_shuffled(rc)
            if not shuffled and vanilla not in LOGIC_ITEMS:
                continue  # behaves like vanilla, gives junk: nothing for logic to know
            entries = rules_by_rc[rc]
            if len(entries) == 1:
                parent = regions[entries[0][0]]
                rule = self._rule(entries[0][1])
            else:  # check listed in several regions: reachable from any of them
                parent = regions["RR_MAX"]
                parts = [(region_name(rr), f) for rr, f in entries]

                def rule(state, parts=parts):
                    return any(state.can_reach_region(rn, p) and (f is None or f(self.view(state))) for rn, f in parts)
            rule = self._with_souls(rule, souls_for.get(rc))
            loc = MM2S2HLocation(p, name, BASE_ID + RC_ID[rc] if shuffled else None, parent)
            if rule and self.ro["RO_LOGIC"] == LOGIC_GLITCHLESS:
                loc.access_rule = rule
            if shuffled:
                self.shuffled.append(rc)
                self._item_rule(loc, scene, rctype)
            else:
                vanilla_item = self.create_item(ITEMS[vanilla][0])
                vanilla_item.code = None  # locked vanilla item behaves like an event
                loc.place_locked_item(vanilla_item)
                self.locked_vanilla += 1
            parent.locations.append(loc)
        # goal
        lair = regions["RR_MOON_MAJORAS_LAIR"]
        goal = MM2S2HLocation(p, "Defeat Majora", None, lair)
        goal.place_locked_item(MM2S2HItem("Victory", IC.progression, None, p))
        # 2S2H's logic stops at the door (Moon.cpp: "TODO: Add a check for Game Completion?"), so on its own the
        # goal would be in logic without anything to fight with. A sword (any human sword, Great Fairy's Sword or
        # Fierce Deity) wins all three phases; the soul is needed in every logic mode.
        soul = (ITEMS["RI_SOUL_BOSS_MAJORA"][0]
                if self.ro["RO_SHUFFLE_BOSS_SOULS"] and not self.ro["RO_SHUFFLE_TRIFORCE_PIECES"] else None)
        fight = self._rule(CAN_USE_SWORD)
        if soul or fight:
            goal.access_rule = lambda state: (not soul or state.has(soul, p)) and (not fight or fight(state))
        lair.locations.append(goal)
        if self.ro["RO_SHUFFLE_TRIFORCE_PIECES"]:
            need = self.ro["RO_TRIFORCE_PIECES_REQUIRED"]
            mw.completion_condition[p] = lambda state: state.has(ITEMS["RI_TRIFORCE_PIECE"][0], p, need)
        else:
            mw.completion_condition[p] = lambda state: state.has("Victory", p)

    def _confined_items(self) -> dict[str, str]:
        """Item name -> dungeon, for every item that has to stay in its own dungeon (port of
        PlacementConstraints.cpp RandoItemIdToDungeon)."""
        confined = {}
        for d in DUNGEON_SCENES:
            for kind, ro in (("SMALL_KEY", "RO_PLACEMENT_SMALL_KEYS"), ("BOSS_KEY", "RO_PLACEMENT_BOSS_KEYS"),
                             ("STRAY_FAIRY", "RO_PLACEMENT_STRAY_FAIRIES")):
                if self.ro[ro] == 1:
                    confined[ITEMS[f"RI_{d}_{kind}"][0]] = d
        return confined

    def _item_rule(self, loc: Location, scene: str, rctype: str) -> None:
        """Placement constraints for this world's own items: own-dungeon items (PlacementConstraints.cpp) and the
        Nearly No Logic blacklists (NearlyNoLogic.cpp)."""
        dungeon = next((d for d, scenes in DUNGEON_SCENES.items() if scene in scenes), None)
        forbidden = set()
        if self.ro["RO_LOGIC"] == LOGIC_NEARLY_NO_LOGIC:
            forbidden = {ITEMS[ri][0] for ri, scenes in NNL_SCENE_BLACKLIST.items() if scene in scenes}
            forbidden |= {ITEMS[ri][0] for ri, t in NNL_TYPE_BLACKLIST.items() if t == rctype}
        if not self.confined and not forbidden:
            return
        if dungeon and self.confined:
            self.dungeon_locations[dungeon].append(loc)
        player, confined = self.player, self.confined
        loc.item_rule = lambda item: not (item.player == player and (item.name in forbidden
                                                                     or confined.get(item.name, dungeon) != dungeon))

    def pre_fill(self) -> None:
        """Place own-dungeon items before the main fill, like 2S2H's PreplaceConfinedItems, but logic-aware:
        every item goes to a location that is reachable with everything else in the pool."""
        if not self.confined:
            return
        mw = self.multiworld
        items = [item for item in mw.itempool if item.player == self.player and item.name in self.confined]
        if not items:
            return
        for item in items:
            mw.itempool.remove(item)
        locations = []
        for d, locs in self.dungeon_locations.items():
            free = [loc for loc in locs if loc.item is None]
            need = sum(self.confined[item.name] == d for item in items)
            if need > len(free):
                raise OptionError(f"{self.player_name}: {need} own-dungeon items for {len(free)} locations in "
                                  f"{d.replace('_', ' ').title()}; shuffle more check types or place them anywhere")
            locations += free
        state = mw.get_all_state()  # every other item of every world, without the confined ones
        self.random.shuffle(locations)
        self.random.shuffle(items)
        fill_restrictive(mw, state, locations, items, single_player_placement=True, lock=True,
                         allow_excluded=True, name="MM2S2H own-dungeon items")

    # ------------------------------------------------------------------------------------------
    def create_item(self, name: str) -> MM2S2HItem:
        ri = RI_BY_NAME[name]
        if ri in LOGIC_ITEMS:
            cls = IC.progression_skip_balancing if ri in SKIP_BALANCING else IC.progression
        elif ri == "RI_TRAP":
            cls = IC.trap
        elif ITEMS[ri][1] in ("RITYPE_MAJOR", "RITYPE_MASK", "RITYPE_LESSER"):
            cls = IC.useful
        else:
            cls = IC.filler
        return MM2S2HItem(name, cls, self.item_name_to_id[name], self.player)

    def get_filler_item_name(self) -> str:
        return ITEMS[JUNK_FILL][0]

    def create_items(self) -> None:
        ro = self.ro
        pool = [LOCATIONS[rc][3] for rc in self.shuffled]
        pool += ["RI_PROGRESSIVE_SWORD", "RI_SHIELD_HERO"]
        if ro["RO_SHUFFLE_BOSS_SOULS"]:
            pool += [k for k in RI_ID if k.startswith("RI_SOUL_BOSS_")
                     and not (k == "RI_SOUL_BOSS_MAJORA" and ro["RO_SHUFFLE_TRIFORCE_PIECES"])]
        if ro["RO_SHUFFLE_ENEMY_SOULS"]:
            pool += [k for k in RI_ID if k.startswith("RI_SOUL_ENEMY_")]
        if ro["RO_SHUFFLE_NPC_SOULS"]:  # not in 2S2H's own pools: an addition of this fork
            pool += list(NPC_SOULS)
        if ro["RO_CLOCK_SHUFFLE"]:
            pool += (["RI_TIME_DAY_1", "RI_TIME_NIGHT_1", "RI_TIME_DAY_2", "RI_TIME_NIGHT_2", "RI_TIME_DAY_3", "RI_TIME_NIGHT_3"]
                     if ro["RO_CLOCK_SHUFFLE_PROGRESSIVE"] == 0 else ["RI_TIME_PROGRESSIVE"] * 6)
        if ro["RO_SHUFFLE_SWIM"]:
            pool.append("RI_ABILITY_SWIM")
        if ro["RO_SHUFFLE_OCARINA_BUTTONS"]:
            pool += [k for k in RI_ID if k.startswith("RI_OCARINA_BUTTON_")]
        for opt, ri in (("RO_SHUFFLE_SONG_SUN", "RI_SONG_SUN"), ("RO_SHUFFLE_SONG_DOUBLE_TIME", "RI_SONG_DOUBLE_TIME"),
                        ("RO_SHUFFLE_SONG_INVERTED_TIME", "RI_SONG_INVERTED_TIME"), ("RO_SHUFFLE_SONG_SARIA", "RI_SONG_SARIA"),
                        ("RO_SHUFFLE_TYCOON_WALLET", "RI_PROGRESSIVE_WALLET")):
            if ro[opt]:
                pool.append(ri)
        if ro["RO_SHUFFLE_TRIFORCE_PIECES"]:
            pool += ["RI_TRIFORCE_PIECE"] * ro["RO_TRIFORCE_PIECES_MAX"]
        if ro["RO_SHUFFLE_SKELETON_KEY"] and ro["RO_PLACEMENT_SMALL_KEYS"] != 2:
            pool.append("RI_SKELETON_KEY")
        for d in ("STONE_TOWER", "GREAT_BAY", "SNOWHEAD", "WOODFALL"):
            ri = f"RI_{d}_STRAY_FAIRY"
            extra = pool.count(ri) - ro["RO_STRAY_FAIRIES_MAX"]
            for _ in range(max(0, extra)):
                pool.remove(ri)
        for ri in self.starting:
            if ri in pool:
                pool.remove(ri)
            self.multiworld.push_precollected(self.create_item(ITEMS[ri][0]))
        hearts = ro["RO_STARTING_HEALTH"]
        if hearts < 3:
            pool += ["RI_HEART_PIECE"] * (4 * (3 - hearts))
        elif hearts > 3:
            for _ in range(4 * (hearts - 3)):
                if "RI_HEART_PIECE" not in pool:
                    break
                pool.remove("RI_HEART_PIECE")
        if ro["RO_PLENTIFUL_ITEMS"]:
            pool += [ri for ri in pool if ri not in ("RI_TRIFORCE_PIECE", "RI_SKELETON_KEY") and (
                ITEMS[ri][1] in ("RITYPE_BOSS_KEY", "RITYPE_SMALL_KEY", "RITYPE_MASK", "RITYPE_MAJOR")
                or (ITEMS[ri][1] == "RITYPE_LESSER" and ITEMS[ri][2] not in ("ITEM_TINGLE_MAP", "ITEM_DUNGEON_MAP", "ITEM_COMPASS")))]
        if ro["RO_SHUFFLE_TRAPS"]:
            pool += ["RI_TRAP"] * ro["RO_TRAP_AMOUNT"]
        pool = [ri for ri in pool if ri not in ("RI_UNKNOWN", "RI_NONE")]
        # balance against the number of locations (2S2H fills with junk the same way)
        n_locs = len(self.shuffled)
        if len(pool) > n_locs:
            junk = [i for i, ri in enumerate(pool) if ITEMS[ri][1] == "RITYPE_JUNK" or ri == "RI_JUNK"]
            for i in reversed(junk[:len(pool) - n_locs]):
                pool.pop(i)
            if len(pool) > n_locs:
                raise OptionError(f"{self.player_name}: {len(pool)} items for {n_locs} locations; "
                                  "enable more check types or fewer item-adding options")
        pool += [JUNK_FILL] * (n_locs - len(pool))
        self.multiworld.itempool += [self.create_item(ITEMS[ri][0]) for ri in pool]

    # ------------------------------------------------------------------------------------------
    def collect(self, state: CollectionState, item: Item) -> bool:
        change = super().collect(state, item)
        if change:
            state.mm2s2h_views.pop(self.player, None)
        return change

    def remove(self, state: CollectionState, item: Item) -> bool:
        change = super().remove(state, item)
        if change:
            state.mm2s2h_views.pop(self.player, None)
        return change

    # ------------------------------------------------------------------------------------------
    def connect_entrances(self) -> None:
        """Entrance shuffle (EntranceShuffle.cpp rows whose option is on): coupled, so a door's way back is the exit
        of the area behind it. An exit keeps its own rule, i.e. what it takes to use the door stays at the door."""
        self.entrance_pairs: list[tuple[int, int]] = []  # [door's "into", "into" of the area behind it] (slot data)
        rows = [row for row in ENTRANCE_SHUFFLE if self.ro.get(row["option"])]
        if not rows:
            return
        doors = [self.exit_entrances[(row["door_region"], row["into"])] for row in rows]
        insides = [self.exit_entrances[(row["inside_region"], row["out"])] for row in rows]
        row_of_inside = {row["inside_region"]: row for row in rows}
        if self.ut_entrance_pairs is not None:  # Universal Tracker: the pairing of the generated seed
            by_value = {row["into_value"]: i for i, row in enumerate(rows)}
            behind = {by_value[door]: by_value[inside] for door, inside in self.ut_entrance_pairs}
            for i, row in enumerate(rows):
                j = behind.get(i, i)
                self._reconnect(doors[i], rows[j]["inside_region"])
                self._reconnect(insides[j], row["door_region"])
        else:
            groups = sorted({row["option"] for row in rows})
            lookup = {}
            for row, door, inside in zip(rows, doors, insides):
                g = 2 * groups.index(row["option"]) + 1
                door.randomization_type = inside.randomization_type = EntranceType.TWO_WAY
                door.randomization_group, inside.randomization_group = g, g + 1
                lookup[g], lookup[g + 1] = [g + 1], [g]
            for exit_ in doors + insides:
                disconnect_entrance_for_randomization(exit_)
            randomize_entrances(self, True, lookup, exits=doors + insides)
        rr_of = {region.name: rr for rr, region in self.regions_by_rr.items()}
        for row, door in zip(rows, doors):
            inside = row_of_inside[rr_of[door.connected_region.name]]
            self.entrance_pairs.append((row["into_value"], inside["into_value"]))
            self.multiworld.spoiler.set_entrance(_door_name(row), _door_name(inside), "both", self.player)
            # the boss of the area behind this door warps back out of this door (EntranceShuffle.cpp), and the door
            # also needs what the vanilla way to that area's own door guarantees (shuffleAreaRules)
            if inside is not row:
                area = self._rule(ENTRANCE_AREA_RULES.get(inside["into"]))
                if area is not None:
                    own = door.access_rule
                    door.access_rule = lambda state, own=own, area=area: own(state) and area(state)
                for boss_rr, warp in inside["boss_warps"]:
                    self._reconnect(self.exit_entrances[(boss_rr, warp)], row["door_region"])

    def _reconnect(self, exit_: Entrance, rr: str) -> None:
        if exit_.connected_region is not None:
            exit_.connected_region.entrances.remove(exit_)
            exit_.connected_region = None
        exit_.connect(self.regions_by_rr[rr])

    def fill_slot_data(self) -> dict[str, Any]:
        return {
            "version_2s2h": VERSION_2S2H,
            "apworld_version": self.world_version.as_simple_string(),
            "base_id": BASE_ID,
            "options": self.ro,  # RO_* -> value, written into randoSaveOptions by the client
            "prices": {RC_ID[rc]: price for rc, price in self.prices.items()},
            "starting_items": [RI_ID[ri] for ri in self.starting],
            "vanilla_skulltulas": sorted(RC_ID[rc] for rc in self.vanilla_skulltulas),
            "entrance_pairs": [list(pair) for pair in self.entrance_pairs],
            "death_link": bool(self.options.death_link.value),
            "seed": self.random.getrandbits(32),
        }
