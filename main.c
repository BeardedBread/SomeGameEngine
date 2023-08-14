#include "raylib.h"
#include "scene_impl.h"
#include "mempool.h"
#include <stdio.h>
#define N_SCENES 3

Scene_t *scenes[N_SCENES];
static GameEngine_t engine = {
    .scenes = scenes,
    .max_scenes = 2,
    .curr_scene = 0,
    .assets = {0}
};

const int screenWidth = 1280;
const int screenHeight = 640;

static void load_assets(void)
{
    Texture2D* tex = add_texture(&engine.assets, "plr_tex", "res/test_tex.png");
    Sprite_t* spr = add_sprite(&engine.assets, "plr_stand", tex);
    spr->origin = (Vector2){0, 0};
    spr->frame_size = (Vector2){32, 32};

    spr = add_sprite(&engine.assets, "plr_run", tex);
    spr->frame_count = 4;
    spr->origin = (Vector2){0, 0};
    spr->frame_size = (Vector2){32, 32};
    spr->speed = 15;
}

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    sc_queue_init(&key_buffer);
    InitWindow(screenWidth, screenHeight, "raylib");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    init_memory_pools();

    init_assets(&engine.assets);
    load_assets();

    LevelScene_t level_scene;
    level_scene.scene.engine = &engine;
    init_sandbox_scene(&level_scene);
    MenuScene_t menu_scene;
    menu_scene.scene.engine = &engine;
    init_menu_scene(&menu_scene);
    scenes[0] = &menu_scene.scene;
    scenes[1] = &level_scene.scene;
    change_scene(&engine, 0);

    //Camera2D camera = { 0 };
    //camera.offset = (Vector2){0,0};
    //camera.rotation = 0.0f;
    //camera.zoom = 1.0f;
    while (!WindowShouldClose())
    {
        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib
        Scene_t* curr_scene = engine.scenes[engine.curr_scene];

        unsigned int sz = sc_queue_size(&key_buffer);
        // Process any existing pressed key
        for (size_t i = 0; i < sz; i++)
        {
            int button = sc_queue_del_first(&key_buffer);
            ActionType_t action = sc_map_get_64(&curr_scene->action_map, button);
            if (IsKeyReleased(button))
            {
                do_action(curr_scene, action, false);
            }
            else
            {
                do_action(curr_scene, action, true);
                sc_queue_add_last(&key_buffer, button);
            }
        }

        // Detect new key presses
        while(true)
        {
            int button = GetKeyPressed();
            if (button == 0) break;
            ActionType_t action = sc_map_get_64(&curr_scene->action_map, button);
            if (!sc_map_found(&curr_scene->action_map)) continue;
            do_action(curr_scene, action, true);
            sc_queue_add_last(&key_buffer, button);
        }

        update_scene(curr_scene);
        update_entity_manager(&curr_scene->ent_manager);
        // This is needed to advance time delta
        render_scene(curr_scene);

        if (curr_scene->state != SCENE_PLAYING)
        {
            sc_queue_clear(&key_buffer);
        }
    }
    free_sandbox_scene(&level_scene);
    free_menu_scene(&menu_scene);
    sc_queue_term(&key_buffer);
    term_assets(&engine.assets);
    CloseWindow(); 
}
