#ifndef __ENTITY_H
#define __ENTITY_H
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "sc/map/sc_map.h"
#include "sc/queue/sc_queue.h"

#include "engine_conf.h"

typedef struct EntityManager EntityManager_t;
typedef struct Entity Entity_t;

typedef enum AnchorPoint {
    AP_TOP_LEFT,
    AP_TOP_CENTER,
    AP_TOP_RIGHT,
    AP_MID_LEFT,
    AP_MID_CENTER,
    AP_MID_RIGHT,
    AP_BOT_LEFT,
    AP_BOT_CENTER,
    AP_BOT_RIGHT,
} AnchorPoint_t;

typedef enum MovementMode {
    REGULAR_MOVEMENT = 0,
    KINEMATIC_MOVEMENT,
}MovementMode_t;

/** Fundamental components:
// - Transform
// - BBox
// - TileCoords
**/
/** The component enums are purely a index store. Here, there are 3 basic component, that is necessary for the engine to function (mostly collisions)
 * To extend the component list, define another set of enums as pure integer store and begin the enum at N_BASIC_COMPS
 * These integers are also used by the component mempool as indices. Thus, the mempool must have the first 3 components as the basic components
 **/
#define N_BASIC_COMPS 3

enum BasicComponentEnum {
    CBBOX_COMP_T = 0,
    CTRANSFORM_COMP_T,
    CTILECOORD_COMP_T,
};

typedef struct _CTransform_t {
    Vector2 prev_position;
    Vector2 prev_velocity;
    Vector2 velocity;
    Vector2 accel;
    Vector2 fric_coeff;
    Vector2 shape_factor;
    float grav_delay;
    float grav_timer;
    MovementMode_t movement_mode;
    bool active;
} CTransform_t;

typedef struct _CBBox_t {
    Vector2 size;
    Vector2 offset;
    Vector2 half_size;
    bool solid;
    bool fragile;
} CBBox_t;

static inline void set_bbox(CBBox_t* p_bbox, unsigned int x, unsigned int y)
{
    p_bbox->size.x = x;
    p_bbox->size.y = y;
    p_bbox->half_size.x = (unsigned int)(x / 2);
    p_bbox->half_size.y = (unsigned int)(y / 2);
}
// This is to store the occupying tiles
// Limits to store 4 tiles at a tile,
// Thus the size of the entity cannot be larger than the tile
typedef struct _CTileCoord_t {
    unsigned int tiles[8];
    unsigned int n_tiles;
} CTileCoord_t;

struct Entity {
    Vector2 position;
    unsigned long m_id;
    unsigned int m_tag;
    unsigned long components[N_COMPONENTS]; 
    EntityManager_t* manager;
    bool m_alive;
    bool m_active;
};

enum EntityUpdateEvent
{
    COMP_ADDTION,
    COMP_DELETION,
};

struct EntityUpdateEventInfo
{
    unsigned long e_id;
    unsigned int comp_type;
    unsigned long c_id;
    enum EntityUpdateEvent evt_type;
};

sc_queue_def(struct EntityUpdateEventInfo, ent_evt);

struct EntityManager {
    // All fields are Read-Only
    struct sc_map_64v entities; // ent id : entity
    struct sc_map_64v entities_map[N_TAGS]; // [{ent id: ent}]
    bool tag_map_inited[N_TAGS];
    struct sc_map_64v component_map[N_COMPONENTS]; // [{ent id: comp}, ...]
    struct sc_queue_uint to_add;
    struct sc_queue_uint to_remove;
    struct sc_queue_ent_evt to_update;
};

void init_entity_manager(EntityManager_t* p_manager);
void init_entity_tag_map(EntityManager_t* p_manager, unsigned int tag_number, unsigned int initial_size);
void update_entity_manager(EntityManager_t* p_manager);
void clear_entity_manager(EntityManager_t* p_manager);
void free_entity_manager(EntityManager_t* p_manager);

Entity_t* add_entity(EntityManager_t* p_manager, unsigned int tag);
void remove_entity(EntityManager_t* p_manager, unsigned long id);
Entity_t *get_entity(EntityManager_t* p_manager, unsigned long id);

void* add_component(Entity_t *entity, unsigned int comp_type);
void* get_component(Entity_t *entity, unsigned int comp_type);
void remove_component(Entity_t* entity, unsigned int comp_type);

#endif // __ENTITY_H
