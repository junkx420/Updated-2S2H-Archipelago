#ifndef SOULS_H
#define SOULS_H

#include "Rando/Rando.h"

#define SOUL_RI_TO_RANDO_INF(randoItemId) ((randoItemId - RI_SOUL_BOSS_GOHT) + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)

// Two blocks: the 30 souls of apworld 0.2 and the souls appended for 0.12, each with its own RandoInf block.
#define RI_SOUL_NPC_FIRST RI_SOUL_NPC_ANJU
#define RI_SOUL_NPC_LAST RI_SOUL_NPC_TREASURE_CHEST_SHOP_GAL
#define RI_SOUL_NPC2_FIRST RI_SOUL_NPC_KAFEI
#define RI_SOUL_NPC2_LAST RI_SOUL_NPC_PIRATES
#define IS_NPC_SOUL(randoItemId)                                                  \
    (((randoItemId) >= RI_SOUL_NPC_FIRST && (randoItemId) <= RI_SOUL_NPC_LAST) || \
     ((randoItemId) >= RI_SOUL_NPC2_FIRST && (randoItemId) <= RI_SOUL_NPC2_LAST))
#define NPC_SOUL_RI_TO_RANDO_INF(randoItemId)                                            \
    ((randoItemId) >= RI_SOUL_NPC2_FIRST                                                 \
         ? (((randoItemId) - RI_SOUL_NPC2_FIRST) + RANDO_INF_OBTAINED_SOUL_OF_NPC_KAFEI) \
         : (((randoItemId) - RI_SOUL_NPC_FIRST) + RANDO_INF_OBTAINED_SOUL_OF_NPC_ANJU))

extern bool HaveEnemySoul(ActorId enemyId);
// False while Shuffle NPC Souls is on and the soul of this actor's NPC has not been found.
extern bool HaveNpcSoul(Actor* actor);
bool HaveNpcSoulsFor(RandoCheckId check);
bool HaveNpcSoulsFor(RandoEvent event);

#endif // SOULS_H
