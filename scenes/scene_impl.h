/* This implements the scene
 * ie it will insert the scene-specific data and systems
 * based on the function called
 * */
#ifndef __SCENE_IMPL_H
#define __SCENE_IMPL_H
#include "engine.h"
#include "gui.h"

#define CONTAINER_OF(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

typedef enum TileType {
    EMPTY_TILE = 0,
    SOLID_TILE,
    ONEWAY_TILE,
    LADDER,
    SPIKES,
} TileType_t;

#define MAX_TILE_SPRITES 32

typedef struct CoinCounter
{
    uint16_t current;
    uint16_t total;
}CoinCounter_t;

typedef enum CameraMode {
    CAMERA_FOLLOW_PLAYER = 0,
    CAMERA_RANGED_MOVEMENT,
} CameraMode_t;

typedef struct LevelCamera {
    Camera2D cam;
    CameraMode_t mode;
    Vector2 target_pos;
    float base_y;
    //Vector2 prev_pos;
    Vector2 current_vel;
    float mass;
    float c; // damping factor
    float k; // spring constant
    float range_limit;
}LevelCamera_t;

typedef enum LevelSceneState {
    LEVEL_STATE_STARTING = 0,
    LEVEL_STATE_RUNNING,
    LEVEL_STATE_DEAD,
    LEVEL_STATE_COMPLETE,
} LevelSceneState_t;

typedef struct LevelSceneStateMachine {
    LevelSceneState_t state;
    system_func_t state_functions[4];

    // Engine has no timeline support, so make a pseudo timeline implementation
    float fractional;
    uint32_t counter;
} LevelSceneStateMachine_t;

typedef struct LevelSceneData {
    TileGrid_t tilemap;
    // TODO: game_rec is actually obsolete since this is in the scene game layer
    Rectangle game_rec;
    LevelCamera_t camera;
    Sprite_t* tile_sprites[MAX_TILE_SPRITES];
    Sprite_t* solid_tile_sprites;
    uint8_t selected_solid_tilemap;
    LevelPack_t* level_pack;
    unsigned int current_level;
    CoinCounter_t coins;
    bool show_grid;
    Vector2 player_spawn;
    LevelSceneStateMachine_t sm;
    RenderManager render_manager;
}LevelSceneData_t;

static inline void change_level_state(LevelSceneData_t* data, LevelSceneState_t state)
{
    data->sm.state = state;
    data->sm.counter = 0;
    data->sm.fractional = 0;
}

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_game_scene(LevelScene_t* scene);
void free_game_scene(LevelScene_t* scene);
void init_sandbox_scene(LevelScene_t* scene);
void free_sandbox_scene(LevelScene_t* scene);
void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles, Rectangle view_zone);
void clear_an_entity(Scene_t* scene, TileGrid_t* tilemap, Entity_t* p_ent);
void clear_all_game_entities(LevelScene_t* scene);
void term_level_scene_data(LevelSceneData_t* data);
void reload_level_tilemap(LevelScene_t* scene);
void load_next_level_tilemap(LevelScene_t* scene);
void load_prev_level_tilemap(LevelScene_t* scene);
bool load_level_tilemap(LevelScene_t* scene, unsigned int level_num);
void change_a_tile(TileGrid_t* tilemap, unsigned int tile_idx, TileType_t new_type);

typedef enum GuiMode {
    KEYBOARD_MODE,
    MOUSE_MODE
} GuiMode_t;

typedef struct MenuSceneData {
    UIComp_t buttons[4];
    unsigned int selected_comp;
    unsigned int max_comp;
    GuiMode_t mode;
} MenuSceneData_t;

typedef struct MenuScene {
    Scene_t scene;
    MenuSceneData_t data;
} MenuScene_t;

typedef struct LevelSelectSceneData {
    VertScrollArea_t scroll_area;
    LevelPack_t* level_pack;
} LevelSelectSceneData_t;

typedef struct LevelSelectScene {
    Scene_t scene;
    LevelSelectSceneData_t data;
} LevelSelectScene_t;
void init_menu_scene(MenuScene_t* scene);
void free_menu_scene(MenuScene_t* scene);
void init_level_select_scene(LevelSelectScene_t* scene);
void free_level_select_scene(LevelSelectScene_t* scene);

#endif // __SCENE_IMPL_H
