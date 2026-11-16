"""Options of the 2S2H randomizer (Rando/Types.h RO_*), with the defaults of StaticData/Options.cpp.

Every option carries `ro`, the RO_* id it maps to. Slot data sends {RO_*: value} and the client writes
those straight into the save's randoSaveOptions. The values of Choice options are the RO enum values.
test/test_options.py checks that every default here equals OPTION_DEFAULTS from the generator.
"""
from dataclasses import dataclass

from Options import Choice, DeathLink, ItemDict, PerGameCommonOptions, Range, StartInventoryPool, Toggle


class RO:
    ro: str = ""


# ---- logic & access --------------------------------------------------------------------------
class Logic(RO, Choice):
    """Glitchless: the seed is beatable without glitches.
    No Logic: your items can be anywhere (the seed may be unbeatable).
    Nearly No Logic: like No Logic, but Deku Mask and Sonata are never in Woodfall Temple, Zora Mask and New Wave
    Bossa Nova never in Great Bay Temple, those and the boss remains and Oath to Order never on the Moon, and Epona's
    Song never on a cow."""
    display_name = "Logic"
    ro = "RO_LOGIC"
    option_glitchless = 0
    option_no_logic = 1
    option_nearly_no_logic = 2
    default = 0


class AccessDungeons(RO, Choice):
    """What opens the four temples."""
    display_name = "Dungeon Access"
    ro = "RO_ACCESS_DUNGEONS"
    option_form_and_song = 0
    option_form_or_song = 1
    option_form_only = 2
    option_song_only = 3
    option_open = 4
    default = 0


class AccessTrials(RO, Choice):
    """What opens the Moon trials."""
    display_name = "Trials Access"
    ro = "RO_ACCESS_TRIALS"
    option_20_masks = 0
    option_remains = 1
    option_forms = 2
    option_open = 3
    default = 0


class MajoraRemains(RO, Range):
    """Boss remains required to fight Majora."""
    display_name = "Majora Access Remains Required"
    ro = "RO_ACCESS_MAJORA_REMAINS_COUNT"
    range_start, range_end, default = 0, 4, 0


class MajoraMasks(RO, Range):
    """Non-transformation masks required to fight Majora."""
    display_name = "Majora Access Masks Required"
    ro = "RO_ACCESS_MAJORA_MASKS_COUNT"
    range_start, range_end, default = 0, 20, 0


class MoonRemains(RO, Range):
    """Boss remains required to enter the Moon."""
    display_name = "Moon Access Remains Required"
    ro = "RO_ACCESS_MOON_REMAINS_COUNT"
    range_start, range_end, default = 0, 4, 4


class MoonMasks(RO, Range):
    """Non-transformation masks required to enter the Moon."""
    display_name = "Moon Access Masks Required"
    ro = "RO_ACCESS_MOON_MASKS_COUNT"
    range_start, range_end, default = 0, 20, 0


# ---- time -------------------------------------------------------------------------------------
class ClockShuffle(RO, Toggle):
    """Each half-day (Day 1 ... Night 3) is an item. You start with one."""
    display_name = "Shuffle Time"
    ro = "RO_CLOCK_SHUFFLE"


class ClockShuffleMode(RO, Choice):
    """Any order: six distinct clocks (2S2H "Random"). Ascending/Descending: progressive clocks, in time order."""
    display_name = "Time Progression Mode"
    ro = "RO_CLOCK_SHUFFLE_PROGRESSIVE"
    option_any_order = 0
    option_ascending = 1
    option_descending = 2
    default = 0


class ClockTerminalTime(RO, Range):
    """Minutes into Night 3 at which the Final Hours start (Clock Shuffle)."""
    display_name = "Final Hours Start Time"
    ro = "RO_CLOCK_TERMINAL_TIME"
    range_start, range_end, default = 0, 359, 350


# ---- check pool (Rando/Logic/GeneratePools.cpp) ---------------------------------------------------
def _shuffle(cls_name: str, ro_id: str, name: str, doc: str):
    # class name == module attribute name, so the option pickles (AP test_pickle_dumps_default)
    return type(cls_name, (RO, Toggle), {"__doc__": doc, "display_name": name, "ro": ro_id, "__module__": __name__})


