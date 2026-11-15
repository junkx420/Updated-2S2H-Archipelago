#pragma once
// Parity harness: collects the region init functions; harness main() runs them.
#include <functional>
#include <set>
#include <string>
#include <vector>

inline std::vector<std::function<void()>>& ShipInitFuncs() {
    static std::vector<std::function<void()>> funcs;
    return funcs;
}

struct RegisterShipInitFunc {
    RegisterShipInitFunc(std::function<void()> initFunc, const std::set<std::string>& updatePaths = {}) {
        ShipInitFuncs().push_back(initFunc);
    }
};
