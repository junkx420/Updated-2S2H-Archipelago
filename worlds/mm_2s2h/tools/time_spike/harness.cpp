// Standalone harness: runs the *verbatim* 2S2H 5.0.1 time-propagation code (extracted by extract.py)
// on small region graphs read from a scenario file, and prints the resulting time masks.
// Only the game globals the code touches are mocked; the algorithm itself is not re-written here.
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

typedef int32_t s32;
typedef uint16_t u16;

// ---- mocks of game state --------------------------------------------------------------------
enum RandoOptionId { RO_CLOCK_SHUFFLE, RO_CLOCK_SHUFFLE_PROGRESSIVE, RO_MAX };
enum { RO_CLOCK_SHUFFLE_RANDOM, RO_CLOCK_SHUFFLE_ASCENDING, RO_CLOCK_SHUFFLE_DESCENDING };
static int RANDO_SAVE_OPTIONS[RO_MAX];
enum RandoInf { RANDO_INF_OBTAINED_CLOCK_DAY_1 = 0 }; // the 6 clock flags are consecutive
static unsigned gClockFlags = 0;
static unsigned gItems = 0;
static bool Flags_GetRandoInf(RandoInf f) { return (gClockFlags >> (int)f) & 1; }
typedef int RandoRegionId;
#define SPDLOG_ERROR(...)

namespace Rando {
namespace Logic {

#include "build/types.inc"

uint64_t gCurrentRegionTime = 0;
#include "build/setcur.inc"

struct RandoRegionExit {
    s32 returnEntrance;
    std::function<bool()> condition;
    std::string conditionString;
};
struct RandoRegion {
    const char* name = "";
    std::map<int, std::pair<std::function<bool()>, std::string>> checks;
    std::map<s32, RandoRegionExit> exits;
    std::map<RandoRegionId, std::pair<std::function<bool()>, std::string>> connections;
    std::set<s32> oneWayEntrances;
    uint64_t timeSlices = 0;
    bool canStayOverTime = true;
    std::unordered_map<TimeSlice, std::function<bool()>> timeStayRestrictions;
};
std::map<RandoRegionId, RandoRegion> Regions;
RandoRegionId GetRegionIdFromEntrance(s32) { return -1; } // exits are not used by the harness

namespace TimeLogic {
uint64_t ExpandTimeForward(uint64_t timeSlices, const RandoRegion& region);
uint64_t GetOwnedTimeSlices();
} // namespace TimeLogic

#include "build/helpers.inc"

namespace TimeLogic {
#include "build/timelogic.inc"
} // namespace TimeLogic

#include "build/propagation.inc"

// ---- tiny condition language, same as time_reference.py ------------------------------------
// expr := conj ('|' conj)* ; conj := atom ('&' atom)*
// atom := true | false | item:K | at:S | before:S | after:S | between:S:E | day1..night3 | isday | isnight
static bool EvalAtom(const std::string& a) {
    std::vector<std::string> p;
    std::stringstream ss(a);
    std::string t;
    while (std::getline(ss, t, ':')) p.push_back(t);
    const std::string& k = p[0];
    auto S = [&](int i) { return static_cast<TimeSlice>(std::stoi(p[i])); };
    if (k == "true") return true;
    if (k == "false") return false;
    if (k == "item") return (gItems >> std::stoi(p[1])) & 1;
    if (k == "at") return AT(S(1));
    if (k == "before") return BEFORE(S(1));
    if (k == "after") return AFTER(S(1));
    if (k == "between") return BETWEEN(S(1), S(2));
    if (k == "day1") return IS_DAY1();
    if (k == "night1") return IS_NIGHT1();
    if (k == "day2") return IS_DAY2();
    if (k == "night2") return IS_NIGHT2();
    if (k == "day3") return IS_DAY3();
    if (k == "night3") return IS_NIGHT3();
    if (k == "isday") return IS_DAY();
    if (k == "isnight") return IS_NIGHT();
    std::cerr << "bad atom " << a << "\n";
    std::exit(2);
}
static std::function<bool()> Compile(const std::string& e) {
    return [e] {
        std::stringstream ors(e);
        std::string conj;
        while (std::getline(ors, conj, '|')) {
            std::stringstream ands(conj);
            std::string atom;
            bool all = true;
            while (std::getline(ands, atom, '&'))
                if (!EvalAtom(atom)) { all = false; break; }
            if (all) return true;
        }
        return false;
    };
}

// ---- the outer loop of ApplyGlitchlessLogicToSaveContext, reduced to a fixed item state --------
static void RunScenario(const std::string& name, RandoRegionId start,
                        std::vector<std::pair<RandoRegionId, std::pair<int, std::function<bool()>>>>& checkDefs) {
    std::set<RandoRegionId> regionsInLogic = { start };
    auto regionTimeStates = InitializeRegionTimeStates(start);
    while (true) {
        auto prev = regionsInLogic.size();
        for (RandoRegionId regionId : regionsInLogic) FindReachableRegions(regionId, regionsInLogic, regionTimeStates);
        if (regionsInLogic.size() == prev) break;
    }
    std::cout << "scenario " << name << "\n";
    for (RandoRegionId r : regionsInLogic) std::cout << "mask " << r << " " << std::hex << regionTimeStates[r].timeSlices << std::dec << "\n";
    for (auto& [r, c] : checkDefs) {
        if (!regionsInLogic.contains(r)) { std::cout << "check " << c.first << " 0\n"; continue; }
        SetCurrentRegionTime(regionTimeStates, r);
        std::cout << "check " << c.first << " " << (c.second() ? 1 : 0) << "\n";
    }
    std::cout << "end\n";
}

} // namespace Logic
} // namespace Rando

int main() {
    using namespace Rando::Logic;
    std::string line, name;
    RandoRegionId start = 0;
    std::vector<std::pair<RandoRegionId, std::pair<int, std::function<bool()>>>> checks;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string kw;
        ss >> kw;
        if (kw == "scenario") { ss >> name; Regions.clear(); checks.clear(); }
        else if (kw == "options") { ss >> RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE] >> RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE_PROGRESSIVE] >> gClockFlags >> gItems; }
        else if (kw == "region") { int id, stay; ss >> id >> stay; Regions[id].canStayOverTime = stay; }
        else if (kw == "start") { ss >> start; }
        else if (kw == "stay") { int r, s; std::string c; ss >> r >> s >> c; Regions[r].timeStayRestrictions[static_cast<TimeSlice>(s)] = Compile(c); }
        else if (kw == "conn") { int a, b; std::string c; ss >> a >> b >> c; Regions[a].connections[b] = { Compile(c), c }; }
        else if (kw == "check") { int r, id; std::string c; ss >> r >> id >> c; checks.push_back({ r, { id, Compile(c) } }); }
        else if (kw == "end") { RunScenario(name, start, checks); }
    }
}