ShuffleOwlStatues = _shuffle("ShuffleOwlStatues", "RO_SHUFFLE_OWL_STATUES", "Owl Statues", "Activating an owl statue is a check.")
ShuffleShops = _shuffle("ShuffleShops", "RO_SHUFFLE_SHOPS", "Shops", "Items sold in shops are checks, with randomized prices.")
ShuffleTingleShops = _shuffle("ShuffleTingleShops", "RO_SHUFFLE_TINGLE_SHOPS", "Tingle Maps", "Maps sold by Tingle are checks.")
ShuffleBossRemains = _shuffle("ShuffleBossRemains", "RO_SHUFFLE_BOSS_REMAINS", "Boss Remains", "Temple bosses give a shuffled item instead of their remains.")
ShuffleCows = _shuffle("ShuffleCows", "RO_SHUFFLE_COWS", "Cows", "Playing Epona's Song to a cow is a check.")
ShuffleEnemyDrops = _shuffle("ShuffleEnemyDrops", "RO_SHUFFLE_ENEMY_DROPS", "Enemy Drops", "The first drop of each non-boss enemy is a check.")
ShuffleFrogs = _shuffle("ShuffleFrogs", "RO_SHUFFLE_FROGS", "Frogs", "Each scattered choir frog is a check.")
ShuffleButterflies = _shuffle("ShuffleButterflies", "RO_SHUFFLE_BUTTERFLIES", "Butterflies", "Butterfly swarms are checks.")
ShuffleFreestanding = _shuffle("ShuffleFreestanding", "RO_SHUFFLE_FREESTANDING_ITEMS", "Freestanding Items", "Items lying in the world are checks.")
ShuffleWonderItems = _shuffle("ShuffleWonderItems", "RO_SHUFFLE_WONDER_ITEMS", "Wonder Items", "Invisible touch/shoot triggers are checks.")
ShuffleGoldSkulltulas = _shuffle("ShuffleGoldSkulltulas", "RO_SHUFFLE_GOLD_SKULLTULAS", "Gold Skulltulas", "Spider House Gold Skulltulas are checks.")
ShufflePots = _shuffle("ShufflePots", "RO_SHUFFLE_POT_DROPS", "Pot Drops", "Breaking each pot is a check.")
ShuffleCrates = _shuffle("ShuffleCrates", "RO_SHUFFLE_CRATE_DROPS", "Crate Drops", "Breaking each crate is a check.")
ShuffleBarrels = _shuffle("ShuffleBarrels", "RO_SHUFFLE_BARREL_DROPS", "Barrel Drops", "Breaking each barrel is a check.")
ShuffleSnowballs = _shuffle("ShuffleSnowballs", "RO_SHUFFLE_SNOWBALL_DROPS", "Snowball Drops", "Breaking each large snowball is a check.")
ShuffleGrass = _shuffle("ShuffleGrass", "RO_SHUFFLE_GRASS_DROPS", "Grass Drops", "Cutting each clump of grass is a check.")
ShuffleTrees = _shuffle("ShuffleTrees", "RO_SHUFFLE_TREE_DROPS", "Tree Drops", "Bonking each shaking tree is a check.")
ShuffleHives = _shuffle("ShuffleHives", "RO_SHUFFLE_HIVE_DROPS", "Beehive Drops", "Knocking down each beehive is a check.")

# ---- item pool ------------------------------------------------------------------------------
ShuffleBossSouls = _shuffle("ShuffleBossSouls", "RO_SHUFFLE_BOSS_SOULS", "Boss Souls", "Bosses cannot be defeated without their soul.")
ShuffleEnemySouls = _shuffle("ShuffleEnemySouls", "RO_SHUFFLE_ENEMY_SOULS", "Enemy Souls", "Enemies cannot be defeated without their soul.")
ShuffleNpcSouls = _shuffle("ShuffleNpcSouls", "RO_SHUFFLE_NPC_SOULS", "NPC Souls",
                           "30 townsfolk and other NPCs (Granny, Anju, the Banker, Tingle, the cows, ...) are not there "
                           "until their soul is found: no rewards, no minigames, no sales from them.")
