#pragma once
// Entrance tracker (apworld 0.11.1): discoveries marked by the real redirect, the shuffled lines the window lists and the
// spoiler safety of its list logic (Rando/EntranceTracker/EntranceTrackerList.cpp, compiled here without ImGui).
#include "Rando/EntranceTracker/EntranceTrackerList.cpp"
#include <random>
#include <set>
#include "legacy11_codec.h"

namespace TrackerTest {
using namespace Rando::EntranceShuffle;
namespace ET = Rando::EntranceTracker;

inline u8* Bits() {
    return gSaveContext.save.shipSaveInfo.rando.entrancesDiscovered;
}

inline void ClearDiscoveries() {
    std::fill(Bits(), Bits() + sizeof(RandoSaveInfo::entrancesDiscovered), u8(0));
}

inline std::set<size_t> Discoveries() {
    std::set<size_t> found;
    for (size_t endpoint = 0; endpoint < 8 * sizeof(RandoSaveInfo::entrancesDiscovered); ++endpoint) {
        if (IsDiscovered(endpoint)) {
            found.insert(endpoint);
        }
    }
    return found;
}

// Into a grotto through the hole of `row`, as z_door_ana.c does (grottos.h has the full sequence).
inline void EnterHole(size_t row, int flag) {
    const GrottoEntrance& grotto = *GrottoOf(row);
    gSaveContext.save.entrance = 0x6000;
    gSaveContext.respawnFlag = flag;
    playState = { grotto.gameEntrance, shuffleEntrances[row].outsideScene, 1 };
    if (grotto.kind == GROTTO_HOLE) {
        Play_SetupRespawnPoint(&playState, RESPAWN_MODE_UNK_3, 0);
        gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = s8(grotto.data);
    } else {
        gSaveContext.respawn[RESPAWN_MODE_UNK_3] = { 0x7770, 0x55, 7 };
    }
    OnBeforeTransitionSetup();
}

// Through one side of a row, as the game sets up that transition: the door from outside, the way out from inside.
inline void Traverse(size_t endpoint, int flag = 0) {
    size_t row = endpoint / 2;
    const ShuffleEntrance& entry = shuffleEntrances[row];
    gSaveContext.nextCutsceneIndex = 0;
    if (IsGrottoRow(row) && endpoint % 2 == 0) {
        EnterHole(row, flag);
        return;
    }
    if (IsGrottoRow(row)) {
        // Standing in grotto X = row: enter it through the hole it lies behind first, then take X's own way out.
        EnterHole(DoorForInside(row), 0);
        ClearDiscoveries();
        gSaveContext.save.entrance = playState.nextEntrance;
        playState.sceneId = SCENE_KAKUSIANA;
        if (GrottoOf(row)->kind == GROTTO_HOLE) {
            playState.nextEntrance = gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance;
            gSaveContext.respawnFlag = flag == 0 ? 4 : flag;
        } else {
            playState.nextEntrance = entry.outOf;
            gSaveContext.respawnFlag = flag;
        }
        OnBeforeTransitionSetup();
        return;
    }
    gSaveContext.respawnFlag = flag;
    playState = { endpoint % 2 == 0 ? entry.into : entry.outOf,
                  endpoint % 2 == 0 ? entry.outsideScene : entry.insideScene, 0 };
    OnBeforeTransitionSetup();
}

// The other side of the same coupled pairing, whose use marks the same two endpoints.
inline size_t ReverseOf(const TrackedEntrance& line) {
    if (line.mixed || line.twoWay) {
        return line.to;
    }
    size_t back = OverworldOpposite(line.to / 2);
    assert(back != NO_ROW);
    return 2 * back;
}

inline u16 ArrivalOf(const TrackedEntrance& line) {
    if (line.mixed) {
        return EndpointArrival(line.to);
    }
    const GrottoEntrance* grotto = IsGrottoRow(line.to / 2) ? GrottoOf(line.to / 2) : nullptr;
    return grotto != nullptr ? grotto->gameEntrance : shuffleEntrances[line.to / 2].into;
}

inline std::vector<ApSlot::SlotData> Slots() {
    std::vector<ApSlot::SlotData> slots;
    ApSlot::SlotData legacy;
    assert(legacy.Parse(json::parse(legacy09Slot)).empty()); // all pools, unmixed (real 0.9 slot data)
    slots.push_back(legacy);
    for (const char* fixture : mixedSlots) { // every combination of mixed pools, all other pools on
        ApSlot::SlotData slot;
        assert(slot.Parse(json::parse(fixture)).empty());
        slots.push_back(slot);
    }
    return slots;
}

inline void TestDiscoveries() {
    long lines = 0, traversals = 0, respawns = 0, warps = 0;
    std::set<RandoOptionId> pools;
    bool sawMixed = false, sawIdentity = false;
    auto slots = Slots();
    // The slots, then the 0.9 options with every door at its own place (each line drew itself).
    for (size_t config = 0; config <= slots.size(); ++config) {
        LoadSlot(slots[config % slots.size()]);
        if (config == slots.size()) {
            SetPairs({}, {});
        }
        auto tracked = TrackedEntrances();
        assert(!tracked.empty());
        std::set<size_t> froms;
        for (const auto& line : tracked) {
            assert(froms.insert(line.from).second); // one line per spoiler line
            assert(!FromName(line).empty() && FromName(line) != "?" && ToName(line) != "?");
        }
        for (const auto& line : tracked) {
            ++lines;
            pools.insert(line.pool);
            sawMixed |= line.mixed;
            sawIdentity |= !line.mixed && line.to / 2 == line.from / 2;
            const std::set<size_t> pair = { line.from, ReverseOf(line) };
            // Through the line: it leads where the tracker says, and marks the way there and the way back.
            ClearDiscoveries();
            Traverse(line.from);
            assert(playState.nextEntrance == ArrivalOf(line));
            assert(Discoveries() == pair);
            ++traversals;
            // The way back marks the same pairing. A boss room's way out is its blue warp (below).
            if (!(line.twoWay && shuffleEntrances[line.to / 2].templeExit != 0)) {
                ClearDiscoveries();
                Traverse(ReverseOf(line));
                if (!IsGrottoRow(line.from / 2)) {
                    assert(playState.nextEntrance == EndpointArrival(line.from));
                }
                assert(Discoveries() == pair);
                ++traversals;
            }
            // Void outs, deaths, songs and warps through the same entrance mark nothing.
            for (int flag : { 1, -1, -3, -6, 2, 5 }) {
                ClearDiscoveries();
                Traverse(line.from, flag);
                assert(Discoveries().empty());
                ++respawns;
            }
        }
        // Blue warps (repeat and the first one through the Giants' Chamber) mark nothing.
        for (const auto& row : shuffleEntrances) {
            if (row.templeExit == 0) {
                continue;
            }
            for (u16 warp : row.bossWarpsTo) {
                ClearDiscoveries();
                gSaveContext.respawnFlag = 0;
                playState = { warp, row.insideScene, 0 };
                OnBeforeTransitionSetup();
                assert(Discoveries().empty());
                ++warps;
            }
            ClearDiscoveries();
            gameInteractor.events = { GIEventTransition{ row.outOf, 0xFFF2, row.insideScene } };
            playState = { ENTRANCE(GIANTS_CHAMBER, 0), row.insideScene, 0 };
            OnBeforeTransitionSetup();
            assert(Discoveries().empty());
            ++warps;
        }
    }
    assert(sawMixed && sawIdentity && pools.size() == 6);
    // Nothing shuffled: no lines, and no transition marks anything.
    gSaveContext = {};
    std::fill(std::begin(options), std::end(options), 0);
    assert(TrackedEntrances().empty());
    for (size_t row = 0; row < shuffleEntrances.size(); ++row) {
        if (!IsGrottoRow(row)) {
            Traverse(2 * row);
        }
    }
    assert(Discoveries().empty());
    std::cout << "entrance tracker: " << lines << " lines in " << slots.size() + 1 << " saves, " << traversals
              << " traversals mark both ways, " << respawns << " respawns and " << warps << " warps mark nothing\n";
}

inline bool SameRows(const std::vector<ET::Row>& a, const std::vector<ET::Row>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].group != b[i].group || a[i].from != b[i].from || a[i].to != b[i].to || a[i].tooltip != b[i].tooltip ||
            a[i].discovered != b[i].discovered || a[i].current != b[i].current || a[i].twoWay != b[i].twoWay ||
            a[i].fromEndpoint != b[i].fromEndpoint) {
            return false;
        }
    }
    return true;
}

