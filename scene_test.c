#include "mempool.h"
#include "scene_impl.h"
#include "ent_impl.h"
#include "assets_loader.h"
#include <stdio.h>
#include <unistd.h>
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif


Scene_t* scenes[1];
static GameEngine_t engine =
{
    .scenes = scenes,
    .max_scenes = 1,
    .curr_scene = 0,
    .assets = {0}
};

void update_loop(void)
{
    Scene_t* scene = engine.scenes[engine.curr_scene];
    process_inputs(&engine, scene);

    update_scene(scene);
    update_entity_manager(&scene->ent_manager);
    // This is needed to advance time delta
    render_scene(scene);
}

int main(void)
{
    init_engine(&engine);
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_memory_pools();

    init_assets(&engine.assets);
    //load_from_infofile("res/assets.info.raw", &engine.assets);
    //init_player_creation("res/player_spr.info", &engine.assets);
    load_from_rres("res/myresources.rres", &engine.assets);
    init_player_creation_rres("res/myresources.rres", "player_spr.info", &engine.assets);
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

    #if defined(PLATFORM_WEB)
        puts("Setting emscripten main loop");
        emscripten_set_main_loop(update_loop, 0, 1);
    #else
        puts("Regular main loop");
        while(true)
        {

            // This entire key processing relies on the assumption that a pressed key will
            // appear in the polling of raylib
            process_inputs(&engine, &scene.scene);

            update_scene(&scene.scene);
            update_entity_manager(&scene.scene.ent_manager);
            // This is needed to advance time delta
            render_scene(&scene.scene);
            if (WindowShouldClose()) break;
        }
    #endif
    free_sandbox_scene(&scene);
    deinit_engine(&engine);
    term_assets(&engine.assets);
    CloseWindow();

}
