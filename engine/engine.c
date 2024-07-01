#include "engine.h"
#include "mempool.h"

#include <math.h>

void init_engine(GameEngine_t* engine, Vector2 starting_win_size)
{
    InitAudioDevice();
    sc_queue_init(&engine->key_buffer);
    sc_queue_init(&engine->scene_stack);
    sc_heap_init(&engine->scenes_render_order, 0);
    engine->sfx_list.n_sfx = N_SFX;
    memset(engine->sfx_list.sfx, 0, engine->sfx_list.n_sfx * sizeof(SFX_t));
    init_memory_pools();
    init_assets(&engine->assets);
    engine->intended_window_size = starting_win_size;
    InitWindow(starting_win_size.x, starting_win_size.y, "raylib");
}

void deinit_engine(GameEngine_t* engine)
{
    term_assets(&engine->assets);
    free_memory_pools();
    sc_queue_term(&engine->key_buffer);
    sc_queue_term(&engine->scene_stack);
    sc_heap_term(&engine->scenes_render_order);
    CloseAudioDevice();
    CloseWindow(); 
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
    // Backwards compat
    change_active_scene(engine, idx);
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

void init_scene(Scene_t* scene, action_func_t action_func)
{
    sc_map_init_64(&scene->action_map, 32, 0);
    sc_array_init(&scene->systems);
    init_entity_manager(&scene->ent_manager);
    init_particle_system(&scene->part_sys);

    //scene->scene_type = scene_type;
    scene->layers.n_layers = 0;
    scene->bg_colour = WHITE;

    scene->action_function = action_func;
    scene->state = SCENE_COMPLETE_ACTIVE;
    scene->time_scale = 1.0f;
}

bool add_scene_layer(Scene_t* scene, int width, int height, Rectangle render_area)
{
    if (scene->layers.n_layers >= MAX_RENDER_LAYERS) return false;

    scene->layers.render_layers[scene->layers.n_layers].layer_tex = LoadRenderTexture(width, height);
    scene->layers.render_layers[scene->layers.n_layers].render_area = render_area;
    scene->layers.n_layers++;
    return true;
}

void free_scene(Scene_t* scene)
{
    sc_map_term_64(&scene->action_map);
    sc_array_term(&scene->systems);
    for (uint8_t i = 0; i < scene->layers.n_layers; ++i)
    {
        UnloadRenderTexture(scene->layers.render_layers[i].layer_tex);
    }
    free_entity_manager(&scene->ent_manager);
    deinit_particle_system(&scene->part_sys);
}

inline void update_scene(Scene_t* scene, float delta_time)
{
    if ((scene->state & SCENE_ACTIVE_BIT) == 0) return;

    scene->delta_time = delta_time * scene->time_scale;
    system_func_t sys;
    sc_array_foreach(&scene->systems, sys)
    {
        sys(scene);
    }
    update_particle_system(&scene->part_sys, scene->delta_time);
}

static void _internal_render_scene(Scene_t* scene)
{
    if ((scene->state & SCENE_RENDER_BIT) == 0) return;

    if (scene->parent_scene == NULL)
    {
        ClearBackground(scene->bg_colour);
    }
    else
    {
        ClearBackground((Color){255,255,255,0});
    }

    for (uint8_t i = 0; i < scene->layers.n_layers; ++i)
    {
        RenderLayer_t* layer = scene->layers.render_layers + i;
        Rectangle draw_rec = layer->render_area;
        Vector2 draw_pos = {draw_rec.x, draw_rec.y};
        draw_rec.x = 0;
        draw_rec.y = 0;
        draw_rec.height *= -1;
        DrawTextureRec(
            layer->layer_tex.texture,
            draw_rec,
            draw_pos,
            WHITE
        );
    }
}

inline void render_scene(Scene_t* scene)
{
    BeginDrawing();
    _internal_render_scene(scene);
    EndDrawing();
}

inline void do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    scene->action_function(scene, action, pressed);
}

void process_active_scene_inputs(GameEngine_t* engine)
{
    if (engine->focused_scene == NULL) return;

    process_inputs(engine, engine->focused_scene);
}

void update_curr_scene(GameEngine_t* engine)
{
    if (engine->curr_scene == engine->max_scenes) return;
    const float DT = 1.0f/60.0f;
    float frame_time = GetFrameTime();
    float delta_time = fminf(frame_time, DT);

    sc_queue_clear(&engine->scene_stack);
    sc_heap_clear(&engine->scenes_render_order);

    sc_queue_add_first(&engine->scene_stack, engine->scenes[engine->curr_scene]);
    
    while (!sc_queue_empty(&engine->scene_stack))
    {
        Scene_t* scene = sc_queue_del_first(&engine->scene_stack);

        update_scene(scene, delta_time);

        if (scene->child_scene.next != NULL)
        {
            sc_queue_add_first(&engine->scene_stack, scene->child_scene.next);
        }
        if (scene->child_scene.scene != NULL)
        {
            sc_queue_add_first(&engine->scene_stack, scene->child_scene.scene);
        }
        
		sc_heap_add(&engine->scenes_render_order, scene->depth_index, scene);
    }
}

void render_curr_scene(GameEngine_t* engine)
{
	struct sc_heap_data *elem;
    BeginDrawing();
    while ((elem = sc_heap_pop(&engine->scenes_render_order)) != NULL)
    {
        Scene_t* scene = elem->data;

        _internal_render_scene(scene);
	}
    EndDrawing();
}

void add_child_scene(GameEngine_t* engine, unsigned int child_idx, unsigned int parent_idx)
{
    if (
        child_idx >= engine->max_scenes
        || parent_idx >= engine->max_scenes
    ) return;

    Scene_t* child = engine->scenes[child_idx];
    Scene_t* parent = engine->scenes[parent_idx];

    if (parent == NULL) return;

    if (parent->child_scene.scene == NULL)
    {
        parent->child_scene.scene = child;
    }
    else
    {
        Scene_t* curr = parent->child_scene.scene;

        while (curr->child_scene.next != NULL)
        {
            curr = curr->child_scene.next;
        }
        curr->child_scene.next = child;
    }

    child->parent_scene = parent;
}

void remove_child_scene(GameEngine_t* engine, unsigned int idx)
{
    if (idx >= engine->max_scenes) return;

    Scene_t* child = engine->scenes[idx];
    if (child == NULL) return;

    if (child->parent_scene == NULL) return;

    Scene_t* parent = child->parent_scene;
    if (parent->child_scene.scene == NULL) return;

    Scene_t* prev = NULL;
    Scene_t* curr = parent->child_scene.scene;

    while (curr != NULL)
    {
        if (curr == child)
        {
            if (prev != NULL)
            {
                prev->child_scene.next = curr->child_scene.next;
            }
            else
            {
                parent->child_scene.scene = curr->child_scene.next;
            }
            break;
        }

        prev = curr;
        curr = curr->child_scene.next;
    }

    child->parent_scene = NULL;
}

void change_active_scene(GameEngine_t* engine, unsigned int idx)
{
    engine->scenes[engine->curr_scene]->state = 0;
    engine->curr_scene = idx;
    engine->scenes[engine->curr_scene]->state = SCENE_COMPLETE_ACTIVE;
}

void change_focused_scene(GameEngine_t* engine, unsigned int idx)
{
    engine->focused_scene = engine->scenes[idx];
}