// Two saves with the same options and discoveries but different targets: with "Show All" off the window must not tell
// them apart, whatever the search (also every target's name), grouping or current scene.
inline long CompareHidden(const ApSlot::SlotData& slot, const std::vector<std::pair<int32_t, int32_t>>& otherPairs,
                          const std::vector<std::pair<int32_t, int32_t>>& otherMixed, std::mt19937& rng) {
    LoadSlot(slot);
    auto saveA = gSaveContext;
    auto linesA = TrackedEntrances();
    SetPairs(otherPairs, otherMixed);
    auto saveB = gSaveContext;
    auto linesB = TrackedEntrances();
    assert(linesA.size() == linesB.size());
    std::vector<std::string> searches = { "", "temple", "grotto", "clock", "-> ", "???", "zora", "#2", "room" };
    bool differ = false;
    for (size_t i = 0; i < linesA.size(); ++i) {
        assert(linesA[i].from == linesB[i].from);
        searches.push_back(ToName(linesB[i]));
        differ |= linesA[i].to != linesB[i].to;
    }
    gSaveContext = saveA;
    for (const auto& line : linesA) {
        searches.push_back(ToName(line));
    }
    assert(differ);
    std::vector<s16> scenes = { -1, SCENE_KAKUSIANA, SCENE_INISIE_R };
    for (const auto& row : shuffleEntrances) {
        scenes.push_back(row.insideScene);
        scenes.push_back(row.outsideScene);
    }
    long compared = 0;
    for (int round = 0; round < 3; ++round) {
        // The same discoveries in both saves: none, then a random third, then a random two thirds of the lines.
        std::vector<u8> bits(sizeof(RandoSaveInfo::entrancesDiscovered), 0);
        for (const auto& line : linesA) {
            if (round > 0 && int(rng() % 3) < round) {
                bits[line.from / 8] |= u8(1 << (line.from % 8));
            }
        }
        for (auto* save : { &saveA, &saveB }) {
            std::copy(bits.begin(), bits.end(), save->save.shipSaveInfo.rando.entrancesDiscovered);
        }
        for (bool byPool : { false, true }) {
            for (size_t s = 0; s < searches.size(); ++s) {
                ET::Options opts;
                opts.groupByPool = byPool;
                opts.search = searches[s];
                opts.currentScene = scenes[s % scenes.size()];
                opts.currentGrottoRow = s % 7 == 0 ? (size_t)-1 : (8 + s % 26);
                opts.sceneName = [](s16 scene) { return "Scene " + std::to_string(scene); };
                gSaveContext = saveA;
                auto rowsA = ET::BuildRows(linesA, opts);
                gSaveContext = saveB;
                auto rowsB = ET::BuildRows(linesB, opts);
                // Undiscovered lines must look the same; discovered ones may differ only in their target.
                std::vector<ET::Row> hiddenA, hiddenB;
                assert(rowsA.size() == rowsB.size() || round > 0);
                for (const auto& row : rowsA) {
                    if (!row.discovered) {
                        hiddenA.push_back(row);
                        assert(row.to == ET::HIDDEN_TARGET && row.tooltip.find(", to ") == std::string::npos);
                    }
                }
                for (const auto& row : rowsB) {
                    if (!row.discovered) {
                        hiddenB.push_back(row);
                    }
                }
                assert(SameRows(hiddenA, hiddenB));
                if (round == 0) {
                    assert(SameRows(rowsA, rowsB));
                } else if (opts.search.empty()) {
                    // Sorting: the same lines in the same places in both saves.
                    assert(rowsA.size() == rowsB.size());
                    for (size_t i = 0; i < rowsA.size(); ++i) {
                        assert(rowsA[i].fromEndpoint == rowsB[i].fromEndpoint && rowsA[i].group == rowsB[i].group);
                    }
                }
                ++compared;
            }
        }
        // Show All tells them apart, as it should.
        ET::Options all;
        all.showAll = true;
        gSaveContext = saveA;
        auto rowsA = ET::BuildRows(linesA, all);
        gSaveContext = saveB;
        auto rowsB = ET::BuildRows(linesB, all);
        assert(!SameRows(rowsA, rowsB));
        for (const auto& row : rowsA) {
            assert(row.to != ET::HIDDEN_TARGET);
        }
    }
    return compared;
}

