#ifndef __ENGINE_H
#define __ENGINE_H
#include "actions.h"
#include "collisions.h"
#include "sc/array/sc_array.h"
#include "sc/heap/sc_heap.h"
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

typedef struct GameEngine {
    Scene_t **scenes;
    unsigned int max_scenes;
    unsigned int curr_scene; // Current root scene
    Assets_t assets;
    SFXList_t sfx_list;
    // Maintain own queue to handle key presses
    Scene_t* focused_scene; // The one scene to receive key inputs
    struct sc_queue_32 key_buffer;
    struct sc_queue_ptr scene_stack;
    struct sc_heap scenes_render_order;
    // This is the original size of the window.
    // This is in case of window scaling, where there needs to be
    // an absolute reference
    Vector2 intended_window_size;
} GameEngine_t;

#define SCENE_ACTIVE_BIT (1 << 0) // Systems Active
#define SCENE_RENDER_BIT (1 << 1) // Whether to render
#define SCENE_COMPLETE_ACTIVE (SCENE_ACTIVE_BIT | SCENE_RENDER_BIT)

typedef enum ActionResult {
    ACTION_PROPAGATE = 0,
    ACTION_CONSUMED,
} ActionResult;

typedef void(*system_func_t)(Scene_t*);
typedef ActionResult(*action_func_t)(Scene_t*, ActionType_t, bool);
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
    // Not all scene needs an entity manager
    // but too late to change this
    EntityManager_t ent_manager;
    Scene_t* parent_scene;
    struct sc_map_64 action_map; // key -> actions
    struct sc_array_systems systems;
    SceneRenderLayers_t layers;
    Color bg_colour;
    action_func_t action_function;
    float delta_time;
    float time_scale;
    Vector2 mouse_pos;
    uint8_t state;
    ParticleSystem_t part_sys;
    GameEngine_t *engine;
    int8_t depth_index;
    SceneNode_t child_scene; // Intrusive Linked List for children scene
};


void init_engine(GameEngine_t* engine, Vector2 starting_win_size);
void deinit_engine(GameEngine_t* engine);
void process_inputs(GameEngine_t* engine, Scene_t* scene);

void process_active_scene_inputs(GameEngine_t* engine);
void update_curr_scene(GameEngine_t* engine);
void render_curr_scene(GameEngine_t* engine);

void change_scene(GameEngine_t* engine, unsigned int idx);
void change_active_scene(GameEngine_t* engine, unsigned int idx);
void change_focused_scene(GameEngine_t* engine, unsigned int idx);
bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx);
void play_sfx(GameEngine_t* engine, unsigned int tag_idx);
void play_sfx_pitched(GameEngine_t* engine, unsigned int tag_idx, float pitch);
void update_sfx_list(GameEngine_t* engine);

// Inline functions, for convenience
extern void update_scene(Scene_t* scene, float delta_time);
extern void render_scene(Scene_t* scene);
extern ActionResult do_action(Scene_t* scene, ActionType_t action, bool pressed);

void init_scene(Scene_t* scene, action_func_t action_func);
bool add_scene_layer(Scene_t* scene, int width, int height, Rectangle render_area);
void free_scene(Scene_t* scene);
void add_child_scene(GameEngine_t* engine, unsigned int child_idx, unsigned int parent_idx);
void remove_child_scene(GameEngine_t* engine, unsigned int idx);

#endif // __ENGINE_H
