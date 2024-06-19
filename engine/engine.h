#ifndef __ENGINE_H
#define __ENGINE_H
#include "actions.h"
#include "collisions.h"
#include "sc/array/sc_array.h"
#include "assets.h"
#include "particle_sys.h"

typedef struct Scene Scene_t;

typedef struct SFXList
{
    SFX_t sfx[N_SFX];
    uint32_t sfx_queue[N_SFX];
    uint32_t n_sfx;
    uint32_t played_sfx;
} SFXList_t;

typedef struct SceneNode {
    Scene_t* scene;
    Scene_t* next;
} SceneNode_t;

typedef struct SceneManager {
    Scene_t **scenes; // Array of all possible scenes
    unsigned int max_scenes;
    SceneNode_t* active; // Scenes to update. This allows multiple scene updates
    SceneNode_t* to_render; // Scenes to render. This allows duplicate rendering
} SceneManger_t;

typedef struct GameEngine {
    Scene_t **scenes;
    unsigned int max_scenes;
    unsigned int curr_scene;
    Assets_t assets;
    SFXList_t sfx_list;
    // Maintain own queue to handle key presses
    struct sc_queue_32 key_buffer;
    // This is the original size of the window.
    // This is in case of window scaling, where there needs to be
    // an absolute reference
    Vector2 intended_window_size;
} GameEngine_t;

//typedef enum SceneType {
//    LEVEL_SCENE = 0,
//    MENU_SCENE,
//}SceneType_t;

typedef enum SceneState {
    SCENE_PLAYING = 0,
    SCENE_SUSPENDED,
    SCENE_ENDED,
}SceneState_t;

typedef void(*render_func_t)(Scene_t*);
typedef void(*system_func_t)(Scene_t*);
typedef void(*action_func_t)(Scene_t*, ActionType_t, bool);
sc_array_def(system_func_t, systems);

typedef struct RenderLayer {
    RenderTexture2D layer_tex;
    Rectangle render_area;
}RenderLayer_t;

typedef struct SceneRenderLayers {
    RenderLayer_t render_layers[MAX_RENDER_LAYERS];
    uint8_t n_layers;
} SceneRenderLayers_t;

struct Scene {
    struct sc_map_64 action_map; // key -> actions
    struct sc_array_systems systems;
    SceneRenderLayers_t layers;
    Color bg_colour;
    action_func_t action_function;
    EntityManager_t ent_manager; // TODO: need move in data, Not all scene need this
    float delta_time;
    float time_scale;
    Vector2 mouse_pos;
    //SceneType_t scene_type;
    SceneState_t state;
    ParticleSystem_t part_sys;
    GameEngine_t *engine;
    int8_t depth_index;
};


void init_engine(GameEngine_t* engine, Vector2 starting_win_size);
void deinit_engine(GameEngine_t* engine);
void process_inputs(GameEngine_t* engine, Scene_t* scene);

void change_scene(GameEngine_t* engine, unsigned int idx);
bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx);
void play_sfx(GameEngine_t* engine, unsigned int tag_idx);
void play_sfx_pitched(GameEngine_t* engine, unsigned int tag_idx, float pitch);
void update_sfx_list(GameEngine_t* engine);

// Inline functions, for convenience
extern void update_scene(Scene_t* scene, float delta_time);
extern void render_scene(Scene_t* scene);
extern void do_action(Scene_t* scene, ActionType_t action, bool pressed);

void init_scene(Scene_t* scene, action_func_t action_func);
bool add_scene_layer(Scene_t* scene, int width, int height, Rectangle render_area);
void free_scene(Scene_t* scene);

#endif // __ENGINE_H