inline void TestSpoilerFreeList() {
    std::mt19937 rng(20261118);
    auto slots = Slots();
    long compared = 0;
    // Unmixed pools: the real 0.9 pairing against every door at its own place.
    compared += CompareHidden(slots[0], {}, {}, rng);
    // Mixed pools: the fixture's pairing against one with partners swapped where the client accepts it.
    int swapped = 0;
    for (size_t k = 1; k < slots.size(); ++k) {
        auto mixed = slots[k].mixedEntrancePairs;
        bool changed = false;
        for (int attempt = 0; attempt < 200 && !changed; ++attempt) {
            auto candidate = mixed;
            size_t i = rng() % candidate.size(), j = rng() % candidate.size();
            if (i == j) {
                continue;
            }
            std::swap(candidate[i].second, candidate[j].second);
            LoadSlot(slots[k]);
            SetPairs(slots[k].entrancePairs, candidate);
            if (gSaveContext.save.shipSaveInfo.rando.mixedEntrancePairs[candidate[0].first] != 0) {
                mixed = candidate;
                changed = true;
            }
        }
        assert(changed);
        ++swapped;
        compared += CompareHidden(slots[k], slots[k].entrancePairs, mixed, rng);
    }
    std::cout << "entrance tracker list: " << compared << " hidden views identical across different pairings ("
              << swapped << " mixed fixtures swapped), search/tooltip/sorting reveal no undiscovered target\n";
}

