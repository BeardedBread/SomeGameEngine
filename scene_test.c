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
    .assets = {0},
    .sfx_list = {0},
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
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    InitAudioDevice();
    init_engine(&engine);

#ifndef NDEBUG
    load_from_infofile("res/assets.info.raw", &engine.assets);
    init_player_creation("res/player_spr.info", &engine.assets);
#else
    load_from_rres("res/myresources.rres", &engine.assets);
    init_player_creation_rres("res/myresources.rres", "player_spr.info", &engine.assets);
#endif
    init_item_creation(&engine.assets);

    add_sound(&engine.assets, "snd_jump", "res/jump.ogg");
    add_sound(&engine.assets, "snd_land", "res/land.ogg");
    add_sound(&engine.assets, "snd_wdrop", "res/water_land.ogg");
    add_sound(&engine.assets, "snd_bland", "res/boulder_move.ogg");
    add_sound(&engine.assets, "snd_bubble", "res/bubble.ogg");
    load_sfx(&engine, "snd_jump", PLAYER_JMP_SFX);
    load_sfx(&engine, "snd_land", PLAYER_LAND_SFX);
    load_sfx(&engine, "snd_wdrop", WATER_IN_SFX);
    load_sfx(&engine, "snd_bland", BOULDER_LAND_SFX);
    load_sfx(&engine, "snd_bubble", BUBBLE_SFX);


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

    EmitterConfig_t* conf = add_emitter_conf(&engine.assets, "pe_burst", get_sprite(&engine.assets, "bomb"));
    conf->launch_range[0] = 240;
    conf->launch_range[1] = 300;
    conf->one_shot = true;
    conf->speed_range[0] = 200;
    conf->speed_range[1] = 300;
    conf->particle_lifetime[0] = 30;
    conf->particle_lifetime[1] = 110;

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
            update_sfx_list(&engine);
            if (WindowShouldClose()) break;
        }
    #endif
    free_sandbox_scene(&scene);
    deinit_engine(&engine);
    CloseWindow();

}
