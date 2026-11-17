// Parity harness main: runs 2S2H's real region logic (compiled from the unmodified Regions/*.cpp,
// Logic.h, Logic.cpp, TimeLogic.cpp) on a fixed save state read from stdin and prints what is reachable.
//
// Scenario format (one token per line, see compare.py):
//   scenario <name> / opt <RO_*> <v> / price <RC_*> <v> / inv <ITEM_*> / quest <QUEST_*> / inf <RANDO_INF_*>
//   week <WEEKEVENTREG_*> / owl <OWL_WARP_*> / sword|shield|quiver|bombbag|wallet|magic|health|bottle <v>
//   key <DUNGEON_SCENE_INDEX_*> <v> / fairy <DUNGEON_SCENE_INDEX_*> <v> / token <SCENE_*> <v>
//   ditem <DUNGEON_*> <DUNGEON_SCENE_INDEX_*> / end
#include <iostream>
#include <sstream>

#include "2s2h/ShipInit.hpp"
#include "2s2h/Rando/Logic/Logic.h"
#include "names.h"

MockSave gSaveContext;
MockState gMock;
int RANDO_SAVE_OPTIONS[RO_MAX];
int RANDO_EVENTS[RE_MAX];
MockSaveCheck RANDO_SAVE_CHECKS[RC_MAX];
namespace Rando::StaticData {
std::map<RandoCheckId, MockStaticCheck> Checks;
}

#include "souls.inc"

bool HaveEnemySoul(ActorId enemyId) { // Souls.cpp, verbatim logic
    auto findSoulFlag = enemySoulMap.find(enemyId);
    if (findSoulFlag != enemySoulMap.end()) {
        RandoItemId randoItemId = findSoulFlag->second;
        if (randoItemId != RI_UNKNOWN) {
            return Flags_GetRandoInf(SOUL_RI_TO_RANDO_INF(randoItemId));
        }
    }
    return true;
}

#include "logic_cpp.inc"
#include "timelogic.inc"

// Logic.cpp asks EntranceShuffle.cpp (not part of the harness) for a shuffled grotto; parity runs with it off.
namespace Rando::EntranceShuffle {
bool LogicGrottoEntrance(s32, s32*) {
    return false;
}
} // namespace Rando::EntranceShuffle

using namespace Rando::Logic;

static std::unordered_map<int, std::string> Reverse(const std::unordered_map<std::string, int>& m) {
    std::unordered_map<int, std::string> r;
    for (auto& [k, v] : m) {
        r[v] = k;
    }
    return r;
}

static int C(const std::string& name) {
    auto& m = ConstByName();
    auto it = m.find(name);
    if (it == m.end()) {
        std::cerr << "unknown constant " << name << "\n";
        std::exit(2);
    }
    return it->second;
}

static int N(const std::unordered_map<std::string, int>& m, const std::string& name) {
    auto it = m.find(name);
    if (it == m.end()) {
        std::cerr << "unknown enum name " << name << "\n";
        std::exit(2);
    }
    return it->second;
}

// The outer loop of ApplyGlitchlessLogicToSaveContext for a fixed inventory (no placement).
static void Solve(const std::string& name) {
    std::set<RandoRegionId> regionsInLogic = { RR_MAX };
    std::set<std::pair<RandoEvent, std::function<bool()>>*> eventsInLogic;
    auto regionTimeStates = InitializeRegionTimeStates(RR_MAX);
    for (int i = 0; i < RE_MAX; i++) {
        RANDO_EVENTS[i] = 0;
    }
    while (true) {
        bool changed = false;
        auto prev = regionsInLogic.size();
        for (RandoRegionId regionId : regionsInLogic) {
            FindReachableRegions(regionId, regionsInLogic, regionTimeStates);
        }
        changed |= regionsInLogic.size() != prev;
        for (RandoRegionId regionId : regionsInLogic) {
            auto& randoRegion = Regions[regionId];
            SetCurrentRegionTime(regionTimeStates, regionId);
            for (auto& randoEvent : randoRegion.events) {
                if (!eventsInLogic.contains(&randoEvent) && randoEvent.second()) {
                    RANDO_EVENTS[randoEvent.first]++;
                    eventsInLogic.insert(&randoEvent);
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    static auto rr = Reverse(RRByName()), rc = Reverse(RCByName()), re = Reverse(REByName());
    std::set<std::string> checks;
    for (RandoRegionId regionId : regionsInLogic) {
        SetCurrentRegionTime(regionTimeStates, regionId);
        for (auto& [checkId, logic] : Regions[regionId].checks) {
            if (logic.first()) {
                checks.insert(rc[checkId]);
            }
        }
    }
    std::cout << "scenario " << name << "\n";
    for (RandoRegionId r : regionsInLogic) {
        std::cout << "region " << rr[r] << "\n";
    }
    for (auto& c : checks) {
        std::cout << "check " << c << "\n";
    }
    for (int i = 0; i < RE_MAX; i++) {
        if (RANDO_EVENTS[i]) {
            std::cout << "event " << re[i] << " " << RANDO_EVENTS[i] << "\n";
        }
    }
    std::cout << "end" << std::endl;
}

int main() {
    for (auto& f : ShipInitFuncs()) {
        f();
    }
    std::string line, name;
    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string kw, a, b;
        ss >> kw >> a >> b;
        if (kw == "scenario") {
            name = a;
            gMock = MockState{};
            gSaveContext = MockSave{};
            for (int i = 0; i < RO_MAX; i++) {
                RANDO_SAVE_OPTIONS[i] = 0;
            }
            for (int i = 0; i < RC_MAX; i++) {
                RANDO_SAVE_CHECKS[i] = MockSaveCheck{};
            }
        } else if (kw == "opt") {
            RANDO_SAVE_OPTIONS[N(ROByName(), a)] = std::stoi(b);
        } else if (kw == "price") {
            RANDO_SAVE_CHECKS[N(RCByName(), a)].price = (u16)std::stoi(b);
        } else if (kw == "inv") {
            gMock.inv.insert(C(a));
        } else if (kw == "quest") {
            gMock.quest.insert(C(a));
        } else if (kw == "inf") {
            gMock.inf.insert(N(INFByName(), a));
        } else if (kw == "week") {
            gMock.weekreg.insert(C(a));
        } else if (kw == "owl") {
            gSaveContext.save.saveInfo.playerData.owlActivationFlags |= 1u << C(a);
        } else if (kw == "sword") {
            gMock.sword = std::stoi(a);
        } else if (kw == "shield") {
            gMock.shield = std::stoi(a);
        } else if (kw == "quiver") {
            gMock.quiver = std::stoi(a);
        } else if (kw == "bombbag") {
            gMock.bombbag = std::stoi(a);
        } else if (kw == "wallet") {
            gMock.wallet = std::stoi(a);
        } else if (kw == "magic") {
            gSaveContext.save.saveInfo.playerData.isMagicAcquired = std::stoi(a);
        } else if (kw == "health") {
            gSaveContext.save.saveInfo.playerData.healthCapacity = (u16)std::stoi(a);
        } else if (kw == "bottle") {
            gMock.bottle = std::stoi(a) != 0;
        } else if (kw == "key") {
            gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[C(a)] = std::stoi(b);
        } else if (kw == "fairy") {
            gSaveContext.save.saveInfo.inventory.strayFairies[C(a)] = (u8)std::stoi(b);
        } else if (kw == "token") {
            gMock.tokens[C(a)] = std::stoi(b);
        } else if (kw == "ditem") {
            gMock.dungeonItems.insert({ C(a), C(b) });
        } else if (kw == "end") {
            Solve(name);
        }
    }
}
