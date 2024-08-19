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

typedef enum ComponentEnum {
    CBBOX_COMP_T = 0,
    CTRANSFORM_COMP_T,
    CTILECOORD_COMP_T,
    CMOVEMENTSTATE_T,
    CJUMP_COMP_T,
    CPLAYERSTATE_T,
    CCONTAINER_T,
    CHITBOXES_T,
    CHURTBOX_T,
    CSPRITE_T,
    CMOVEABLE_T,
    CLIFETIMER_T,
    CWATERRUNNER_T,
    CAIRTIMER_T,
    CEMITTER_T,
} ComponentEnum_t;

typedef enum MovementMode {
    REGULAR_MOVEMENT = 0,
    KINEMATIC_MOVEMENT,
}MovementMode_t;

typedef struct _CBBox_t {
    Vector2 size;
    Vector2 offset;
    Vector2 half_size;
    bool solid;
    bool fragile;
} CBBox_t;

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

typedef struct _CMovementState_t {
    uint8_t ground_state;
    uint8_t water_state;
    uint8_t x_dir;
    float water_overlap;
} CMovementState_t;

// This is to store the occupying tiles
// Limits to store 4 tiles at a tile,
// Thus the size of the entity cannot be larger than the tile
typedef struct _CTileCoord_t {
    unsigned int tiles[8];
    unsigned int n_tiles;
} CTileCoord_t;

typedef struct _CJump_t {
    int jump_speed;
    uint8_t jumps;
    uint8_t max_jumps;
    uint8_t coyote_timer;
    bool jumped;
    bool jump_ready;
    bool short_hop;
    bool jump_released;
} CJump_t;

typedef enum PlayerState {
    GROUNDED,
    AIR,
} PlayerState_t;

typedef struct _CPlayerState_t {
    Vector2 player_dir;
    uint8_t jump_pressed;
    uint8_t is_crouch;
    bool ladder_state;
    bool locked; // Whether to respond to inputs
} CPlayerState_t;

typedef enum ContainerItem {
    CONTAINER_EMPTY,
    CONTAINER_LEFT_ARROW,
    CONTAINER_RIGHT_ARROW,
    CONTAINER_UP_ARROW,
    CONTAINER_DOWN_ARROW,
    CONTAINER_COIN,
    CONTAINER_BOMB,
    CONTAINER_EXPLOSION,
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
    uint8_t atk;
    bool one_hit;
} CHitBoxes_t;

typedef struct _CHurtbox_t {
    Vector2 offset;
    Vector2 size;
    uint8_t def;
    unsigned int damage_src;
} CHurtbox_t;

typedef struct _CLifeTimer_t {
    float life_time;
} CLifeTimer_t;

typedef struct _CAirTimer_t {
    float max_ftimer;
    float curr_ftimer;
    float decay_rate;
    uint8_t max_count;
    uint8_t curr_count;
} CAirTimer_t;

typedef struct _BFSTile {
    int32_t to;
    int32_t from;
    bool reachable;
}BFSTile_t;

typedef struct _BFSTileMap {
    BFSTile_t* tilemap;
    int32_t width;
    int32_t height;
    int32_t len;
}BFSTileMap_t;

typedef enum _WaterRunnerState
{
    BFS_RESET = 0,
    BFS_START,
    LOWEST_POINT_SEARCH,
    LOWEST_POINT_MOVEMENT,
    REACHABILITY_SEARCH,
    SCANLINE_FILL,
    FILL_COMPLETE,
}WaterRunerState_t;

typedef struct _CWaterRunner {
    BFSTileMap_t bfs_tilemap;
    WaterRunerState_t state;
    struct sc_queue_32 bfs_queue;
    bool* visited;
    int32_t current_tile;
    int32_t target_tile;
    int32_t fill_idx;
    int16_t fill_range[2];
    uint8_t movement_delay;
    int8_t movement_speed;
    int16_t counter;
    float fractional;
}CWaterRunner_t;

// Credits to bedroomcoders.co.uk for this
typedef struct Sprite {
    Texture2D* texture;
    Vector2 frame_size;
    Vector2 origin; // TL of the frame
    Vector2 anchor; // Where transformation anchors on
    uint8_t frame_per_row;
    int frame_count;
    int speed;
    char* name;
} Sprite_t;

typedef unsigned int (*sprite_transition_func_t)(Entity_t *ent); // Transition requires knowledge of the entity

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

typedef struct _SpriteRenderInfo
{
    Sprite_t* sprite;
    Vector2 offset;
    AnchorPoint_t src_anchor;
    AnchorPoint_t dest_anchor;
} SpriteRenderInfo_t;

typedef struct _CSprite_t {
    SpriteRenderInfo_t* sprites;
    sprite_transition_func_t transition_func;
    unsigned int current_idx;
    bool flip_x;
    bool flip_y;
    bool pause;
    int current_frame;
    float fractional;
    float rotation; // Degree
    float rotation_speed; // Degree / s
    int elapsed;
    Vector2 offset;
    Color colour;
} CSprite_t;

typedef struct _CMoveable_t {
    uint16_t move_speed;
    Vector2 prev_pos;
    Vector2 target_pos;
    bool gridmove;
} CMoveable_t;

typedef uint16_t EmitterHandle;
typedef struct _CEmitter_t
{
    EmitterHandle handle;
    Vector2 offset;
} CEmitter_t;

static inline void set_bbox(CBBox_t* p_bbox, unsigned int x, unsigned int y)
{
    p_bbox->size.x = x;
    p_bbox->size.y = y;
    p_bbox->half_size.x = (unsigned int)(x / 2);
    p_bbox->half_size.y = (unsigned int)(y / 2);
}

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
    ComponentEnum_t comp_type;
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

void* add_component(Entity_t *entity, ComponentEnum_t comp_type);
void* get_component(Entity_t *entity, ComponentEnum_t comp_type);
void remove_component(Entity_t* entity, ComponentEnum_t comp_type);

#endif // __ENTITY_H
