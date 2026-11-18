#ifndef RANDO_OBJECT_MARKER_H
#define RANDO_OBJECT_MARKER_H

#include <optional>
#include "Rando/Rando.h"

namespace Rando::ObjectMarker {

enum class TintPasses { Opaque, OpaqueAndTranslucent };

bool IsOpen(RandoCheckId checkId);
std::optional<Color_RGBA8> GetColor(RandoCheckId checkId);
// A shared object shows the fixed marker as long as any check in its range is open.
std::optional<Color_RGBA8> GetFixedColor(RandoCheckId first, RandoCheckId last);

void AttachTint(Actor* actor, RandoCheckId checkId, TintPasses passes = TintPasses::Opaque);
void AttachTint(Actor* actor, RandoCheckId first, RandoCheckId last, TintPasses passes = TintPasses::Opaque);
void DrawSparkle(PlayState* play, Vec3f pos, std::optional<Color_RGBA8> color);

} // namespace Rando::ObjectMarker

#endif
