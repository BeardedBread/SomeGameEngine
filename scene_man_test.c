#include "constants.h"
#include "scene_impl.h"
#include "ent_impl.h"
#include "water_flow.h"
#include "game_systems.h"
#include "assets_loader.h"
#include "raymath.h"
#include <stdio.h>
#include <unistd.h>

static Tile_t all_tiles[MAX_N_TILES] = {0};

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

Scene_t* scenes[6];
static GameEngine_t engine =
{
    .scenes = scenes,
    .max_scenes = 6,
    .curr_scene = 0,
    .assets = {0}
};

#define GAME_LAYER 0
struct DummyScene {
    Scene_t scene;
    unsigned int number;
    float elapsed;
    Vector2 text_pos;
};


static void level_scene_render_func(Scene_t* scene)
{
    struct DummyScene* data = CONTAINER_OF(scene, struct DummyScene, scene);
    char text[32];
    sprintf(text, "Scene %u", data->number);
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
    DrawText(text, 32 * data->number, 32 * data->number, 12, BLACK);
    EndTextureMode();
}

static inline unsigned int get_tile_idx(int x, int y, const TileGrid_t* tilemap)
{
    unsigned int tile_x = x / TILE_SIZE;
    unsigned int tile_y = y / TILE_SIZE;

    if (tile_x < tilemap->width && tile_y < tilemap->height)
    {
        return tile_y * tilemap->width + tile_x;
    }

    return MAX_N_TILES;
}
static void print_number_sys(Scene_t* scene)
{
    struct DummyScene* data = CONTAINER_OF(scene, struct DummyScene, scene);

    data->elapsed += scene->delta_time;
    if (data->elapsed > 1.0f)
    {
        printf("Data: %u\n", data->number);
        data->elapsed -= 1.0f;
    }
}

static void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    CPlayerState_t* p_playerstate;
    sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    {
        switch(action)
        {
            case ACTION_UP:
                p_playerstate->player_dir.y = (pressed)? -1 : 0;
            break;
            case ACTION_DOWN:
                p_playerstate->player_dir.y = (pressed)? 1 : 0;
            break;
            case ACTION_LEFT:
                p_playerstate->player_dir.x = (pressed)? -1 : 0;
            break;
            case ACTION_RIGHT:
                p_playerstate->player_dir.x = (pressed)? 1 : 0;
            break;
            default:
            break;
        }
    }
    switch (action)
    {
        case ACTION_RESTART:
            puts("Restarting!");
        break;
        default:
        break;
    }
}

int main(void)
{
    init_engine(&engine, (Vector2){1280,640});
    SetTargetFPS(60);

    // TODO: Add render function
    // Add a way to switch focused scene
    static struct DummyScene dummy_scenes[6];
    for (uint8_t i = 0; i < 6; ++i)
    {
        scenes[i] = &dummy_scenes[i].scene;
        init_scene(&dummy_scenes[i].scene, &level_do_action);
        dummy_scenes[i].scene.engine = &engine;
        dummy_scenes[i].number = i;
        add_scene_layer(
            &dummy_scenes[i].scene, 1280, 640, (Rectangle){0,0,1280,640}
        );
        dummy_scenes[i].scene.bg_colour = WHITE;
        sc_array_add(&dummy_scenes[i].scene.systems, &print_number_sys);
        sc_array_add(&dummy_scenes[i].scene.systems, &level_scene_render_func);
        //sc_map_put_64(&scene.scene.action_map, KEY_R, ACTION_RESTART);
        //sc_map_put_64(&scene.scene.action_map, KEY_UP, ACTION_UP);
        //sc_map_put_64(&scene.scene.action_map, KEY_DOWN, ACTION_DOWN);
        //sc_map_put_64(&scene.scene.action_map, KEY_LEFT, ACTION_LEFT);
        //sc_map_put_64(&scene.scene.action_map, KEY_RIGHT, ACTION_RIGHT);
        //sc_map_put_64(&scene.scene.action_map, KEY_P, ACTION_METAL_TOGGLE);
    }

    change_active_scene(&engine, 0);
    add_child_scene(&engine, 1, 0);
    add_child_scene(&engine, 2, 0);
    add_child_scene(&engine, 3, 0);

    float timer = 0;
    while(timer < 1.2f)
    {
        timer += GetFrameTime();
        process_active_scene_inputs(&engine);
        update_curr_scene(&engine);
        // This is needed to advance time delta
        render_curr_scene(&engine);
        if (WindowShouldClose()) break;
    }

    remove_child_scene(&engine, 2);
    timer = 0;
    while(timer < 1.2f)
    {
        timer += GetFrameTime();
        process_active_scene_inputs(&engine);
        update_curr_scene(&engine);
        // This is needed to advance time delta
        render_curr_scene(&engine);
        if (WindowShouldClose()) break;
    }

    add_child_scene(&engine, 4, 0);
    timer = 0;
    while(timer < 1.2f)
    {
        timer += GetFrameTime();
        process_active_scene_inputs(&engine);
        update_curr_scene(&engine);
        // This is needed to advance time delta
        render_curr_scene(&engine);
        if (WindowShouldClose()) break;
    }

    add_child_scene(&engine, 2, 1);
    timer = 0;
    while(timer < 1.2f)
    {
        timer += GetFrameTime();
        process_active_scene_inputs(&engine);
        update_curr_scene(&engine);
        // This is needed to advance time delta
        render_curr_scene(&engine);
        if (WindowShouldClose()) break;
    }


    for (uint8_t i = 0; i < 6; ++i)
    {
        free_scene(&dummy_scenes[i].scene);
    }
    deinit_engine(&engine);
}
