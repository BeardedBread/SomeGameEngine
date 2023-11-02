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

typedef struct GameEngine {
    Scene_t **scenes;
    unsigned int max_scenes;
    unsigned int curr_scene;
    Assets_t assets;
    SFXList_t sfx_list;
    // Maintain own queue to handle key presses
    struct sc_queue_32 key_buffer;
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

typedef void(*system_func_t)(Scene_t*);
typedef void(*action_func_t)(Scene_t*, ActionType_t, bool);
sc_array_def(system_func_t, systems);

struct Scene {
    struct sc_map_64 action_map; // key -> actions
    struct sc_array_systems systems;
    system_func_t render_function;
    action_func_t action_function;
    EntityManager_t ent_manager;
    //SceneType_t scene_type;
    SceneState_t state;
    ParticleSystem_t part_sys;
    GameEngine_t *engine;
};

void init_engine(GameEngine_t* engine);
void deinit_engine(GameEngine_t* engine);
void process_inputs(GameEngine_t* engine, Scene_t* scene);

void change_scene(GameEngine_t* engine, unsigned int idx);
bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx);
void play_sfx(GameEngine_t* engine, unsigned int tag_idx);
void update_sfx_list(GameEngine_t* engine);

// Inline functions, for convenience
extern void update_scene(Scene_t* scene);
extern void render_scene(Scene_t* scene);
extern void do_action(Scene_t* scene, ActionType_t action, bool pressed);

//void init_scene(Scene_t* scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func);
void init_scene(Scene_t* scene, system_func_t render_func, action_func_t action_func);
void free_scene(Scene_t* scene);

#endif // __ENGINE_H
