#include "constants.h"
#include "mempool.h"
#include "raymath.h"
#include "scene_impl.h"
#include "ent_impl.h"
#include "water_flow.h"
#include "game_systems.h"
#include "assets_loader.h"
#include <stdio.h>
#include <unistd.h>

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
    init_engine(&engine);
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_memory_pools();

    init_assets(&engine.assets);
    load_from_infofile("res/test_assets.info", &engine.assets);
    LevelPack_t* pack = get_level_pack(&engine.assets, "TestLevels");
    assert(pack != NULL);

    LevelScene_t scene;
    scene.scene.engine = &engine;
    scene.data.level_pack = pack;
    scene.data.current_level = 0;

    init_game_scene(&scene);
    assert(load_level_tilemap(&scene, 0) == true);

    scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    scenes[0] = &scene.scene;
    change_scene(&engine, 0);

    while(true)
    {
        process_inputs(&engine, &scene.scene);
        update_scene(&scene.scene);
        update_entity_manager(&scene.scene.ent_manager);
        // This is needed to advance time delta
        render_scene(&scene.scene);
        if (WindowShouldClose()) break;
    }

    unsigned int m_id;
    Entity_t* ent;
    sc_map_foreach(&scene.scene.ent_manager.entities_map[DYNMEM_ENT_TAG], m_id, ent)
    {
        free_water_runner(ent, &scene.scene.ent_manager);
    }
    free_scene(&scene.scene);
    term_level_scene_data(&scene.data);
    deinit_engine(&engine);
    term_assets(&engine.assets);
    CloseWindow();
}