// A save written by the client before the tracker (commit eafa920b7) has no entrancesDiscovered: it loads with no
// discoveries and its pairs unchanged.
inline void TestLegacy11Save() {
    auto slots = Slots();
    const auto& slot = slots.back();
    LoadSlot(slot);
    const auto& saved = gSaveContext.save.shipSaveInfo.rando;
    legacy11::RandoSaveInfo old{};
    std::copy(std::begin(saved.entrancePairs), std::end(saved.entrancePairs), old.entrancePairs);
    std::copy(std::begin(saved.mixedEntrancePairs), std::end(saved.mixedEntrancePairs), old.mixedEntrancePairs);
    std::copy(std::begin(options), std::begin(options) + legacy11::RO_MAX, old.randoSaveOptions);
    json encoded;
    legacy11::to_json(encoded, old);
    assert(!encoded.contains("entrancesDiscovered"));
    RandoSaveInfo restored{};
    std::fill(std::begin(restored.entrancesDiscovered), std::end(restored.entrancesDiscovered), u8(0xFF));
    from_json(json::parse(encoded.dump()), restored);
    assert(std::equal(std::begin(old.entrancePairs), std::end(old.entrancePairs), restored.entrancePairs));
    assert(std::equal(std::begin(old.mixedEntrancePairs), std::end(old.mixedEntrancePairs), restored.mixedEntrancePairs));
    assert(std::all_of(std::begin(restored.entrancesDiscovered), std::end(restored.entrancesDiscovered),
                       [](u8 bits) { return bits == 0; }));
    gSaveContext.save.shipSaveInfo.rando = restored;
    auto lines = TrackedEntrances();
    assert(!lines.empty());
    for (const auto& line : lines) {
        assert(!IsDiscovered(line.from));
    }
    // Round trip of the new field.
    std::mt19937 rng(7);
    for (auto& bits : gSaveContext.save.shipSaveInfo.rando.entrancesDiscovered) {
        bits = u8(rng());
    }
    json file = gSaveContext.save.shipSaveInfo.rando;
    RandoSaveInfo again{};
    from_json(file, again);
    assert(std::equal(std::begin(again.entrancesDiscovered), std::end(again.entrancesDiscovered),
                      gSaveContext.save.shipSaveInfo.rando.entrancesDiscovered));
    std::cout << "save from client eafa920b7 (no entrancesDiscovered): pairs kept, " << lines.size()
              << " lines undiscovered; 128 discovery bytes round-trip\n";
}

} // namespace TrackerTest

inline void TestEntranceTracker() {
    TrackerTest::TestDiscoveries();
    TrackerTest::TestSpoilerFreeList();
    TrackerTest::TestLegacy11Save();
}
