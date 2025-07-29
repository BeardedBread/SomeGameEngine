#include "raylib.h"
#include "assets_loader.h"
#include "scene_impl.h"
#include "ent_impl.h"
#include "mempool.h"
#include "constants.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "tracy/TracyC.h"
#define N_SCENES 1

Scene_t *scenes[N_SCENES];
static GameEngine_t engine = {
    .scenes = scenes,
    .max_scenes = 1,
    .curr_scene = 0,
    .assets = {0}
};

const int screenWidth = VIEWABLE_MAP_WIDTH * TILE_SIZE;
const int screenHeight = VIEWABLE_MAP_HEIGHT * TILE_SIZE;

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

int main(int argc, char** argv) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    init_engine(&engine, (Vector2){screenWidth, screenHeight});
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    load_from_infofile("res/assets.info.raw", &engine.assets);
    init_player_creation("res/player_spr.info", &engine.assets);
    init_item_creation(&engine.assets);

    load_sfx(&engine, "snd_jump", PLAYER_JMP_SFX);
    load_sfx(&engine, "snd_land", PLAYER_LAND_SFX);
    load_sfx(&engine, "snd_wdrop", WATER_IN_SFX);
    load_sfx(&engine, "snd_bland", BOULDER_LAND_SFX);
    load_sfx(&engine, "snd_bubble", BUBBLE_SFX);
    load_sfx(&engine, "snd_step", PLAYER_STEP_SFX);
    load_sfx(&engine, "snd_dead", PLAYER_DEAD_SFX);
    load_sfx(&engine, "snd_drwg", PLAYER_DROWNING_SFX);
    load_sfx(&engine, "snd_climb", PLAYER_CLIMB_SFX);
    load_sfx(&engine, "snd_mdestroy", METAL_DESTROY_SFX);
    load_sfx(&engine, "snd_wdestroy", WOOD_DESTROY_SFX);
    load_sfx(&engine, "snd_cland", WOOD_LAND_SFX);
    load_sfx(&engine, "snd_explsn", EXPLOSION_SFX);
    load_sfx(&engine, "snd_coin", COIN_SFX);
    load_sfx(&engine, "snd_arrhit", ARROW_DESTROY_SFX);
    load_sfx(&engine, "snd_launch", ARROW_RELEASE_SFX);
    load_sfx(&engine, "snd_launch", BOMB_RELEASE_SFX);

    LevelScene_t level_scene;
    level_scene.scene.engine = &engine;
    init_game_scene(&level_scene);
    level_scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    level_scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    level_scene.data.tile_sprites[SPIKES] = get_sprite(&engine.assets, "d_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_90CWROT] = get_sprite(&engine.assets, "l_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_90CCWROT] = get_sprite(&engine.assets, "r_spikes");
    level_scene.data.tile_sprites[SPIKES + TILE_180ROT] = get_sprite(&engine.assets, "u_spikes");
    Texture2D* tex = get_texture(&engine.assets, "bg_tex");
    SetTextureWrap(*tex, TEXTURE_WRAP_REPEAT);
    sc_map_del_64(&level_scene.scene.action_map, ACTION_EXIT);

    // Load level
    LevelPack_t* pack = get_level_pack(&engine.assets, "DefLevels");
    if (pack == NULL)
    {
        puts("Default level pack not found!");
        return 1;
    }
    unsigned int selected_level = 0;
    if (argc == 2) {
        selected_level = strtoul(argv[1], NULL, 10);
        printf("Selected level: %u", selected_level);
    }
    if (selected_level >= pack->n_levels) {
        printf("Level numbers out of bound. Picking 0");
        selected_level = 0;
    }
    level_scene.data.level_pack = pack;
    level_scene.data.current_level = selected_level;
    scenes[0] = &level_scene.scene;
    reload_level_tilemap(&level_scene);
    change_scene(&engine, 0);

    const float DT = 1.0f/60.0f;
    while (!WindowShouldClose())
    {
        TracyCFrameMark;
        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib
        Scene_t* curr_scene = engine.scenes[engine.curr_scene];

        if (curr_scene->state == 0 && engine.curr_scene == 0)
        {
            break;
        }

        process_inputs(&engine, curr_scene);


        float frame_time = GetFrameTime();
        float delta_time = fminf(frame_time, DT);

        {
            TracyCZoneN(ctx, "Update", true)
            update_scene(curr_scene, delta_time);
            update_entity_manager(&curr_scene->ent_manager);
            update_sfx_list(&engine);
            TracyCZoneEnd(ctx)
        }

        // This is needed to advance time delta
        render_scene(curr_scene);

        if (curr_scene->state != 0)
        {
            sc_queue_clear(&key_buffer);
        }
    }
    free_game_scene(&level_scene);
    deinit_engine(&engine);
}