ShuffleSwim = _shuffle("ShuffleSwim", "RO_SHUFFLE_SWIM", "Shuffle Swim", "The ability to swim is an item.")
ShuffleOcarinaButtons = _shuffle("ShuffleOcarinaButtons", "RO_SHUFFLE_OCARINA_BUTTONS", "Ocarina Buttons", "Each ocarina button is an item.")
ShuffleSongSun = _shuffle("ShuffleSongSun", "RO_SHUFFLE_SONG_SUN", "Sun's Song", "Adds Sun's Song to the pool.")
ShuffleSongDoubleTime = _shuffle("ShuffleSongDoubleTime", "RO_SHUFFLE_SONG_DOUBLE_TIME", "Song of Double Time", "Song of Double Time is an item.")
ShuffleSongInvertedTime = _shuffle("ShuffleSongInvertedTime", "RO_SHUFFLE_SONG_INVERTED_TIME", "Inverted Song of Time", "Inverted Song of Time is an item.")
ShuffleSongSaria = _shuffle("ShuffleSongSaria", "RO_SHUFFLE_SONG_SARIA", "Saria's Song", "Adds Saria's Song (hint song) to the pool.")
ShuffleTycoonWallet = _shuffle("ShuffleTycoonWallet", "RO_SHUFFLE_TYCOON_WALLET", "Tycoon's Wallet", "Adds a third wallet upgrade.")
ShuffleDungeonEntrances = _shuffle("ShuffleDungeonEntrances", "RO_SHUFFLE_DUNGEON_ENTRANCES", "Shuffle Dungeon Entrances",
                                   "The doors of Woodfall, Snowhead, Great Bay and Stone Tower Temple lead into a "
                                   "random one of these four temples; its exit and its boss's warp lead back out of "
                                   "the same door. The inverted Stone Tower Temple stays where it is.")
ShuffleSkeletonKey = _shuffle("ShuffleSkeletonKey", "RO_SHUFFLE_SKELETON_KEY", "Skeleton Key", "Adds a key that opens every small-key door.")
ShuffleTraps = _shuffle("ShuffleTraps", "RO_SHUFFLE_TRAPS", "Traps", "Adds traps to the pool.")
PlentifulItems = _shuffle("PlentifulItems", "RO_PLENTIFUL_ITEMS", "Plentiful Items", "Adds a second copy of every major item.")
TriforceHunt = _shuffle("TriforceHunt", "RO_SHUFFLE_TRIFORCE_PIECES", "Triforce Hunt", "Collect Triforce pieces to win.")


class SkulltulasShuffled(RO, Range):
    """How many Gold Skulltulas per Spider House are checks (the rest hold their token)."""
    display_name = "Shuffled Gold Skulltulas"
    ro = "RO_SKULLTULA_SHUFFLED"
    range_start, range_end, default = 1, 30, 30


class SkulltulaTokensRequired(RO, Range):
    """Tokens needed for the Spider House rewards."""
    display_name = "Required Gold Skulltula Tokens"
    ro = "RO_SKULLTULA_TOKENS_REQUIRED"
    range_start, range_end, default = 1, 30, 30


class TrapAmount(RO, Range):
    """Number of traps (with Traps on)."""
    display_name = "Trap Amount"
    ro = "RO_TRAP_AMOUNT"
    range_start, range_end, default = 1, 100, 5


class TriforcePiecesMax(RO, Range):
    """Triforce pieces in the pool."""
    display_name = "Shuffled Triforce Pieces"
    ro = "RO_TRIFORCE_PIECES_MAX"
    range_start, range_end, default = 1, 1000, 15


class TriforcePiecesRequired(RO, Range):
    """Triforce pieces needed to win (capped at the shuffled amount)."""
    display_name = "Required Triforce Pieces"
    ro = "RO_TRIFORCE_PIECES_REQUIRED"
    range_start, range_end, default = 1, 1000, 15


