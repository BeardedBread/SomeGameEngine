#ifndef __SCENE_H
#define __SCENE_H
#include "entManager.h"
#include "actions.h"
#include "sc/array/sc_array.h"
typedef enum SceneType
{
    LEVEL_SCENE = 0,
}SceneType_t;

typedef struct Scene Scene_t;
typedef void(*system_func_t)(Scene_t *);
sc_array_def(system_func_t, systems);

struct Scene
{
    struct sc_map_64 action_map; // key -> actions
    struct sc_queue_64 action_queue;
    struct sc_array_systems systems;
    system_func_t render_function;
    EntityManager_t ent_manager;
    SceneType_t scene_type;
    void * scene_data;
    bool paused;
    bool has_ended;
};

extern void update_scene(Scene_t *scene);
extern void render_scene(Scene_t *scene);
extern void queue_action(Scene_t *scene, ActionType_t action);

void init_scene(Scene_t *scene, SceneType_t scene_type, system_func_t render_func);
void free_scene(Scene_t *scene);

#endif // __SCENE_H
