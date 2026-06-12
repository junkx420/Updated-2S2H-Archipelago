#!/usr/bin/env python3
"""Generate the AP <-> 2S2H ID mapping tables.

Joins the mm_recomp apworld's location/item tables (name -> AP id) against
2S2H's native Rando static data (RandoCheckId / RandoItemId) and emits
ArchipelagoMapping.cpp plus a gap report for everything that could not be
matched automatically.

Usage (from the repo root):
    python mm/2s2h/Network/Archipelago/tools/gen_ap_mapping.py \
        --apworld build/apworld_extract/mm_recomp \
        [--out mm/2s2h/Network/Archipelago/ArchipelagoMapping.cpp] \
        [--report ap_mapping_report.md]

mm_recomp AP location ids embed structured data (verified empirically):
    address = 0x3469420000000 | field, where field is one of
      0x00_GGG  : vanilla GetItem id GGG (NPC/inventory rewards)
      0x05_SSFF : freestanding collectible, scene SS, collectible flag FF
      0x06_SSFF : chest, scene SS, chest flag FF
      others    : stray fairies (0x01), skulls (0x02), great fairies (0x03),
                  weekeventreg-ish (0x04/0x07/0x08), shops (0x09), hearts (0xD)

2S2H's Checks.cpp carries (scene, flagType, flag, vanillaItem) per check, so
chests/collectibles/getitems can be joined STRUCTURALLY. Everything else
falls back to name matching, then to the override tables below.

Matching layers, in order:
  1. manual override tables in this file (always win)
  2. structural: chest / collectible via (scene, flag); getitem via the
     check's vanilla item GI id (only when unique on both sides)
  3. exact match on normalized + region-aliased names
  4. token-set match (word order independent)
Unmatched entries land in the report with their top Jaccard candidates.
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[5]
CHECKS_CPP = REPO_ROOT / "mm/2s2h/Rando/StaticData/Checks.cpp"
ITEMS_CPP = REPO_ROOT / "mm/2s2h/Rando/StaticData/Items.cpp"
SCENE_TABLE_H = REPO_ROOT / "mm/include/tables/scene_table.h"
Z64ITEM_H = REPO_ROOT / "mm/include/z64item.h"

AP_BASE = 0x3469420000000

# ---------------------------------------------------------------------------
# Manual overrides: AP name -> 2S2H enum. Curated by hand; these always win.
# Map an AP name to None to explicitly mark it "no 2S2H equivalent (yet)" so
# it stops showing up in the report.
# ---------------------------------------------------------------------------
LOCATION_OVERRIDES: dict = {
    # --- no 2S2H equivalent (yet): starting inventory slots, events, recomp
    # --- extras. None = intentionally unmapped.
    "Link's Inventory (Kokiri Sword)": None,
    "Link's Inventory (Hero's Shield)": None,
    "Link's Inventory (Heart Item #1)": None,
    "Link's Inventory (Heart Item #2)": None,
    "Link's Inventory (Heart Item #3)": None,
    "Link's Inventory (Heart Item #4)": None,
    "Link's Inventory (Heart Item #5)": None,
    "Link's Inventory (Heart Item #6)": None,
    "Link's Inventory (Heart Item #7)": None,
    "Link's Inventory (Heart Item #8)": None,
    "Before Clock Town Chest": None,  # intro area chest, no 2S2H check
    "Defeat Majora": None,            # goal event
    "Clock Town Trading Post Shop (Night) Item 1": None,  # 2S2H has no night shop slots
    "Clock Town Trading Post Shop (Night) Item 2": None,
    "Clock Town Trading Post Shop (Night) Item 3": None,
    "Clock Town Trading Post Shop (Night) Item 4": None,
    "Clock Town Trading Post Shop (Night) Item 5": None,
    "Clock Town Trading Post Shop (Night) Item 6": None,
    "Clock Town Trading Post Shop (Night) Item 7": None,
    "Clock Town Trading Post Shop (Night) Item 8": None,
    "Goron Village Shop (Spring) Item 1": None,  # 2S2H has no spring shop slots
    "Goron Village Shop (Spring) Item 2": None,
    "Goron Village Shop (Spring) Item 3": None,
    "Clock Town Bomb Shop Item 3 (Stop Thief)": None,  # merged into _04_OR_CURIOSITY below
    "Curiosity Shop Blue Rupee Trade": None,    # sell-item trades don't exist in 2S2H
    "Curiosity Shop Red Rupee Trade": None,
    "Curiosity Shop Purple Rupee Trade": None,
    "Curiosity Shop Gold Rupee Trade": None,

    # --- shops (slot numbering verified against vanilla stock on both sides)
    "Clock Town Trading Post Shop Item 1": "RC_TRADING_POST_SHOP_ITEM_01",
    "Clock Town Trading Post Shop Item 2": "RC_TRADING_POST_SHOP_ITEM_02",
    "Clock Town Trading Post Shop Item 3": "RC_TRADING_POST_SHOP_ITEM_03",
    "Clock Town Trading Post Shop Item 4": "RC_TRADING_POST_SHOP_ITEM_04",
    "Clock Town Trading Post Shop Item 5": "RC_TRADING_POST_SHOP_ITEM_05",
    "Clock Town Trading Post Shop Item 6": "RC_TRADING_POST_SHOP_ITEM_06",
    "Clock Town Trading Post Shop Item 7": "RC_TRADING_POST_SHOP_ITEM_07",
    "Clock Town Trading Post Shop Item 8": "RC_TRADING_POST_SHOP_ITEM_08",
    "Clock Town Bomb Shop Item 1": "RC_BOMB_SHOP_ITEM_01",
    "Clock Town Bomb Shop Item 2": "RC_BOMB_SHOP_ITEM_02",
    "Clock Town Bomb Shop Item 3": "RC_BOMB_SHOP_ITEM_03",
    "Curiosity Shop Night 3 Thief Stolen Item": "RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM",
    "Curiosity Shop Night 3 (Stop Thief)": "RC_CURIOSITY_SHOP_SPECIAL_ITEM",
    "Southern Swamp Witch Shop Item 1": "RC_HAGS_POTION_SHOP_ITEM_01",  # blue
    "Southern Swamp Witch Shop Item 2": "RC_HAGS_POTION_SHOP_ITEM_02",  # green
    "Southern Swamp Witch Shop Item 3": "RC_HAGS_POTION_SHOP_ITEM_03",  # red
    "Goron Village Shop Item 1": "RC_GORON_SHOP_ITEM_01",
    "Goron Village Shop Item 2": "RC_GORON_SHOP_ITEM_02",
    "Goron Village Shop Item 3": "RC_GORON_SHOP_ITEM_03",
    "Zora Hall Shop Item 1": "RC_ZORA_SHOP_ITEM_01",
    "Zora Hall Shop Item 2": "RC_ZORA_SHOP_ITEM_02",
    "Zora Hall Shop Item 3": "RC_ZORA_SHOP_ITEM_03",

    # --- scrub purchases (vanilla stock verified)
    "Southern Swamp Deku Scrub Purchase": "RC_SOUTHERN_SWAMP_SCRUB_BEANS",
    "Goron Village Scrub Purchase": "RC_GORON_VILLAGE_SCRUB_BOMB_BAG",
    "Zora Hall Deku Scrub Purchase": "RC_ZORA_HALL_SCRUB_POTION_REFILL",
    "Ikana Canyon Scrub Purchase": "RC_IKANA_CANYON_SCRUB_POTION_REFILL",  # blue potion purchase
    "Ikana Canyon Zora Scrub Trade": "RC_IKANA_CANYON_SCRUB_HUGE_RUPEE",   # ocean deed trade

    # --- tingle maps: AP is per-map, 2S2H is per-vendor-slot; mapped to the
    # --- region-local vendor slot. Buying the same map from the alternate
    # --- vendor needs game-side aliasing later.
    "Tingle Clock Town Map Purchase": "RC_CLOCK_TOWN_NORTH_TINGLE_MAP_01",
    "Tingle Woodfall Map Purchase": "RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01",
    "Tingle Snowhead Map Purchase": "RC_TWIN_ISLANDS_TINGLE_MAP_01",
    "Tingle Romani Ranch Map Purchase": "RC_MILK_ROAD_TINGLE_MAP_01",
    "Tingle Great Bay Map Purchase": "RC_GREAT_BAY_COAST_TINGLE_MAP_01",
    "Tingle Stone Tower Map Purchase": "RC_IKANA_CANYON_TINGLE_MAP_01",

    # --- clock town
    "Clock Tower Happy Mask Salesman #1": "RC_STARTING_ITEM_SONG_OF_HEALING",
    "Clock Tower Happy Mask Salesman #2": "RC_STARTING_ITEM_DEKU_MASK",
    "Clock Town Hide-and-Seek": "RC_CLOCK_TOWN_BOMBERS_NOTEBOOK",
    "Laundry Pool Stray Fairy (Clock Town)": "RC_CLOCK_TOWN_STRAY_FAIRY",
    "South Clock Town Moon's Tear Trade": "RC_CLOCK_TOWN_SCRUB_DEED",
    "East Clock Town Shooting Gallery 40-49 Points": "RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_HIGH_SCORE",
    "East Clock Town Shooting Gallery Perfect 50 Points": "RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_PERFECT_SCORE",
    "East Clock Town Honey and Darling Any Day": "RC_CLOCK_TOWN_EAST_HONEY_DARLING_ANY_DAY",
    "East Clock Town Honey and Darling All Days": "RC_CLOCK_TOWN_EAST_HONEY_DARLING_ALL_DAYS",
    "North Clock Town Deku Playground Any Day": "RC_DEKU_PLAYGROUND_ANY_DAY",
    "North Clock Town Deku Playground All Days": "RC_DEKU_PLAYGROUND_ALL_DAYS",
    "North Clock Town Great Fairy Reward": "RC_CLOCK_TOWN_GREAT_FAIRY",
    "North Clock Town Great Fairy Reward (Has Transformation Mask)": "RC_CLOCK_TOWN_GREAT_FAIRY_ALT",
    "West Clock Town Lottery Any Day": "RC_CLOCK_TOWN_WEST_LOTTERY",
    "West Clock Town Swordsman Expert Course": "RC_SWORDSMAN_SCHOOL_PIECE_OF_HEART",
    "West Clock Town Postman Counting": "RC_CLOCK_TOWN_WEST_POSTMAN_MINIGAME",
    "West Clock Town Dancing Sisters": "RC_CLOCK_TOWN_WEST_SISTERS_PIECE_OF_HEART",
    "West Clock Town Bank 200 Rupees": "RC_CLOCK_TOWN_WEST_BANK_ADULTS_WALLET",
    "West Clock Town Bank 500 Rupees": "RC_CLOCK_TOWN_WEST_BANK_INTEREST",
    "West Clock Town Bank 1000 Rupees": "RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART",
    "Stock Pot Inn Midnight Toilet Hand": "RC_STOCK_POT_INN_TOILET_HAND",
    "Stock Pot Inn Granny Story #1": "RC_STOCK_POT_INN_GRANDMA_LONG_STORY",   # VERIFY order
    "Stock Pot Inn Granny Story #2": "RC_STOCK_POT_INN_GRANDMA_SHORT_STORY",  # VERIFY order
    "Milk Bar Priority Mail to Aroma": "RC_MILK_BAR_MADAME_AROMA",
    "East Clock Town Milk Bar Milk Purchase": "RC_MILK_BAR_PURCHASE_MILK",
    "East Clock Town Milk Bar Chateau Romani Purchase": "RC_MILK_BAR_PURCHASE_CHATEAU",
    "Bomber's Hideout Astral Observatory": "RC_ASTRAL_OBSERVATORY_MOON_TEAR",

    # --- termina field / swamp
    "Termina Gossip Stones HP": "RC_TERMINA_FIELD_GOSSIP_STONE_GROTTO",
    "Termina Scrub Grotto HP": "RC_TERMINA_FIELD_GROTTO_SCRUB",
    "Termina Log Bombable Grotto Left Cow": "RC_TERMINA_FIELD_COW_FRONT",   # VERIFY left/right
    "Termina Log Bombable Grotto Right Cow": "RC_TERMINA_FIELD_COW_BACK",   # VERIFY left/right
    "Swamp Shooting Gallery 2120 Points": "RC_SWAMP_SHOOTING_GALLERY_HIGH_SCORE",
    "Swamp Shooting Gallery 2180 Points": "RC_SWAMP_SHOOTING_GALLERY_PERFECT_SCORE",
    "Southern Swamp Song Tablet": "RC_SOUTHERN_SWAMP_SONG_OF_SOARING",
    "Southern Swamp Boat Archery": "RC_TOURIST_INFORMATION_ARCHERY",
    "Woodfall Great Fairy Reward": "RC_WOODFALL_GREAT_FAIRY",
    "Woodfall Temple Heart Container": "RC_WOODFALL_TEMPLE_BOSS_CONTAINER",

    # --- mountain
    "Snowhead Great Fairy Reward": "RC_SNOWHEAD_GREAT_FAIRY",
    "Snowhead Temple Heart Container": "RC_SNOWHEAD_TEMPLE_BOSS_HEART_CONTAINER",
    "Mountain Village Spring Waterfall Chest": "RC_MOUNTAIN_VILLAGE_WATERFALL_CHEST",
    "Mountain Village Smithy Upgrade": "RC_MOUNTAIN_VILLAGE_SMITHY_RAZOR_SWORD",
    "Mountain Village Smithy Gold Dust Upgrade": "RC_MOUNTAIN_VILLAGE_SMITHY_GILDED_SWORD",
    "Mountain Village Spring Frog Choir HP": "RC_MOUNTAIN_VILLAGE_FROG_CHOIR",
    "Goron Village Baby Goron Lullaby": "RC_GORON_SHRINE_FULL_LULLABY",
    "Twin Islands Goron Elder Request": "RC_PATH_TO_GORON_VILLAGE_LULLABY_INTRO",
    "Twin Islands Spring Underwater Cave Chest": "RC_TWIN_ISLANDS_UNDERWATER_CHEST_01",  # VERIFY
    "Twin Islands Spring Underwater Ramp Chest": "RC_TWIN_ISLANDS_UNDERWATER_CHEST_02",  # VERIFY

    # --- ranch / great bay
    "Milk Road Gorman Ranch Purchase": "RC_GORMAN_MILK_PURCHASE",
    "Romani Ranch Doggy Race": "RC_DOGGY_RACETRACK_PIECE_OF_HEART",
    "Romani Ranch Romani Game": "RC_ROMANI_RANCH_EPONAS_SONG",  # balloon practice -> Epona's Song
    "Romani Ranch Barn Free Cow": "RC_ROMANI_RANCH_BARN_COW_LEFT",          # VERIFY
    "Romani Ranch Barn Stables Front Cow": "RC_ROMANI_RANCH_BARN_COW_MIDDLE",  # VERIFY
    "Romani Ranch Barn Stables Back Cow": "RC_ROMANI_RANCH_BARN_COW_RIGHT",    # VERIFY
    "Great Bay Great Fairy Reward": "RC_GREAT_BAY_GREAT_FAIRY",
    "Great Bay Feeding Lab Fish": "RC_GREAT_BAY_COAST_MARINE_LAB_FISH_PIECE_OF_HEART",
    "Great Bay Fisherman Game": "RC_GREAT_BAY_COAST_FISHERMAN_MINIGAME",
    "Great Bay Ledge Grotto Left Cow": "RC_GREAT_BAY_COAST_COW_FRONT",      # VERIFY
    "Great Bay Ledge Grotto Right Cow": "RC_GREAT_BAY_COAST_COW_BACK",      # VERIFY
    "Beaver Bros. Race 1": "RC_WATERFALL_RAPIDS_BEAVER_RACE_01",
    "Beaver Bros. Race 2 HP": "RC_WATERFALL_RAPIDS_BEAVER_RACE_02",
    "Zora Hall Evan's Song": "RC_ZORA_HALL_EVANS_PIECE_OF_HEART",
    "Zora Hall Torches Reward": "RC_ZORA_HALL_SCENE_LIGHTS",
    "Pinnacle Rock Seahorse HP": "RC_PINNACLE_ROCK_REUNITE_SEAHORSE",
    "Ocean Spider House Reward": "RC_OCEAN_SPIDER_HOUSE_WALLET",
    "Great Bay Temple Heart Container": "RC_GREAT_BAY_TEMPLE_BOSS_HEART_CONTAINER",

    # --- ikana / moon
    "Ikana Well Cow": "RC_BENEATH_THE_WELL_COW",
    "Graveyard Day 1 Iron Knuckle Song": "RC_BENEATH_THE_GRAVEYARD_SONG_OF_STORMS",
    # ("Graveyard Day 2 Iron Knuckle Chest" structurally matches the HP chest;
    #  "Graveyard Day 2 Dampe Bats" is left unmatched pending investigation)
    "Stone Tower Great Fairy Reward": "RC_IKANA_GREAT_FAIRY",
    "Stone Tower Temple Inverted Entrance Room Sun Face Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_ENTRANCE_CHEST",
    "Stone Tower Temple Inverted Death Armos Maze Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_DEATH_ARMOS_CHEST",
    "Stone Tower Temple Inverted Heart Container": "RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_HEART_CONTAINER",
    "Moon Trade All Masks": "RC_MOON_FIERCE_DEITY_MASK",

    # --- songs (no GetItem id in 2S2H's item table, so the GI join can't see them)
    "Top of Clock Tower (Song of Time)": "RC_CLOCK_TOWER_ROOF_SONG_OF_TIME",
    "Deku Palace Monkey Song": "RC_DEKU_KINGS_CHAMBER_MONKEY",
    "Ikana Castle King Song": "RC_ANCIENT_CASTLE_OF_IKANA_BOSS",
    "Oath to Order": "RC_GIANTS_CHAMBER_OATH_TO_ORDER",

    # --- misc NPC rewards
    "East Clock Town Couples Mask on Mayor": "RC_MAYORS_OFFICE_PIECE_OF_HEART",
    "Mountain Village Hungry Goron": "RC_MOUNTAIN_VILLAGE_DON_GERO_MASK",
    "Great Bay Baby Zora Song": "RC_GREAT_BAY_COAST_NEW_WAVE_BOSSA_NOVA",
    "Goron Racetrack Prize": "RC_GORON_RACETRACK_GOLD_DUST",
    "Southern Swamp Winning Picture": "RC_TOURIST_INFORMATION_GOOD_PHOTO",
    "Southern Swamp Good Picture": None,   # rupee tiers, no 2S2H checks
    "Southern Swamp Okay Picture": None,

    # --- Stone Tower Temple inverted: normal+inverted share SCENE_INISIE_N
    # --- flag space, so the structural join is ambiguous; flags verified
    # --- against Checks.cpp by hand
    "Stone Tower Temple Inverted Eastern Air Gust Room Fire Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_EAST_UPPER_CHEST",
    "Stone Tower Temple Inverted Eastern Air Gust Room Frozen Switch Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_EAST_LOWER_CHEST",
    "Stone Tower Temple Inverted Eastern Air Gust Room Switch Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_EAST_MIDDLE_CHEST",
    "Stone Tower Temple Inverted Gomess Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_KEY",
    "Stone Tower Temple Inverted Eyegore Chest": "RC_STONE_TOWER_TEMPLE_INVERTED_GIANT_MASK",
    "Stone Tower Inverted Middle Chest": "RC_STONE_TOWER_INVERTED_CHEST_02",  # flag 0x1E
    "Stone Tower Inverted Right Chest": "RC_STONE_TOWER_INVERTED_CHEST_01",   # flag 0x1D
    "Stone Tower Inverted Left Chest": "RC_STONE_TOWER_INVERTED_CHEST_03",    # flag 0x1F
}

ITEM_OVERRIDES: dict = {
    "Swamp Skulltula Token": "RI_GS_TOKEN_SWAMP",
    "Ocean Skulltula Token": "RI_GS_TOKEN_OCEAN",
    "Fierce Deity's Mask": "RI_MASK_FIERCE_DEITY",
    "Garo Mask": "RI_MASK_GARO",
    "Romani Mask": "RI_MASK_ROMANI",
    "Kamaro Mask": "RI_MASK_KAMARO",
    "Priority Mail": "RI_LETTER_TO_MAMA",
    "Bottle": "RI_BOTTLE_EMPTY",
    "Fire Arrow": "RI_ARROW_FIRE",
    "Ice Arrow": "RI_ARROW_ICE",
    "Light Arrow": "RI_ARROW_LIGHT",
    "Progressive Shield": "RI_SHIELD_HERO",      # TODO: 2S2H has no progressive shield
    "Bottle of Red Potion": "RI_BOTTLE_RED_POTION",
    # Regional (Tingle) maps: without overrides these name-match the DUNGEON
    # maps (RI_WOODFALL_MAP etc.), which are different items.
    "Clock Town Map": "RI_TINGLE_MAP_CLOCK_TOWN",
    "Woodfall Map": "RI_TINGLE_MAP_WOODFALL",
    "Snowhead Map": "RI_TINGLE_MAP_SNOWHEAD",
    "Great Bay Map": "RI_TINGLE_MAP_GREAT_BAY",
    "Stone Tower Map": "RI_TINGLE_MAP_STONE_TOWER",
    "Romani Ranch Map": "RI_TINGLE_MAP_ROMANI_RANCH",
    "Dungeon Map (Woodfall)": "RI_WOODFALL_MAP",
    "Dungeon Map (Snowhead)": "RI_SNOWHEAD_MAP",
    "Dungeon Map (Great Bay)": "RI_GREAT_BAY_MAP",
    "Dungeon Map (Stone Tower)": "RI_STONE_TOWER_MAP",
    "Bundle of 30 Arrows": "RI_ARROWS_30",
    "Progressive Bombchu Bag": "RI_BOMBCHU_10",  # TODO: 2S2H has no bombchu bag concept
    "Bombchu (1)": "RI_BOMBCHU",
    "Gold Rupee": "RI_RUPEE_HUGE",
}

# Region/phrase aliases applied to BOTH sides before comparison; each pair is
# (pattern, canonical). Applied in order on the lowercased string.
PHRASE_ALIASES = [
    # AP puts the compass direction first, 2S2H puts it last
    ("south clock town", "clock town south"),
    ("north clock town", "clock town north"),
    ("east clock town", "clock town east"),
    ("west clock town", "clock town west"),
    ("laundry pool", "clock town laundry"),
    # common vocabulary differences
    ("freestanding hp", "piece of heart"),
    ("’", "'"),
    ("hide-and-seek", "bombers notebook"),
    ("piece of heart", "heartpiece"),
    ("heart piece", "heartpiece"),
    (" hp", " heartpiece"),
    ("heart container", "heartcontainer"),
    ("stray fairy", "strayfairy"),
    ("great fairy", "greatfairy"),
    ("skulltulla", "skulltula"),
    ("archery", "shooting gallery"),
]

STOP_TOKENS = {"the", "a", "an", "of", "s", "to", "and"}


def normalize(name: str) -> str:
    s = name.lower().replace("'", "")
    for phrase, canon in PHRASE_ALIASES:
        s = s.replace(phrase, canon)
    s = re.sub(r"[^a-z0-9]+", " ", s)
    return re.sub(r"\s+", " ", s).strip()


def token_key(name: str) -> frozenset:
    return frozenset(t for t in normalize(name).split() if t not in STOP_TOKENS)


def jaccard(a: frozenset, b: frozenset) -> float:
    if not a or not b:
        return 0.0
    return len(a & b) / len(a | b)


def enum_to_name(enum: str, prefix: str) -> str:
    return enum[len(prefix):].replace("_", " ").lower()


# ---------------------------------------------------------------------------
# Parsers
# ---------------------------------------------------------------------------
def strip_comment_lines(text: str) -> str:
    """Drop full-line comments (e.g. the apworld's '# ~ "Blue Potion": ...'
    disabled entries) so the entry regex can't latch onto them and swallow the
    following real entry. Only whole commented lines are removed — names like
    "Heart Item #1" contain '#' inside strings and must survive."""
    return "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#"))


def parse_apworld_locations(apworld_dir: Path) -> dict:
    text = strip_comment_lines((apworld_dir / "Locations.py").read_text(encoding="utf-8"))
    out = {}
    pattern = re.compile(r'"(?P<name>[^"]+)":\s*MMRLocationData\((?P<body>.*?)\n\s*\)', re.S)
    for m in pattern.finditer(text):
        addr = re.search(r"address=(0x[0-9A-Fa-f]+)", m.group("body"))
        if addr:  # event locations (locked_item only) have no address
            out[m.group("name")] = int(addr.group(1), 16)
    return out


def parse_apworld_items(apworld_dir: Path) -> dict:
    text = strip_comment_lines((apworld_dir / "Items.py").read_text(encoding="utf-8"))
    out = {}
    pattern = re.compile(r'"(?P<name>[^"]+)":\s*MMRItemData\((?P<body>.*?)\n\s*\)', re.S)
    for m in pattern.finditer(text):
        code = re.search(r"code=(0x[0-9A-Fa-f]+)", m.group("body"))
        if code:
            out[m.group("name")] = int(code.group(1), 16)
    return out


def parse_scene_ids() -> dict:
    """SCENE_NAME -> numeric id, from the /* 0xNN */ DEFINE_SCENE comments."""
    out = {}
    for m in re.finditer(
        r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*DEFINE_SCENE\(\s*[A-Z0-9_]+,\s*(SCENE_[A-Z0-9_]+),",
        SCENE_TABLE_H.read_text(encoding="utf-8"),
    ):
        out[m.group(2)] = int(m.group(1), 16)
    return out


def parse_gi_ids() -> dict:
    """GI_NAME -> numeric id, from the /* 0xNN */ enum comments in z64item.h."""
    out = {}
    for m in re.finditer(
        r"/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(GI_[A-Z0-9_]+)",
        Z64ITEM_H.read_text(encoding="utf-8"),
    ):
        out.setdefault(m.group(2), int(m.group(1), 16))
    return out


def parse_2s2h_checks() -> dict:
    """RC enum -> dict(type, scene, flagType, flag(str), vanillaItem)."""
    out = {}
    for m in re.finditer(
        r"^\s*RC\((RC_[A-Z0-9_]+),\s*(RCTYPE_[A-Z0-9_]+),\s*(SCENE_[A-Z0-9_]+),\s*"
        r"(FLAG_[A-Z0-9_]+),\s*([A-Za-z0-9_]+),\s*(RI_[A-Z0-9_]+)\)",
        CHECKS_CPP.read_text(encoding="utf-8"),
        re.M,
    ):
        out[m.group(1)] = {
            "type": m.group(2),
            "scene": m.group(3),
            "flagType": m.group(4),
            "flag": m.group(5),
            "vanilla": m.group(6),
        }
    return out


def parse_2s2h_items() -> dict:
    """RI enum -> dict(display, gi)."""
    out = {}
    for m in re.finditer(
        r'^\s*RI\((RI_[A-Z0-9_]+),\s*"[^"]*",\s*"([^"]*)",\s*RITYPE_[A-Z0-9_]+,\s*'
        r"[A-Za-z0-9_]+,\s*(GI_[A-Z0-9_]+|[A-Za-z0-9_]+),",
        ITEMS_CPP.read_text(encoding="utf-8"),
        re.M,
    ):
        out[m.group(1)] = {"display": m.group(2), "gi": m.group(3)}
    return out


# ---------------------------------------------------------------------------
# Location matching
# ---------------------------------------------------------------------------
def build_location_mapping(ap_locations, checks, scene_ids, gi_ids, items):
    # Structural indexes -------------------------------------------------------
    def flag_value(s):
        try:
            return int(s, 0)
        except ValueError:
            return None  # WEEKEVENTREG_* macros etc.

    chest_index, collectible_index, fairy_index = {}, {}, {}
    for enum, c in checks.items():
        scene = scene_ids.get(c["scene"])
        flag = flag_value(c["flag"])
        if scene is None or flag is None:
            continue
        key = (scene << 8) | flag
        if c["flagType"] == "FLAG_CYCL_SCENE_CHEST":
            chest_index.setdefault(key, []).append(enum)
        elif c["flagType"] == "FLAG_CYCL_SCENE_COLLECTIBLE":
            collectible_index.setdefault(key, []).append(enum)
        # stray fairies use switch OR collectible flags depending on spawn type;
        # the AP id's low byte is that same flag either way
        if c["type"] == "RCTYPE_STRAY_FAIRY":
            fairy_index.setdefault(key, []).append(enum)

    # vanilla GI id -> checks granting it (for type-0x00 AP locations)
    gi_index = {}
    for enum, c in checks.items():
        ri = items.get(c["vanilla"])
        if ri and ri["gi"] in gi_ids:
            gi_index.setdefault(gi_ids[ri["gi"]], []).append(enum)

    # Name indexes -------------------------------------------------------------
    by_exact, by_tokens = {}, {}
    for enum in checks:
        n = enum_to_name(enum, "RC_")
        by_exact.setdefault(normalize(n), enum)
        by_tokens.setdefault(token_key(n), []).append(enum)

    all_token_keys = [(token_key(enum_to_name(e, "RC_")), e) for e in checks]

    # kinds 0x00 (items) and 0x04 (songs) both store a GetItem id in the low
    # bits; count them together for the both-sides-unique constraint
    ap_gi_count = {}
    for name, ap_id in ap_locations.items():
        field = ap_id - AP_BASE
        if (field >> 12) == 0 or (field >> 16) == 0x04:
            gi = field & 0xFFF
            ap_gi_count[gi] = ap_gi_count.get(gi, 0) + 1

    mapping, unmatched = {}, []
    for name, ap_id in ap_locations.items():
        if name in LOCATION_OVERRIDES:
            target = LOCATION_OVERRIDES[name]
            if target is not None:
                mapping[name] = (ap_id, target, "override")
            continue

        field = ap_id - AP_BASE
        kind = field >> 16
        # chest / collectible / stray fairy: deterministic scene+flag join
        if kind in (0x01, 0x05, 0x06):
            index = {0x01: fairy_index, 0x05: collectible_index, 0x06: chest_index}[kind]
            hits = index.get(field & 0xFFFF, [])
            if len(hits) == 1:
                mapping[name] = (ap_id, hits[0], "scene+flag")
                continue
        # getitem-based (items and songs): join via the check's vanilla item's
        # GI id, only when unique on both sides
        if (field >> 12) == 0 or kind == 0x04:
            gi = field & 0xFFF
            hits = gi_index.get(gi, [])
            if len(hits) == 1 and ap_gi_count.get(gi) == 1:
                mapping[name] = (ap_id, hits[0], "vanilla-gi")
                continue

        exact = by_exact.get(normalize(name))
        if exact:
            mapping[name] = (ap_id, exact, "exact")
            continue
        candidates = by_tokens.get(token_key(name), [])
        if len(candidates) == 1:
            mapping[name] = (ap_id, candidates[0], "tokens")
            continue

        tk = token_key(name)
        scored = sorted(
            ((jaccard(tk, k), e) for k, e in all_token_keys),
            key=lambda t: -t[0],
        )[:3]
        unmatched.append((name, ap_id, [f"{e} ({s:.2f})" for s, e in scored if s > 0.2]))
    return mapping, unmatched


# ---------------------------------------------------------------------------
# Item matching
# ---------------------------------------------------------------------------
def build_item_mapping(ap_items, items):
    by_display, by_tokens, by_enum_name = {}, {}, {}
    for enum, d in items.items():
        by_display.setdefault(normalize(d["display"]), enum)
        by_tokens.setdefault(token_key(d["display"]), []).append(enum)
        by_enum_name.setdefault(normalize(enum_to_name(enum, "RI_")), enum)

    all_token_keys = [(token_key(d["display"]), e) for e, d in items.items()]

    mapping, unmatched = {}, []
    for name, code in ap_items.items():
        if name in ITEM_OVERRIDES:
            target = ITEM_OVERRIDES[name]
            if target is not None:
                mapping[name] = (code, target, "override")
            continue
        hit = by_display.get(normalize(name)) or by_enum_name.get(normalize(name))
        if hit:
            mapping[name] = (code, hit, "name")
            continue
        candidates = by_tokens.get(token_key(name), [])
        if len(candidates) == 1:
            mapping[name] = (code, candidates[0], "tokens")
            continue
        tk = token_key(name)
        scored = sorted(
            ((jaccard(tk, k), e) for k, e in all_token_keys),
            key=lambda t: -t[0],
        )[:3]
        unmatched.append((name, code, [f"{e} ({s:.2f})" for s, e in scored if s > 0.2]))
    return mapping, unmatched


# ---------------------------------------------------------------------------
# Emit
# ---------------------------------------------------------------------------
HEADER = """\
// GENERATED FILE - DO NOT EDIT BY HAND
// Produced by 2s2h/Network/Archipelago/tools/gen_ap_mapping.py from the
// mm_recomp apworld + 2S2H Rando static data. Re-run the script to update.
#ifdef ENABLE_ARCHIPELAGO

