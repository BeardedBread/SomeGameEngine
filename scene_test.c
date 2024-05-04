#include "scene_impl.h"
#include "ent_impl.h"
#include "assets_loader.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    EM_BOOL keyDownCallback(int eventType, const EmscriptenKeyboardEvent *event, void* userData) {
        return true; // Just preventDefault everything lol
    }
#endif
const float DT = 1.0f/60.0f;

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

    float frame_time = GetFrameTime();
    float delta_time = fminf(frame_time, DT);
    update_scene(scene, delta_time);
    update_entity_manager(&scene->ent_manager);
    // This is needed to advance time delta
    render_scene(scene);
    update_sfx_list(&engine);
}

int main(void)
{
    init_engine(&engine, (Vector2){1280,640});
    SetTargetFPS(60);

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

    LevelScene_t scene;
    scene.scene.engine = &engine;
    init_sandbox_scene(&scene);
    scenes[0] = &scene.scene;
    change_scene(&engine, 0);

    #if defined(PLATFORM_WEB)
        puts("Setting emscripten main loop");
        emscripten_set_keypress_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_keydown_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_main_loop(update_loop, 0, 1);
    #else
        puts("Regular main loop");
        const float DT = 1.0f/60.0f;
        while(true)
        {

            // This entire key processing relies on the assumption that a pressed key will
            // appear in the polling of raylib
            process_inputs(&engine, &scene.scene);

            float frame_time = GetFrameTime();
            float delta_time = fminf(frame_time, DT);

            update_scene(&scene.scene, delta_time);
            update_entity_manager(&scene.scene.ent_manager);
            // This is needed to advance time delta
            render_scene(&scene.scene);
            update_sfx_list(&engine);
            if (WindowShouldClose()) break;
        }
    #endif
    free_sandbox_scene(&scene);
    deinit_engine(&engine);
}
