#include "scene.h"

void init_scene(Scene_t *scene, SceneType_t scene_type, system_func_t render_func)
{
    sc_map_init_64(&scene->action_map, 32, 0);
    sc_queue_init(&scene->action_queue);
    sc_array_init(&scene->systems);
    init_entity_manager(&scene->ent_manager);

    scene->scene_type = scene_type;
    scene->render_function = render_func;
    scene->paused = false;
    scene->has_ended = false;
}

void free_scene(Scene_t *scene)
{
    sc_map_term_64(&scene->action_map);
    sc_queue_term(&scene->action_queue);
    sc_array_term(&scene->systems);
    free_entity_manager(&scene->ent_manager);
}

inline void update_scene(Scene_t *scene)
{
    system_func_t sys;
    sc_array_foreach(&scene->systems, sys)
    {
        sys(scene);
    }
}

inline void render_scene(Scene_t *scene)
{
    scene->render_function(scene);
}

inline void queue_action(Scene_t *scene, ActionType_t action)
{
    sc_queue_add_last(&scene->action_queue, action);
}

