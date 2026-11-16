#ifndef SOULS_H
#define SOULS_H

#include "Rando/Rando.h"

#define SOUL_RI_TO_RANDO_INF(randoItemId) ((randoItemId - RI_SOUL_BOSS_GOHT) + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)

#define RI_SOUL_NPC_FIRST RI_SOUL_NPC_ANJU
#define RI_SOUL_NPC_LAST RI_SOUL_NPC_TREASURE_CHEST_SHOP_GAL
#define IS_NPC_SOUL(randoItemId) ((randoItemId) >= RI_SOUL_NPC_FIRST && (randoItemId) <= RI_SOUL_NPC_LAST)
#define NPC_SOUL_RI_TO_RANDO_INF(randoItemId) \
    (((randoItemId) - RI_SOUL_NPC_FIRST) + RANDO_INF_OBTAINED_SOUL_OF_NPC_ANJU)

extern bool HaveEnemySoul(ActorId enemyId);
// False while Shuffle NPC Souls is on and the soul of this actor's NPC has not been found.
extern bool HaveNpcSoul(int16_t actorId);

#endif // SOULS_H