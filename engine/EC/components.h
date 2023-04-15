#ifndef __COMPONENTS_H
#define __COMPONENTS_H
#include "raylib.h"
#include <stdint.h>
#include "entity.h"
// TODO: Look at sc to use macros to auto generate functions

#define N_COMPONENTS 10
typedef enum ComponentEnum {
    CBBOX_COMP_T,
    CTRANSFORM_COMP_T,
    CTILECOORD_COMP_T,
    CMOVEMENTSTATE_T,
    CJUMP_COMP_T,
    CPLAYERSTATE_T,
    CCONTAINER_T,
    CHITBOXES_T,
    CHURTBOX_T,
    CSPRITE_T,
} ComponentEnum_t;

typedef struct _CBBox_t {
    Vector2 size;
    Vector2 offset;
    Vector2 half_size;
    bool solid;
    bool fragile;
} CBBox_t;

typedef struct _CTransform_t {
    Vector2 prev_position;
    Vector2 position;
    Vector2 velocity;
    Vector2 accel;
    Vector2 fric_coeff;
} CTransform_t;

typedef struct _CMovementState_t {
    uint8_t ground_state;
    uint8_t water_state;
} CMovementState_t;

// This is to store the occupying tiles
// Limits to store 4 tiles at a tile,
// Thus the size of the entity cannot be larger than the tile
typedef struct _CTileCoord_t {
    unsigned int tiles[8];
    unsigned int n_tiles;
} CTileCoord_t;

typedef struct _CJump_t {
    unsigned int jumps;
    unsigned int max_jumps;
    int jump_speed;
    bool jumped;
    bool jump_ready;
    bool short_hop;
} CJump_t;

typedef enum PlayerState {
    GROUNDED,
    AIR,
} PlayerState_t;

typedef struct _CPlayerState_t {
    Vector2 player_dir;
    uint8_t jump_pressed;
    uint8_t is_crouch;
} CPlayerState_t;

typedef enum ContainerItem {
    CONTAINER_EMPTY,
    CONTAINER_LEFT_ARROW,
    CONTAINER_RIGHT_ARROW,
    CONTAINER_UP_ARROW,
    CONTAINER_DOWN_ARROW,
    CONTAINER_COIN,
    CONTAINER_BOMB,
} ContainerItem_t;

typedef enum ContainerMaterial {
    WOODEN_CONTAINER,
    METAL_CONTAINER,
} ContainerMaterial_t;

typedef struct _CContainer_t {
    ContainerMaterial_t material;
    ContainerItem_t item;
} CContainer_t;

typedef struct _CHitBoxes_t {
    Rectangle boxes[2];
    uint8_t n_boxes;
    bool strong;
} CHitBoxes_t;

typedef struct _CHurtbox_t {
    Vector2 offset;
    Vector2 size;
    bool fragile;
} CHurtbox_t;

// Credits to bedroomcoders.co.uk for this
typedef struct Sprite {
    Texture2D* texture;
    Vector2 frame_size;
    Vector2 origin;
    int frame_count;
    int current_frame;
    int elapsed;
    int speed;
    char* name;
} Sprite_t;

typedef unsigned int (*sprite_transition_func_t)(Entity_t *ent); // Transition requires knowledge of the entity
typedef struct _CSprite_t {
    const char * const *sprites_map; //Array of all sprite names associated
    Sprite_t* sprite;
    sprite_transition_func_t transition_func;
    Vector2 offset;
    unsigned int current_idx;
} CSprite_t;

static inline void set_bbox(CBBox_t* p_bbox, unsigned int x, unsigned int y)
{
    p_bbox->size.x = x;
    p_bbox->size.y = y;
    p_bbox->half_size.x = (unsigned int)(x / 2);
    p_bbox->half_size.y = (unsigned int)(y / 2);
}
#endif // __COMPONENTS_H