#include "ArchipelagoMapping.h"

namespace ApMapping {

"""

FOOTER = """\
RandoCheckId GetCheckFromApLocation(int64_t apLocationId) {
    auto it = AP_LOCATION_TO_CHECK.find(apLocationId);
    return it != AP_LOCATION_TO_CHECK.end() ? it->second : RC_UNKNOWN;
}

int64_t GetApLocationFromCheck(RandoCheckId checkId) {
    auto it = CHECK_TO_AP_LOCATION.find(checkId);
    return it != CHECK_TO_AP_LOCATION.end() ? it->second : -1;
}

RandoItemId GetItemFromApItem(int64_t apItemId) {
    auto it = AP_ITEM_TO_RANDO_ITEM.find(apItemId);
    return it != AP_ITEM_TO_RANDO_ITEM.end() ? it->second : RI_UNKNOWN;
}

} // namespace ApMapping

#endif // ENABLE_ARCHIPELAGO
"""


def emit_cpp(out_path, loc_mapping, item_mapping, loc_unmatched, item_unmatched):
    lines = [HEADER]

    lines.append("const std::unordered_map<int64_t, RandoCheckId> AP_LOCATION_TO_CHECK = {\n")
    for name, (ap_id, enum, how) in sorted(loc_mapping.items(), key=lambda kv: kv[1][0]):
        lines.append(f"    {{ 0x{ap_id:013X}, {enum} }}, // {name} [{how}]\n")
    lines.append("};\n\n")

    # Several AP locations can map to one check (e.g. "Glitched" alternates of
    # the same physical chest, or per-season variants). The reverse map must be
    # 1:1, so pick the canonical AP id: prefer non-Glitched, non-seasonal names.
    def reverse_priority(item):
        name, (ap_id, enum, how) = item
        return ("Glitched" in name, "(Spring)" in name, ap_id)

    reverse = {}
    for name, (ap_id, enum, how) in sorted(loc_mapping.items(), key=reverse_priority):
        reverse.setdefault(enum, (ap_id, name))

    lines.append("const std::unordered_map<RandoCheckId, int64_t> CHECK_TO_AP_LOCATION = {\n")
    for enum, (ap_id, name) in sorted(reverse.items()):
        lines.append(f"    {{ {enum}, 0x{ap_id:013X} }}, // {name}\n")
    lines.append("};\n\n")

    lines.append("const std::unordered_map<int64_t, RandoItemId> AP_ITEM_TO_RANDO_ITEM = {\n")
    for name, (code, enum, how) in sorted(item_mapping.items(), key=lambda kv: kv[1][0]):
        lines.append(f"    {{ 0x{code:013X}, {enum} }}, // {name} [{how}]\n")
    lines.append("};\n\n")

    if loc_unmatched or item_unmatched:
        lines.append("// ---- UNMATCHED (extend the override tables in gen_ap_mapping.py) ----\n")
        for name, ap_id, candidates in loc_unmatched:
            cand = (" candidates: " + ", ".join(candidates)) if candidates else ""
            lines.append(f"// location 0x{ap_id:013X} {name}{cand}\n")
        for name, code, candidates in item_unmatched:
            cand = (" candidates: " + ", ".join(candidates)) if candidates else ""
            lines.append(f"// item 0x{code:013X} {name}{cand}\n")
        lines.append("\n")

    lines.append(FOOTER)
    out_path.write_text("".join(lines), encoding="utf-8", newline="\n")


def emit_report(report_path, loc_mapping, item_mapping, loc_unmatched, item_unmatched,
                n_loc, n_item):
    with report_path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# AP <-> 2S2H mapping report\n\n")
        f.write(f"- Locations: {len(loc_mapping)}/{n_loc} matched, {len(loc_unmatched)} unmatched\n")
        f.write(f"- Items: {len(item_mapping)}/{n_item} matched, {len(item_unmatched)} unmatched\n\n")
        by_how = {}
        for _, (_, _, how) in loc_mapping.items():
            by_how[how] = by_how.get(how, 0) + 1
        f.write(f"Location match methods: {by_how}\n\n")
        if loc_unmatched:
            f.write("## Unmatched locations\n\n")
            f.write("| AP name | AP id | candidates |\n|---|---|---|\n")
            for name, ap_id, candidates in loc_unmatched:
                f.write(f"| {name} | 0x{ap_id:013X} | {', '.join(candidates)} |\n")
            f.write("\n")
        if item_unmatched:
            f.write("## Unmatched items\n\n")
            f.write("| AP name | AP id | candidates |\n|---|---|---|\n")
            for name, code, candidates in item_unmatched:
                f.write(f"| {name} | 0x{code:013X} | {', '.join(candidates)} |\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apworld", required=True, type=Path)
    ap.add_argument("--out", type=Path,
                    default=Path(__file__).resolve().parents[1] / "ArchipelagoMapping.cpp")
    ap.add_argument("--report", type=Path, default=Path("ap_mapping_report.md"))
    args = ap.parse_args()

    ap_locations = parse_apworld_locations(args.apworld)
    ap_items = parse_apworld_items(args.apworld)
    checks = parse_2s2h_checks()
    items = parse_2s2h_items()
    scene_ids = parse_scene_ids()
    gi_ids = parse_gi_ids()

    print(f"apworld: {len(ap_locations)} locations, {len(ap_items)} items")
    print(f"2s2h:    {len(checks)} checks, {len(items)} items, "
          f"{len(scene_ids)} scenes, {len(gi_ids)} getitems")

    # Validate overrides: a typo'd enum would emit non-compiling C++
    bad = [v for v in LOCATION_OVERRIDES.values() if v is not None and v not in checks]
    bad += [v for v in ITEM_OVERRIDES.values() if v is not None and v not in items]
    if bad:
        print("ERROR: override targets not found in 2S2H static data:")
        for b in bad:
            print(f"  {b}")
        return 1
    overridden = {k for k, v in LOCATION_OVERRIDES.items() if v is not None} | set(ITEM_OVERRIDES)
    stale = (set(LOCATION_OVERRIDES) - set(ap_locations)) | (set(ITEM_OVERRIDES) - set(ap_items))
    if stale:
        print(f"WARNING: overrides for unknown AP names (apworld changed?): {sorted(stale)}")

    loc_mapping, loc_unmatched = build_location_mapping(
        ap_locations, checks, scene_ids, gi_ids, items)
    item_mapping, item_unmatched = build_item_mapping(ap_items, items)

    print(f"locations matched: {len(loc_mapping)}/{len(ap_locations)}")
    print(f"items matched:     {len(item_mapping)}/{len(ap_items)}")

    emit_cpp(args.out, loc_mapping, item_mapping, loc_unmatched, item_unmatched)
    emit_report(args.report, loc_mapping, item_mapping, loc_unmatched, item_unmatched,
                len(ap_locations), len(ap_items))
    print(f"wrote {args.out}")
    print(f"wrote {args.report}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
