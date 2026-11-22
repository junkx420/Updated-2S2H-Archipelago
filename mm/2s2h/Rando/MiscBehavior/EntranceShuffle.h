#ifndef RANDO_ENTRANCE_SHUFFLE_H
#define RANDO_ENTRANCE_SHUFFLE_H

#include "Rando/Rando.h"
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Rando {

namespace EntranceShuffle {

// One door the randomizer may shuffle. "into" is taken from outside (outsideScene) into the area behind the door,
// "outOf" leads from that area (insideScene) back out in front of the door. Shuffling is coupled: when door A leads
// into area B, B's way out leads back in front of A.
struct ShuffleEntrance {
    RandoOptionId option; // the option that shuffles this row (rows sharing an option are shuffled among each other)
    u16 into;
    u16 outOf;
    s16 insideScene;
    s16 outsideScene;
    s16 bossScene;                // boss room whose blue warp should return to this area's door, SCENE_MAX if none
    std::vector<u16> bossWarpsTo; // where that blue warp normally lands
    // Boss rows only: the containing temple's exterior exit. Their outOf is a one-way blue warp, not a back door.
    u16 templeExit = 0;
};

// Append only: a save stores the shuffle per row index (RandoSaveInfo::entrancePairs). Read by the apworld generator.
extern const std::vector<ShuffleEntrance> shuffleEntrances;

// Items the vanilla way to an area's door guarantees, which the area's own logic may silently rely on. The apworld adds
// the condition to a door's rule when this area (named by its "into") lies behind another door. Logic only: read by
// the apworld generator and evaluated by the check tracker.
struct ShuffleAreaRule {
    RandoOptionId option;
    u16 into;
    std::function<bool()> condition;
};

extern const std::vector<ShuffleAreaRule> shuffleAreaRules;

struct OwlWarp {
    u8 id;
    u16 entrance;
    const char* name;
    RandoRegionId region;
    std::function<bool()> canLeave;
};

extern const std::array<OwlWarp, 10> owlWarps;
size_t OwlWarpTarget(size_t source);
void SetOwlWarpTargets(const std::vector<int32_t>& targets);

struct CycleSpawn {
    u16 entrance;
    u16 exitToSouth;
    const char* name;
    RandoRegionId region;
};

const CycleSpawn* SelectedCycleSpawn();
void SetCycleSpawn(int32_t target);
void QueueCycleSpawnOnLoad();

struct TrackerExit {
    RandoRegionId target;
    const std::function<bool()>* areaCondition = nullptr;
};

using TrackerExits = std::map<std::pair<RandoRegionId, s32>, TrackerExit>;
bool HasActiveShuffle();
TrackerExits BuildTrackerExits();
size_t InsideBehind(size_t door);
size_t DoorForInside(size_t inside);
size_t MixedPartner(size_t endpoint);
u16 EndpointArrival(size_t endpoint);
std::optional<size_t> DirectedTarget(size_t source);
void SetDirectedTargets(const std::vector<std::pair<int32_t, int32_t>>& targets);
std::optional<u16> ShuffledBossExterior(size_t boss);

// How the game enters the area of a grotto row (option RO_SHUFFLE_GROTTOS).
enum GrottoKind : u8 {
    // A DoorAna hole: it stores itself as respawn[RESPAWN_MODE_UNK_3] (z_door_ana.c:144), the grotto's exit returns
    // there (exit 0xFFFF, z_player.c:6496).
    GROTTO_HOLE,
    // A plain scene exit without that respawn point; the grotto's exit is the row's outOf.
    GROTTO_SCENE_EXIT,
};

// Game side of a grotto row of shuffleEntrances (matched by "into"). Several grottos share one real entrance; the
// grotto data then decides which one the room is (Logic.cpp GetRegionIdFromEntrance, ObjGrass.cpp chestGrottoMap). A
// grotto is always the pair of both: redirecting a hole sets both to the target's values. Read by the apworld
// generator.
struct GrottoEntrance {
    u16 into;         // the row's logical "into", may be one of the logic's virtual ENTRANCE(GROTTOS, 17..31)
    u16 gameEntrance; // the entrance the game really loads
    s16 data;         // respawn[RESPAWN_MODE_UNK_3].data its hole writes (z_door_ana.c:150), -1 if none
    GrottoKind kind;
};

extern const std::vector<GrottoEntrance> grottoEntrances;

// Opening hours of an interior row (option RO_SHUFFLE_INTERIORS). The time logic gives every region the same mask
// (docs/time-logic.md), so a door's time rule only says that it opens at some time, and checks inside a building
// silently rely on the hours of its own door (e.g. the Town Shooting Gallery only tests the bow). Doors are therefore
// shuffled only among rows of one class; the apworld test proves each class against the logic's own door rules.
enum InteriorHours : u8 {
    INTERIOR_ALWAYS_OPEN, // at every time of day (some with a short gap no check depends on)
    INTERIOR_DAYTIME,     // daytime into the evening; no check inside needs the night
    INTERIOR_OWN_HOURS,   // hours of its own (night-only checks, items or events): stays at its own door
};

// Game side of an interior row of shuffleEntrances (matched by "into"). Read by the apworld generator.
struct InteriorEntrance {
    u16 into;
    InteriorHours hours;
};

extern const std::vector<InteriorEntrance> interiorEntrances;

// "into" of the overworld rows (RO_SHUFFLE_OVERWORLD) that arrive under water. Both ways of such a transition are
// taken swimming, so these rows pair only among each other, the others only among themselves. Read by the generator.
extern const std::vector<u16> overworldWaterEntrances;

// Names of a row as the apworld's spoiler log writes them (matched by "into"), shown by the in-game entrance tracker.
// "name" is the door-pool name (temples, boss rooms, grottos, interiors, special areas: "A <=> B") or, for an overworld
// row, its place ("East Clock Town 1"). doorExit and insideExit are the Archipelago exits of both sides ("Region ->
// Region [#n]"), used by mixed pools; insideExit is empty for overworld rows. The generator takes "name" over and
// checks the exit names against the logic graph. Read by the apworld generator.
struct EntranceName {
    u16 into;
    const char* name;
    const char* doorExit;
    const char* insideExit;
};

extern const std::vector<EntranceName> entranceNames;

// One line of the entrance tracker, one per line of the spoiler log: a door pool's coupled pair (twoWay, "A <=> B"), an
// overworld exit or a mixed endpoint. Endpoints are 2 * row (door side) and 2 * row + 1 (inside side), as in
// RandoSaveInfo::mixedEntrancePairs and entrancesDiscovered.
struct TrackedEntrance {
    size_t from;        // endpoint taken
    size_t to;          // endpoint whose arrival it leads to (the door pools' area behind the door: its inside side)
    RandoOptionId pool; // the row's own option
    bool mixed;
    bool twoWay;
    s16 fromScene; // scene of the side taken (original scene id)
    bool directed = false;
};

// The shuffled lines of the current save; rows whose pool is off, or that never move, are left out.
std::vector<TrackedEntrance> TrackedEntrances();
// Set once Link went through either side of the line (both sides of a coupled pairing are marked together).
bool IsDiscovered(size_t endpoint);
// Display names of a line, word for word the spoiler log's ("from" is the entrance column, "to" the exit column).
std::string FromName(const TrackedEntrance& line);
std::string ToName(const TrackedEntrance& line);
s16 TrackedArrivalScene(const TrackedEntrance& line);
// The grotto row of the current grotto (they all share SCENE_KAKUSIANA), or (size_t)-1.
size_t CurrentGrottoRow();

// In-game check tracker: with Shuffle Grottos on, the logic entrance of the grotto the player is in, decided by the
// real entrance and the grotto data instead of the place outside (the grotto may lie behind another hole).
bool LogicGrottoEntrance(s32 entrance, s32* logicEntrance);

// Writes the pairing from the apworld's slot_data ([door's "into", "into" of the area behind it]) into the save.
void SetPairs(const std::vector<std::pair<int32_t, int32_t>>& pairs,
              const std::vector<std::pair<int32_t, int32_t>>& mixedPairs = {});

// BeforeTransitionSetup hook: replaces play->nextEntrance when it goes through a shuffled door.
void OnBeforeTransitionSetup();

// After an overridden blue warp, replace both respawn records with the actual exterior spawn.
void OnSceneInit(s16 sceneId, s8 spawnNum);

} // namespace EntranceShuffle

} // namespace Rando

#endif
