#include "mempool.h"
#include "scene_impl.h"
#include "assets_loader.h"
#include <stdio.h>
#include <unistd.h>

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

Scene_t* scenes[1];
static GameEngine_t engine =
{
    .scenes = scenes,
    .max_scenes = 1,
    .curr_scene = 0,
    .assets = {0}
};

int main(void)
{
    sc_queue_init(&key_buffer);
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_memory_pools();

    init_assets(&engine.assets);
    load_from_infofile("res/assets.info", &engine.assets);

    LevelScene_t scene;
    scene.scene.engine = &engine;
    init_level_scene(&scene);
    scenes[0] = &scene.scene;
    change_scene(&engine, 0);

    while(true)
    {

        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib

        unsigned int sz = sc_queue_size(&key_buffer);
        // Process any existing pressed key
        for (size_t i = 0; i < sz; i++)
        {
            int button = sc_queue_del_first(&key_buffer);
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (IsKeyReleased(button))
            {
                do_action(&scene.scene, action, false);
            }
            else
            {
                do_action(&scene.scene, action, true);
                sc_queue_add_last(&key_buffer, button);
            }
        }

        // Detect new key presses
        while(true)
        {
            int button = GetKeyPressed();
            if (button == 0) break;
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (!sc_map_found(&scene.scene.action_map)) continue;
            do_action(&scene.scene, action, true);
            sc_queue_add_last(&key_buffer, button);
        }

        update_scene(&scene.scene);
        update_entity_manager(&scene.scene.ent_manager);
        // This is needed to advance time delta
        render_scene(&scene.scene);
        if (WindowShouldClose()) break;
    } 
    free_level_scene(&scene);
    sc_queue_term(&key_buffer);
    term_assets(&engine.assets);
    CloseWindow();
}
