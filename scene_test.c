#include "mempool.h"
#include "scene_impl.h"
#include "ent_impl.h"
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
    init_player_creation("res/player_spr.info", &engine.assets);
    init_item_creation(&engine.assets);

    LevelScene_t scene;
    scene.scene.engine = &engine;
    init_sandbox_scene(&scene);
    scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    scene.data.tile_sprites[SPIKES] = get_sprite(&engine.assets, "d_spikes");
    scene.data.tile_sprites[SPIKES + TILE_90CWROT] = get_sprite(&engine.assets, "l_spikes");
    scene.data.tile_sprites[SPIKES + TILE_90CCWROT] = get_sprite(&engine.assets, "r_spikes");
    scene.data.tile_sprites[SPIKES + TILE_180ROT] = get_sprite(&engine.assets, "u_spikes");
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
    free_sandbox_scene(&scene);
    sc_queue_term(&key_buffer);
    term_assets(&engine.assets);
    CloseWindow();
}
