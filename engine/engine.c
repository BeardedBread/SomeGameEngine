#include "engine.h"
#include "mempool.h"

void init_engine(GameEngine_t* engine)
{
    InitAudioDevice();
    sc_queue_init(&engine->key_buffer);
    engine->sfx_list.n_sfx = N_SFX;
    memset(engine->sfx_list.sfx, 0, engine->sfx_list.n_sfx * sizeof(SFX_t));
    init_memory_pools();
    init_assets(&engine->assets);
}

void deinit_engine(GameEngine_t* engine)
{
    term_assets(&engine->assets);
    free_memory_pools();
    sc_queue_term(&engine->key_buffer);
    CloseAudioDevice();
}

void process_inputs(GameEngine_t* engine, Scene_t* scene)
{
    Vector2 raw_mouse_pos = GetMousePosition();
    scene->mouse_pos = raw_mouse_pos;

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

    
    // Mouse button handling
    ActionType_t action = sc_map_get_64(&scene->action_map, MOUSE_BUTTON_RIGHT);
    if (sc_map_found(&scene->action_map))
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            do_action(scene, action, true);
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            do_action(scene, action, false);
        }
    }
    action = sc_map_get_64(&scene->action_map, MOUSE_BUTTON_LEFT);
    if (sc_map_found(&scene->action_map))
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            do_action(scene, action, true);
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            do_action(scene, action, false);
        }
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
    return true;
}

void play_sfx_pitched(GameEngine_t* engine, unsigned int tag_idx, float pitch)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;
    if (sfx->snd != NULL)
    {
        SetSoundPitch(*sfx->snd, pitch);
        //if (sfx->snd != NULL)
        {
            PlaySound(*sfx->snd);
            sfx->plays++;
        }
        //SetSoundPitch(*sfx->snd, 0.0f);
    }
}

void play_sfx(GameEngine_t* engine, unsigned int tag_idx)
{
    play_sfx_pitched(engine, tag_idx, 0.0f);
}

void stop_sfx(GameEngine_t* engine, unsigned int tag_idx)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;
    if (sfx->snd != NULL && IsSoundPlaying(*sfx->snd))
    {
        StopSound(*sfx->snd);
        //sfx->plays--;
    }
}

void update_sfx_list(GameEngine_t* engine)
{
    for (uint32_t i = 0; i< engine->sfx_list.n_sfx; ++i)
    {
        if (!IsSoundPlaying(*engine->sfx_list.sfx->snd))
        {
            engine->sfx_list.sfx[i].plays = 0;
        }
    }
    engine->sfx_list.played_sfx = 0;
}

//void init_scene(Scene_t* scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func)
void init_scene(Scene_t* scene, render_func_t render_func, action_func_t action_func)
{
    sc_map_init_64(&scene->action_map, 32, 0);
    sc_array_init(&scene->systems);
    init_entity_manager(&scene->ent_manager);
    init_particle_system(&scene->part_sys);

    //scene->scene_type = scene_type;
    scene->render_function = render_func;
    scene->action_function = action_func;
    scene->state = SCENE_ENDED;
    scene->time_scale = 1.0f;
}

void free_scene(Scene_t* scene)
{
    sc_map_term_64(&scene->action_map);
    sc_array_term(&scene->systems);
    free_entity_manager(&scene->ent_manager);
    deinit_particle_system(&scene->part_sys);
}

inline void update_scene(Scene_t* scene, float delta_time)
{
    scene->delta_time = delta_time * scene->time_scale;
    system_func_t sys;
    sc_array_foreach(&scene->systems, sys)
    {
        sys(scene);
    }
    update_particle_system(&scene->part_sys, scene->delta_time);
}

inline void render_scene(Scene_t* scene)
{
    if (scene->render_function != NULL)
    {
        scene->render_function(scene);
    }
}

inline void do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    scene->action_function(scene, action, pressed);
}
