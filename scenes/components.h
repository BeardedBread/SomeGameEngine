#include "EC.h"
#include "render_queue.h"
#include "particle_sys.h" // includes assets

enum ComponentEnum {
    CMOVEMENTSTATE_T = N_BASIC_COMPS,
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
    CSQUISHABLE_T
};

typedef struct _CMovementState_t {
    uint8_t ground_state;
    uint8_t water_state;
    uint8_t x_dir;
    float water_overlap;
} CMovementState_t;

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

typedef unsigned int (*sprite_transition_func_t)(Entity_t *ent); // Transition requires knowledge of the entity

typedef struct _SpriteRenderInfo
{
    Sprite_t* sprite;
    Vector2 offset;
    AnchorPoint_t src_anchor;
    AnchorPoint_t dest_anchor;
} SpriteRenderInfo_t;

typedef struct _CSprite_t {
    RenderInfoNode node;
    SpriteRenderInfo_t* sprites;
    sprite_transition_func_t transition_func;
    unsigned int current_idx;
    int current_frame;
    float fractional;
    float rotation_speed; // Degree / s
    int elapsed;
    Vector2 offset;
    uint8_t depth;
    bool pause;
} CSprite_t;

typedef struct _CMoveable_t {
    uint16_t move_speed;
    Vector2 prev_pos;
    Vector2 target_pos;
    bool gridmove;
} CMoveable_t;

typedef struct _CEmitter_t
{
    EmitterHandle handle;
    Vector2 offset;
} CEmitter_t;

typedef struct _CSquishable_t {
    bool active; //< Dummy variable
} CSquishable_t;
