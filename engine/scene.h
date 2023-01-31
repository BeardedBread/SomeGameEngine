#ifndef __SCENE_H
#define __SCENE_H
#include "entManager.h"
#include "actions.h"
#include "sc/array/sc_array.h"
typedef enum SceneType
{
    LEVEL_SCENE = 0,
    MENU_SCENE,
}SceneType_t;

typedef struct Scene Scene_t;
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
    void * scene_data;
    bool paused;
    bool has_ended;
};

// Inline functions, for convenience
extern void update_scene(Scene_t *scene);
extern void render_scene(Scene_t *scene);
extern void do_action(Scene_t *scene, ActionType_t action, bool pressed);

void init_scene(Scene_t *scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func);
void free_scene(Scene_t *scene);

#endif // __SCENE_H