class _Placement(RO, Choice):
    option_anywhere = 0
    option_own_dungeon = 1
    option_start_with = 2
    default = 0


class SmallKeys(_Placement):
    """Where small keys may be placed."""
    display_name = "Small Keys"
    ro = "RO_PLACEMENT_SMALL_KEYS"


class BossKeys(_Placement):
    """Where boss keys may be placed."""
    display_name = "Boss Keys"
    ro = "RO_PLACEMENT_BOSS_KEYS"


class StrayFairies(_Placement):
    """Where stray fairies may be placed."""
    display_name = "Stray Fairies"
    ro = "RO_PLACEMENT_STRAY_FAIRIES"


class StrayFairiesMax(RO, Range):
    """Stray fairies per temple in the pool."""
    display_name = "Stray Fairies in Pool"
    ro = "RO_STRAY_FAIRIES_MAX"
    range_start, range_end, default = 1, 15, 15


class StrayFairiesRequired(RO, Range):
    """Stray fairies per temple needed for the Great Fairy reward."""
    display_name = "Required Stray Fairies"
    ro = "RO_STRAY_FAIRIES_REQUIRED"
    range_start, range_end, default = 1, 15, 15


# ---- starting state ----------------------------------------------------------------------------
class StartingHealth(RO, Range):
    """Starting heart containers."""
    display_name = "Starting Health"
    ro = "RO_STARTING_HEALTH"
    range_start, range_end, default = 1, 20, 3


StartingConsumables = _shuffle("StartingConsumables", "RO_STARTING_CONSUMABLES", "Consumables Full", "Start with full deku sticks and nuts.")
StartingRupees = _shuffle("StartingRupees", "RO_STARTING_RUPEES", "Wallet Full", "Start with a full wallet.")
StartingMapsCompasses = _shuffle("StartingMapsCompasses", "RO_STARTING_MAPS_AND_COMPASSES", "Maps and Compasses", "Start with every map and compass.")
InfiniteRupees = _shuffle("InfiniteRupees", "RO_PURCHASE_INFINITE_RUPEES", "Repeatable Rupee Purchases", "Rupee shop items can be bought again.")


class StartingItems(ItemDict):
    """2S2H starting items (by AP item name), removed from the pool like in 2S2H."""
    display_name = "Starting Items"
    default = {"Progressive Sword": 1, "Hero's Shield": 1, "Ocarina of Time": 1, "Song of Time": 1}


# ---- hints (client side, passed through) ------------------------------------------------------
HintsBankSign = _shuffle("HintsBankSign", "RO_HINTS_BANK_SIGN", "Hint: Bank Reward", "The bank sign hints its reward.")
HintsBossRemains = _shuffle("HintsBossRemains", "RO_HINTS_BOSS_REMAINS", "Hint: Boss Remains", "Hints for the boss remains.")
HintsGossipStones = _shuffle("HintsGossipStones", "RO_HINTS_GOSSIP_STONES", "Hint: Gossip Stones", "Gossip stones give static hints.")
HintsHookshot = _shuffle("HintsHookshot", "RO_HINTS_HOOKSHOT", "Hint: Hookshot", "Hint for the Hookshot location.")
HintsOathToOrder = _shuffle("HintsOathToOrder", "RO_HINTS_OATH_TO_ORDER", "Hint: Oath to Order", "Hint for Oath to Order.")
HintsPurchaseable = _shuffle("HintsPurchaseable", "RO_HINTS_PURCHASEABLE", "Hint: Purchaseable", "Gossip stone hints can be bought.")
HintsSongOfSoaring = _shuffle("HintsSongOfSoaring", "RO_HINTS_SONG_OF_SOARING", "Hint: Song of Soaring", "Hint for Song of Soaring.")
HintsSpiderHouses = _shuffle("HintsSpiderHouses", "RO_HINTS_SPIDER_HOUSES", "Hint: Spider Houses", "Spider House rewards are hinted.")
HintsTransformations = _shuffle("HintsTransformations", "RO_HINTS_TRANSFORMATIONS", "Hint: Transformation Masks", "Hints for the transformation masks.")


