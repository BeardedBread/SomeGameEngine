#ifndef __COMPONENTS_H
#define __COMPONENTS_H
#include "raylib.h"
// TODO: Look at sc to use macros to auto generate functions

#define N_COMPONENTS 3
enum ComponentEnum
{
    CBBOX_COMP_T,
    CTRANSFORM_COMP_T,
    CTILECOORD_COMP_T,
};
typedef enum ComponentEnum ComponentEnum_t;

typedef struct _CBBox_t
{
    Vector2 size;
}CBBox_t;

typedef struct _CTransform_t
{
    Vector2 prev_position;
    Vector2 position;
    Vector2 velocity;
    Vector2 accel;
}CTransform_t;

// This is to store the occupying tiles
// Limits to store 4 tiles at a tile,
// Thus the size of the entity cannot be larger than the tile
typedef struct _CTileCoord_t
{
    unsigned int tiles[8];
    unsigned int n_tiles;
}CTileCoord_t;

#endif // __COMPONENTS_H
