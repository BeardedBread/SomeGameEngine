#ifndef __ENGINE_H
#define __ENGINE_H
#include "entManager.h"
#include "actions.h"
#include "sc/array/sc_array.h"
typedef struct Scene Scene_t;

typedef struct GameEngine
{
    Scene_t **scenes;
    unsigned int max_scenes;
    unsigned int curr_scene;
}GameEngine_t;
void change_scene(GameEngine_t *engine, unsigned int idx);

typedef enum SceneType
{
    LEVEL_SCENE = 0,
    MENU_SCENE,
}SceneType_t;

typedef enum SceneState
{
    SCENE_PLAYING = 0,
    SCENE_SUSPENDED,
    SCENE_ENDED,
}SceneState_t;

typedef void(*system_func_t)(Scene_t *);
typedef void(*action_func_t)(Scene_t *, ActionType_t, bool);
sc_array_def(system_func_t, systems);

struct Scene
{
    struct sc_map_64 action_map; // key -> actions
    struct sc_array_systems systems;
    system_func_t render_function;
    action_func_t action_function;
    EntityManager_t ent_manager;
    SceneType_t scene_type;
    SceneState_t state;
    void * scene_data;
    GameEngine_t *engine;
};

// Inline functions, for convenience
extern void update_scene(Scene_t *scene);
extern void render_scene(Scene_t *scene);
extern void do_action(Scene_t *scene, ActionType_t action, bool pressed);

void init_scene(Scene_t *scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func);
void free_scene(Scene_t *scene);

#endif // __ENGINE_H
