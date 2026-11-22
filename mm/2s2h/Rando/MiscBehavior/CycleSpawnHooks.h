#ifndef RANDO_CYCLE_SPAWN_HOOKS_H
#define RANDO_CYCLE_SPAWN_HOOKS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prepare a shuffled cycle start. Return true at dawn to run DayTelop before entering the scene.
bool Rando_PrepareCycleSpawn(void);

#ifdef __cplusplus
}
#endif
#endif
