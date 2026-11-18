#pragma once
#define ENABLE_ARCHIPELAGO
#include "Network/Archipelago/ArchipelagoSlotData.cpp"
#undef ENABLE_ARCHIPELAGO
#include <fstream>
#include "legacy09_codec.h"
#include "mixed_slots.h"

inline void LoadSlot(const ApSlot::SlotData& slot) {
    gSaveContext = {};
    std::fill(std::begin(options), std::end(options), 0);
    const std::pair<const char*, RandoOptionId> names[] = {
        { "RO_SHUFFLE_DUNGEON_ENTRANCES", RO_SHUFFLE_DUNGEON_ENTRANCES },
        { "RO_SHUFFLE_BOSS_ENTRANCES", RO_SHUFFLE_BOSS_ENTRANCES },
        { "RO_SHUFFLE_GROTTOS", RO_SHUFFLE_GROTTOS },
        { "RO_SHUFFLE_INTERIORS", RO_SHUFFLE_INTERIORS },
        { "RO_SHUFFLE_OVERWORLD", RO_SHUFFLE_OVERWORLD },
        { "RO_SHUFFLE_SPECIAL_AREAS", RO_SHUFFLE_SPECIAL_AREAS },
    };
    for (auto [name, id] : names) {
        options[id] = slot.options.at(name);
    }
    auto mix = slot.options.find("RO_MIX_ENTRANCE_POOLS");
    if (mix != slot.options.end()) {
        options[RO_MIX_ENTRANCE_POOLS] = mix->second;
    }
    Rando::EntranceShuffle::SetPairs(slot.entrancePairs, slot.mixedEntrancePairs);
}

inline void TestMixedEntrances() {
    using namespace Rando::EntranceShuffle;
    long transitions = 0, warps = 0, rejected = 0;
    bool templeShop = false, landDoor = false;
    auto rowOf = [](u16 into) {
        auto row = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                [into](const auto& entry) { return entry.into == into; });
        assert(row != shuffleEntrances.end());
        return int(row - shuffleEntrances.begin());
    };
    for (const char* fixture : mixedSlots) {
        ApSlot::SlotData slot;
        assert(slot.Parse(json::parse(fixture)).empty());
        assert(slot.apworldVersion == "0.11.1");
        LoadSlot(slot);
        auto& save = gSaveContext.save.shipSaveInfo.rando;
        std::copy(std::begin(options), std::end(options), save.randoSaveOptions);
        json file = save;
        save = {};
        from_json(file, save);
        assert(json(save) == file);
        std::map<int32_t, int32_t> partner;
        for (auto [a, b] : slot.mixedEntrancePairs) {
            partner[a] = b;
            partner[b] = a;
        }
        for (auto [source, target] : partner) {
            const auto& a = shuffleEntrances[source / 2];
            const auto& b = shuffleEntrances[target / 2];
            const bool inner = source % 2 != 0;
            u16 entered = inner ? a.outOf : a.into;
            u16 expected = target % 2 ? b.into : b.outOf;
            s16 scene = inner ? a.insideScene : a.outsideScene;
            templeShop |= a.option == RO_SHUFFLE_DUNGEON_ENTRANCES && b.option == RO_SHUFFLE_INTERIORS;
            landDoor |= a.option == RO_SHUFFLE_OVERWORLD && b.option != RO_SHUFFLE_OVERWORLD;
            for (int flag : { 0, -2, 1, -1, -3 }) {
                gSaveContext.respawnFlag = flag;
                gSaveContext.nextCutsceneIndex = 0xFFF2;
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance = entered;
                playState = { entered, scene, 0 };
                OnBeforeTransitionSetup();
                bool redirected = (flag == 0 || flag == -2) && expected != entered;
                assert(playState.nextEntrance == (redirected ? expected : entered));
                assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == playState.nextEntrance);
                assert(gSaveContext.nextCutsceneIndex == (redirected ? 0xFFEF : 0xFFF2));
                ++transitions;
            }
        }
        for (size_t boss = 4; boss < 8; ++boss) {
            const auto& row = shuffleEntrances[boss];
            const auto& door = shuffleEntrances[DoorForInside(boss)];
            auto temple = std::find_if(shuffleEntrances.begin(), shuffleEntrances.end(),
                                       [&](const auto& r) { return r.outOf == door.templeExit; });
            int endpoint = 2 * int(temple - shuffleEntrances.begin()) + 1;
            u16 exterior = door.templeExit;
            if (partner.count(endpoint)) {
                int other = partner.at(endpoint);
                exterior = shuffleEntrances[other / 2].outOf;
            } else {
                exterior = shuffleEntrances[DoorForInside(endpoint / 2)].outOf;
            }
            bool moved = &door != &row || exterior != door.templeExit;
            for (u16 warp : row.bossWarpsTo) {
                gSaveContext.respawnFlag = 0;
                playState = { warp, row.insideScene, 0 };
                OnBeforeTransitionSetup();
                assert(playState.nextEntrance == (moved ? exterior : warp));
                if (moved) {
                    gSaveContext.save.entrance = exterior;
                    playState.position = 123;
                    OnSceneInit(0, 0);
                    assert(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == exterior);
                }
                ++warps;
            }
            gSaveContext.respawnFlag = 0;
            gameInteractor.events = { GIEventTransition{ row.outOf, 0xFFF2, row.insideScene } };
            playState = { ENTRANCE(GIANTS_CHAMBER, 0), row.insideScene, 0 };
            OnBeforeTransitionSetup();
            assert(std::get<GIEventTransition>(gameInteractor.events[0]).entrance == (moved ? exterior : row.outOf));
            OnSceneInit(0, 0);
            ++warps;
        }
        // Replace one partner, so rejection cannot be caused by duplicating the source in an extra record.
        for (int bad : { 2 * rowOf(ENTRANCE(GROTTOS, 0)), 2 * rowOf(ENTRANCE(TWINMOLDS_LAIR, 0)),
                         2 * rowOf(ENTRANCE(ZORA_HALL, 0)), 2 * rowOf(ENTRANCE(TRADING_POST, 0)),
                         2 * rowOf(ENTRANCE(TOWN_SHOOTING_GALLERY, 0)), slot.mixedEntrancePairs[1].second, 9999 }) {
            auto pairs = slot.mixedEntrancePairs;
            pairs.front().second = bad;
            SetPairs(slot.entrancePairs, pairs);
            auto& saved = gSaveContext.save.shipSaveInfo.rando.mixedEntrancePairs;
            assert(std::all_of(std::begin(saved), std::end(saved), [](u16 value) { return value == 0; }));
            ++rejected;
        }
    }
    assert(templeShop && landDoor);
    std::cout << std::size(mixedSlots) << " mixed pool combinations: " << transitions << " transitions, " << warps
              << " warps and " << rejected << " rejected pair maps passed\n";
}