class GossipHintStrength(RO, Range):
    """Gossip stone hint strength."""
    display_name = "Gossip Stone Hint Strength"
    ro = "RO_HINTS_GOSSIP_STONE_STRENGTH"
    range_start, range_end, default = 0, 100, 50


@dataclass
class MM2S2HOptions(PerGameCommonOptions):
    logic: Logic
    access_dungeons: AccessDungeons
    access_trials: AccessTrials
    majora_remains_required: MajoraRemains
    majora_masks_required: MajoraMasks
    moon_remains_required: MoonRemains
    moon_masks_required: MoonMasks
    clock_shuffle: ClockShuffle
    clock_shuffle_mode: ClockShuffleMode
    clock_terminal_time: ClockTerminalTime
    shuffle_owl_statues: ShuffleOwlStatues
    shuffle_shops: ShuffleShops
    shuffle_tingle_shops: ShuffleTingleShops
    shuffle_boss_remains: ShuffleBossRemains
    shuffle_cows: ShuffleCows
    shuffle_enemy_drops: ShuffleEnemyDrops
    shuffle_frogs: ShuffleFrogs
    shuffle_butterflies: ShuffleButterflies
    shuffle_freestanding: ShuffleFreestanding
    shuffle_wonder_items: ShuffleWonderItems
    shuffle_gold_skulltulas: ShuffleGoldSkulltulas
    skulltulas_shuffled: SkulltulasShuffled
    skulltula_tokens_required: SkulltulaTokensRequired
    shuffle_pots: ShufflePots
    shuffle_crates: ShuffleCrates
    shuffle_barrels: ShuffleBarrels
    shuffle_snowballs: ShuffleSnowballs
    shuffle_grass: ShuffleGrass
    shuffle_trees: ShuffleTrees
    shuffle_hives: ShuffleHives
    shuffle_boss_souls: ShuffleBossSouls
    shuffle_enemy_souls: ShuffleEnemySouls
    shuffle_npc_souls: ShuffleNpcSouls
    shuffle_swim: ShuffleSwim
    shuffle_ocarina_buttons: ShuffleOcarinaButtons
    shuffle_song_sun: ShuffleSongSun
    shuffle_song_double_time: ShuffleSongDoubleTime
    shuffle_song_inverted_time: ShuffleSongInvertedTime
    shuffle_song_saria: ShuffleSongSaria
    shuffle_tycoon_wallet: ShuffleTycoonWallet
    shuffle_skeleton_key: ShuffleSkeletonKey
    shuffle_dungeon_entrances: ShuffleDungeonEntrances
    shuffle_traps: ShuffleTraps
    trap_amount: TrapAmount
    plentiful_items: PlentifulItems
    triforce_hunt: TriforceHunt
    triforce_pieces_max: TriforcePiecesMax
    triforce_pieces_required: TriforcePiecesRequired
    small_keys: SmallKeys
    boss_keys: BossKeys
    stray_fairies: StrayFairies
    stray_fairies_max: StrayFairiesMax
    stray_fairies_required: StrayFairiesRequired
    starting_health: StartingHealth
    starting_consumables: StartingConsumables
    starting_rupees: StartingRupees
    starting_maps_and_compasses: StartingMapsCompasses
    infinite_rupees: InfiniteRupees
    starting_items: StartingItems
    hints_bank_sign: HintsBankSign
    hints_boss_remains: HintsBossRemains
    hints_gossip_stones: HintsGossipStones
    gossip_hint_strength: GossipHintStrength
    hints_hookshot: HintsHookshot
    hints_oath_to_order: HintsOathToOrder
    hints_purchaseable: HintsPurchaseable
    hints_song_of_soaring: HintsSongOfSoaring
    hints_spider_houses: HintsSpiderHouses
    hints_transformations: HintsTransformations
    death_link: DeathLink
    start_inventory_from_pool: StartInventoryPool
