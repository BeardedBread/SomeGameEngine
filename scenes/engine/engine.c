#include "engine.h"

void init_engine(GameEngine_t* engine)
{
    sc_queue_init(&engine->key_buffer);
}

void deinit_engine(GameEngine_t* engine)
{
    sc_queue_term(&engine->key_buffer);
}

void process_inputs(GameEngine_t* engine, Scene_t* scene)
{
    unsigned int sz = sc_queue_size(&engine->key_buffer);
    // Process any existing pressed key
    for (size_t i = 0; i < sz; i++)
    {
        int button = sc_queue_del_first(&engine->key_buffer);
        ActionType_t action = sc_map_get_64(&scene->action_map, button);
        if (IsKeyReleased(button))
        {
            do_action(scene, action, false);
        }
        else
        {
            do_action(scene, action, true);
            sc_queue_add_last(&engine->key_buffer, button);
        }
    }

    // Detect new key presses
    while(true)
    {
        int button = GetKeyPressed();
        if (button == 0) break;
        ActionType_t action = sc_map_get_64(&scene->action_map, button);
        if (!sc_map_found(&scene->action_map)) continue;
        do_action(scene, action, true);
        sc_queue_add_last(&engine->key_buffer, button);
    }
}

void change_scene(GameEngine_t* engine, unsigned int idx)
{
    engine->scenes[engine->curr_scene]->state = SCENE_ENDED;
    engine->curr_scene = idx;
    engine->scenes[engine->curr_scene]->state = SCENE_PLAYING;
}

//void init_scene(Scene_t* scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func)
void init_scene(Scene_t* scene, system_func_t render_func, action_func_t action_func)
{
    sc_map_init_64(&scene->action_map, 32, 0);
    sc_array_init(&scene->systems);
    init_entity_manager(&scene->ent_manager);

    //scene->scene_type = scene_type;
    scene->render_function = render_func;
    scene->action_function = action_func;
    scene->state = SCENE_ENDED;
}

void free_scene(Scene_t* scene)
{
    sc_map_term_64(&scene->action_map);
    sc_array_term(&scene->systems);
    free_entity_manager(&scene->ent_manager);
}

inline void update_scene(Scene_t* scene)
{
    system_func_t sys;
    sc_array_foreach(&scene->systems, sys)
    {
        sys(scene);
    }
}

inline void render_scene(Scene_t* scene)
{
    scene->render_function(scene);
}

inline void do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    scene->action_function(scene, action, pressed);
}
