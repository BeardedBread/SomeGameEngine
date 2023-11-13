#ifndef _ASSETS_TAG_H
#define _ASSETS_TAG_H
typedef enum EntityTag {
    NO_ENT_TAG = 0,
    PLAYER_ENT_TAG,
    ENEMY_ENT_TAG,
    CRATES_ENT_TAG,
    CHEST_ENT_TAG,
    ARROW_ENT_TAG,
    BOULDER_ENT_TAG,
    LEVEL_END_TAG,
    DESTRUCTABLE_ENT_TAG,
    DYNMEM_ENT_TAG,
} EntityTag_t;

typedef enum SFXTag {
    PLAYER_JMP_SFX = 0,
    PLAYER_LAND_SFX,
    PLAYER_RUN_SFX,
    PLAYER_LADDER_SFX,
    PLAYER_WATER_RUN_SFX,
    WATER_IN_SFX,
    WATER_OUT_SFX,
    WOOD_LAND_SFX,
    WOOD_DESTROY_SFX,
    METAL_LAND_SFX,
    METAL_DESTROY_SFX,
    BOULDER_LAND_SFX,
    BOULDER_DESTROY_SFX,
    ARROW_RELEASE_SFX,
    ARROW_DESTROY_SFX,
    BOMB_RELEASE_SFX,
    EXPLOSION_SFX,
    BUBBLE_SFX,
    COIN_SFX,
} SFXTag_t;
#endif
