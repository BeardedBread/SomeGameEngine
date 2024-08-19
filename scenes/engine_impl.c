#include "mempool.h"
#include "components.h"
/** This file is supposed to implement any required engine functions
 */

DEFINE_COMP_MEMPOOL_BUF(CMovementState_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CJump_t, 8)
DEFINE_COMP_MEMPOOL_BUF(CPlayerState_t, 8)
DEFINE_COMP_MEMPOOL_BUF(CContainer_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CHitBoxes_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CHurtbox_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CSprite_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CMoveable_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CLifeTimer_t, MAX_COMP_POOL_SIZE)
DEFINE_COMP_MEMPOOL_BUF(CWaterRunner_t, 32)
DEFINE_COMP_MEMPOOL_BUF(CAirTimer_t, 8)
DEFINE_COMP_MEMPOOL_BUF(CEmitter_t, 32)

// Component mempools are added in the order of the component enums
BEGIN_DEFINE_COMP_MEMPOOL
    ADD_COMP_MEMPOOL(CMovementState_t)
    ADD_COMP_MEMPOOL(CJump_t)
    ADD_COMP_MEMPOOL(CPlayerState_t)
    ADD_COMP_MEMPOOL(CContainer_t)
    ADD_COMP_MEMPOOL(CHitBoxes_t)
    ADD_COMP_MEMPOOL(CHurtbox_t)
    ADD_COMP_MEMPOOL(CSprite_t)
    ADD_COMP_MEMPOOL(CMoveable_t)
    ADD_COMP_MEMPOOL(CLifeTimer_t)
    ADD_COMP_MEMPOOL(CWaterRunner_t)
    ADD_COMP_MEMPOOL(CAirTimer_t)
    ADD_COMP_MEMPOOL(CEmitter_t)
END_DEFINE_COMP_MEMPOOL
