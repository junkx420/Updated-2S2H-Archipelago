#ifndef RANDO_ABILITIES_H
#define RANDO_ABILITIES_H

#include "2s2h/Rando/Types.h"

namespace Rando::Abilities {

struct Ability {
    RandoOptionId option;
    RandoItemId item;
    RandoInf flag;
};

const Ability* FindByItem(RandoItemId item);
void RegisterHooks();

} // namespace Rando::Abilities

#endif
