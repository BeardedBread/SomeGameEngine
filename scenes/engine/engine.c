#include "engine.h"

void init_engine(GameEngine_t* engine)
{
    sc_queue_init(&engine->key_buffer);
    memset(engine->sfx_list.sfx, 0, engine->sfx_list.n_sfx * sizeof(SFX_t));
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

bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return false;
    Sound* snd = get_sound(&engine->assets, snd_name);
    if (snd == NULL) return false;
    engine->sfx_list.sfx[tag_idx].snd = snd;
    engine->sfx_list.sfx[tag_idx].cooldown = 0;
    engine->sfx_list.sfx[tag_idx].plays = 0;
}

void play_sfx(GameEngine_t* engine, unsigned int tag_idx)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;
    if (sfx->plays == 0 && sfx->snd != NULL)
    {
        PlaySound(*sfx->snd);
        sfx->plays++;
        engine->sfx_list.sfx_queue[engine->sfx_list.played_sfx++] = tag_idx;
    }
}

void update_sfx_list(GameEngine_t* engine)
{
    for (uint32_t i = 0; i< engine->sfx_list.played_sfx; ++i)
    {
        uint32_t tag_idx = engine->sfx_list.sfx_queue[i];
        engine->sfx_list.sfx[tag_idx].plays = 0;
    }
    engine->sfx_list.played_sfx = 0;
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
