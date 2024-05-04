#include "raylib.h"
#include "assets_loader.h"
#include "scene_impl.h"
#include "ent_impl.h"
#include "mempool.h"
#include <stdio.h>
#include <math.h>
#define N_SCENES 4

Scene_t *scenes[N_SCENES];
static GameEngine_t engine = {
    .scenes = scenes,
    .max_scenes = 3,
    .curr_scene = 0,
    .assets = {0}
};

const int screenWidth = 1280;
const int screenHeight = 640;

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    init_engine(&engine, (Vector2){screenWidth, screenHeight});
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
#ifndef NDEBUG
    load_from_infofile("res/assets.info.raw", &engine.assets);
    init_player_creation("res/player_spr.info", &engine.assets);
#else
    load_from_rres("res/myresources.rres", &engine.assets);
    init_player_creation_rres("res/myresources.rres", "player_spr.info", &engine.assets);
#endif
    init_item_creation(&engine.assets);

    load_sfx(&engine, "snd_jump", PLAYER_JMP_SFX);
    load_sfx(&engine, "snd_land", PLAYER_LAND_SFX);
    load_sfx(&engine, "snd_wdrop", WATER_IN_SFX);
    load_sfx(&engine, "snd_bland", BOULDER_LAND_SFX);
    load_sfx(&engine, "snd_bubble", BUBBLE_SFX);
    load_sfx(&engine, "snd_mdestroy", METAL_DESTROY_SFX);
    load_sfx(&engine, "snd_wdestroy", WOOD_DESTROY_SFX);
    load_sfx(&engine, "snd_cland", WOOD_LAND_SFX);
    load_sfx(&engine, "snd_explsn", EXPLOSION_SFX);
    load_sfx(&engine, "snd_coin", COIN_SFX);
    load_sfx(&engine, "snd_arrhit", ARROW_DESTROY_SFX);
    load_sfx(&engine, "snd_launch", ARROW_RELEASE_SFX);
    load_sfx(&engine, "snd_launch", BOMB_RELEASE_SFX);

    LevelScene_t sandbox_scene;
    sandbox_scene.scene.engine = &engine;
    init_sandbox_scene(&sandbox_scene);

    LevelScene_t level_scene;
    level_scene.scene.engine = &engine;
    init_game_scene(&level_scene);
    level_scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    level_scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    level_scene.data.tile_sprites[SPIKES] = get_sprite(&engine.assets, "d_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_90CWROT] = get_sprite(&engine.assets, "l_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_90CCWROT] = get_sprite(&engine.assets, "r_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_180ROT] = get_sprite(&engine.assets, "u_spikes");
    LevelPack_t* pack = get_level_pack(&engine.assets, "TestLevels");
    if (pack != NULL)
    {
        level_scene.data.level_pack = pack;
        level_scene.data.current_level = 0;
        load_level_tilemap(&level_scene, 0);
    }

    MenuScene_t menu_scene;
    menu_scene.scene.engine = &engine;
    init_menu_scene(&menu_scene);
    scenes[0] = &menu_scene.scene;
    scenes[1] = &level_scene.scene;
    scenes[2] = &sandbox_scene.scene;
    change_scene(&engine, 0);

    const float DT = 1.0f/60.0f;
    while (!WindowShouldClose())
    {
        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib
        Scene_t* curr_scene = engine.scenes[engine.curr_scene];

        if (curr_scene->state == SCENE_ENDED && engine.curr_scene == 0)
        {
            break;
        }

        process_inputs(&engine, curr_scene);


        float frame_time = GetFrameTime();
        float delta_time = fminf(frame_time, DT);

        update_scene(curr_scene, delta_time);
        update_entity_manager(&curr_scene->ent_manager);
        // This is needed to advance time delta
        render_scene(curr_scene);
        update_sfx_list(&engine);

        if (curr_scene->state != SCENE_PLAYING)
        {
            sc_queue_clear(&key_buffer);
        }

    }
    free_sandbox_scene(&sandbox_scene);
    free_game_scene(&level_scene);
    free_menu_scene(&menu_scene);
    deinit_engine(&engine);
}
