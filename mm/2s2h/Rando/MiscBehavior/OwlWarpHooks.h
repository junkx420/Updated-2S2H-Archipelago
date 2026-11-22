#ifndef RANDO_OWL_WARP_HOOKS_H
#define RANDO_OWL_WARP_HOOKS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool Rando_CanSelectOwlWarp(int32_t source);
uint16_t Rando_ResolveOwlWarp(int32_t source, uint16_t vanillaEntrance);

#ifdef __cplusplus
}
#endif
#endif
