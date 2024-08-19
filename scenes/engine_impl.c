#include "mempool.h"
// Static allocate buffers
static CBBox_t bbox_buffer[MAX_COMP_POOL_SIZE];
static CTransform_t ctransform_buffer[MAX_COMP_POOL_SIZE];
static CTileCoord_t ctilecoord_buffer[MAX_COMP_POOL_SIZE];
static CMovementState_t cmstate_buffer[MAX_COMP_POOL_SIZE];
static CJump_t cjump_buffer[8]; // Only player is expected to have this
static CPlayerState_t cplayerstate_buffer[8]; // Only player is expected to have this
static CContainer_t ccontainer_buffer[MAX_COMP_POOL_SIZE];
static CHitBoxes_t chitboxes_buffer[MAX_COMP_POOL_SIZE];
static CHurtbox_t churtbox_buffer[MAX_COMP_POOL_SIZE];
static CSprite_t csprite_buffer[MAX_COMP_POOL_SIZE];
static CMoveable_t cmoveable_buffer[MAX_COMP_POOL_SIZE];
static CLifeTimer_t clifetimer_buffer[MAX_COMP_POOL_SIZE];
static CWaterRunner_t cwaterrunner_buffer[32];
static CAirTimer_t cairtimer_buffer[8]; // Only player is expected to have this
static CEmitter_t cemitter_buffer[MAX_COMP_POOL_SIZE]; // Only player is expected to have this
;

MemPool_t comp_mempools[N_COMPONENTS] = {
    {bbox_buffer, MAX_COMP_POOL_SIZE, sizeof(CBBox_t), NULL, {0}},
    {ctransform_buffer, MAX_COMP_POOL_SIZE, sizeof(CTransform_t), NULL, {0}},
    {ctilecoord_buffer, MAX_COMP_POOL_SIZE, sizeof(CTileCoord_t), NULL, {0}},
    {cmstate_buffer, MAX_COMP_POOL_SIZE, sizeof(CMovementState_t), NULL, {0}},
    {cjump_buffer, 8, sizeof(CJump_t), NULL, {0}},
    {cplayerstate_buffer, 8, sizeof(CPlayerState_t), NULL, {0}},
    {ccontainer_buffer, MAX_COMP_POOL_SIZE, sizeof(CContainer_t), NULL, {0}},
    {chitboxes_buffer, MAX_COMP_POOL_SIZE, sizeof(CHitBoxes_t), NULL, {0}},
    {churtbox_buffer, MAX_COMP_POOL_SIZE, sizeof(CHurtbox_t), NULL, {0}},
    {csprite_buffer, MAX_COMP_POOL_SIZE, sizeof(CSprite_t), NULL, {0}},
    {cmoveable_buffer, MAX_COMP_POOL_SIZE, sizeof(CMoveable_t), NULL, {0}},
    {clifetimer_buffer, MAX_COMP_POOL_SIZE, sizeof(CLifeTimer_t), NULL, {0}},
    {cwaterrunner_buffer, 32, sizeof(CWaterRunner_t), NULL, {0}},
    {cairtimer_buffer, 8, sizeof(CAirTimer_t), NULL, {0}},
    {cemitter_buffer, MAX_COMP_POOL_SIZE, sizeof(CEmitter_t), NULL, {0}},
};