inline void TestLegacy09(const char* savePath) {
    using namespace Rando::EntranceShuffle;
    ApSlot::SlotData slot;
    assert(slot.Parse(json::parse(legacy09Slot)).empty());
    assert(slot.apworldVersion == "0.9.0" && slot.entrancePairs.size() == 157 && slot.mixedEntrancePairs.empty());
    LoadSlot(slot);
    const auto& saved = gSaveContext.save.shipSaveInfo.rando;
    std::map<u16, size_t> rows;
    for (size_t i = 0; i < shuffleEntrances.size(); ++i) {
        rows[shuffleEntrances[i].into] = i;
    }
    for (auto [source, target] : slot.entrancePairs) {
        assert(saved.entrancePairs[rows.at(source)] == rows.at(target) + 1);
        assert(InsideBehind(rows.at(source)) == rows.at(target));
    }
    legacy09::RandoSaveInfo old = {};
    old.finalSeed = slot.randomSeed;
    for (const auto& [name, value] : slot.options) {
        old.randoSaveOptions[legacy09::optionIds.at(name)] = value;
    }
    std::map<u16, size_t> oldRows;
    for (size_t row = 0; row < std::size(legacy09::entrances); ++row) {
        oldRows[legacy09::entrances[row]] = row;
    }
    for (auto [source, target] : slot.entrancePairs) {
        old.entrancePairs[oldRows.at(source)] = u16(oldRows.at(target) + 1);
    }
    json encoded;
    legacy09::to_json(encoded, old);
    {
        std::ofstream out(savePath, std::ios::binary);
        out << encoded;
        out.close();
        assert(out.good());
    }
    std::ifstream in(savePath, std::ios::binary);
    const json file = json::parse(in);
    assert(file == encoded);
    assert(!file.contains("mixedEntrancePairs") && file.at("randoSaveOptions").size() == legacy09::RO_MAX);

    RandoSaveInfo restored = {};
    restored.randoSaveOptions[RO_MIX_ENTRANCE_POOLS] = 42;
    std::fill(std::begin(restored.mixedEntrancePairs), std::end(restored.mixedEntrancePairs), 42);
    from_json(file, restored);
    assert(std::equal(std::begin(old.entrancePairs), std::end(old.entrancePairs), restored.entrancePairs));
    assert(restored.randoSaveOptions[RO_MIX_ENTRANCE_POOLS] == RO_GENERIC_OFF);
    assert(std::all_of(std::begin(restored.mixedEntrancePairs), std::end(restored.mixedEntrancePairs),
                       [](u16 value) { return value == 0; }));
    gSaveContext.save.shipSaveInfo.rando = restored;
    std::copy(std::begin(restored.randoSaveOptions), std::end(restored.randoSaveOptions), options);
    for (auto [source, target] : slot.entrancePairs) {
        assert(InsideBehind(rows.at(source)) == rows.at(target));
    }
    const auto& inverted = shuffleEntrances.back();
    playState = { inverted.into, inverted.outsideScene, 0 };
    OnBeforeTransitionSetup();
    assert(playState.nextEntrance == inverted.into);
    std::cout << "0.9 save written with " << legacy09::revision << " and loaded with the current codec: "
              << slot.entrancePairs.size() << " pairs preserved, mixing off, "
              << std::size(restored.mixedEntrancePairs) << " zero mixed endpoints\n";
}
