#ifndef __COLLISION_FUNCS_H
#define __COLLISION_FUNCS_H
#include "EC.h"
#include "render_queue.h"

typedef enum SolidType
{
    NOT_SOLID = 0,
    SOLID,
    ONE_WAY,
}SolidType_t;

typedef enum TileRotation
{
    TILE_NOROTATE = 0,
    TILE_90CWROT,
    TILE_90CCWROT,
    TILE_180ROT
}TileRotation_t;

// Some of the fields are game-dependent, may have to think about doing OOP
// to separate out standard fields vs context-dependent fields
typedef struct Tile {
    unsigned int tile_type;
    uint8_t connectivity;
    SolidType_t solid;
    TileRotation_t rotation;
    uint8_t def;
    uint8_t water_level;
    uint8_t max_water_level;
    struct sc_map_64v entities_set;
    Vector2 offset;
    Vector2 size;
    bool moveable;
    bool wet;
}Tile_t;

typedef struct TileGrid
{
    unsigned int width;
    unsigned int height;
    unsigned int n_tiles;
    unsigned int max_tiles;
    unsigned int tile_size;
    Tile_t* tiles;
    RenderInfoNode* render_nodes;
}TileGrid_t;

typedef struct TileArea {
    unsigned int tile_x1;
    unsigned int tile_y1;
    unsigned int tile_x2;
    unsigned int tile_y2;
} TileArea_t;

typedef struct CollideEntity {
    Entity_t* p_ent;
    Rectangle bbox;
    Rectangle prev_bbox;
    TileArea_t area;
} CollideEntity_t;


void remove_entity_from_tilemap(EntityManager_t *p_manager, TileGrid_t* tilemap, Entity_t* p_ent);
uint8_t check_collision(const CollideEntity_t* ent, TileGrid_t* grid, bool check_oneway);
uint8_t check_collision_line(const CollideEntity_t* ent, TileGrid_t* grid, bool check_oneway);
uint8_t check_collision_at(Entity_t* p_ent, Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid);
bool check_on_ground(Entity_t* p_ent, Vector2 prev_pos, Vector2 bbox_sz, TileGrid_t* grid);
uint8_t check_bbox_edges(TileGrid_t* tilemap, Entity_t* p_ent, Vector2 bbox, bool ignore_fragile);
#endif // __COLLISION_FUNCS_H
