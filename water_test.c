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

#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

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

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_ent;

    BeginTextureMode(data->game_viewport);
        ClearBackground(WHITE);
        BeginMode2D(data->cam);
        for (size_t i = 0; i < tilemap.n_tiles; ++i)
        {
            char buffer[6] = {0};
            int x = (i % tilemap.width) * TILE_SIZE;
            int y = (i / tilemap.width) * TILE_SIZE;
            sprintf(buffer, "%u", sc_map_size_64v(&tilemap.tiles[i].entities_set));

            if (!tilemap.tiles[i].moveable)
            {
                // Draw water tile
                Color water_colour = ColorAlpha(RED, 0.2);
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, water_colour);
            }

            if (tilemap.tiles[i].tile_type == SOLID_TILE)
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
            }

            if (tilemap.tiles[i].water_level > 0)
            {
                // Draw water tile
                Color water_colour = ColorAlpha(BLUE, 0.5);
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, water_colour);
            }
        }

        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
            if (p_ct == NULL) continue;


            CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
            if (p_cspr != NULL)
            {
                const SpriteRenderInfo_t spr = p_cspr->sprites[p_cspr->current_idx];
                if (spr.sprite != NULL)
                {
                    Vector2 pos = Vector2Add(p_ct->position, spr.offset);
                    draw_sprite(spr.sprite, pos, p_cspr->flip_x);
                }
            }
        }
        sc_map_foreach_value(&scene->ent_manager.entities_map[DYNMEM_ENT_TAG], p_ent)
        {
            CWaterRunner_t* p_runner = get_component(p_ent, CWATERRUNNER_T);

            unsigned int x = ((p_runner->current_tile) % tilemap.width) * tilemap.tile_size; 
            unsigned int y = ((p_runner->current_tile) / tilemap.width) * tilemap.tile_size; 
            DrawCircle(x+16, y+16, 8, ColorAlpha(BLACK, 0.2));
            if (p_runner->target_tile < p_runner->bfs_tilemap.len)
            {
                unsigned int x = ((p_runner->target_tile) % tilemap.width) * tilemap.tile_size; 
                unsigned int y = ((p_runner->target_tile) / tilemap.width) * tilemap.tile_size; 
                DrawCircle(x+16, y+16, 8, ColorAlpha(BLUE, 0.2));
            }
        }

        char buffer[64] = {0};
        for (size_t i = 0; i < tilemap.n_tiles; ++i)
        {
            int x = (i % tilemap.width) * TILE_SIZE;
            int y = (i / tilemap.width) * TILE_SIZE;
            sprintf(buffer, "%u", sc_map_size_64v(&tilemap.tiles[i].entities_set));

            if (tilemap.tiles[i].solid > 0)
            {
                DrawText(buffer, x, y, 10, WHITE);
            }
            else
            {
                // Draw water tile
                DrawText(buffer, x, y, 10, BLACK);
            }
        }

        // Draw tile grid
        for (size_t i = 0; i < tilemap.width; ++i)
        {
            int x = (i+1)*TILE_SIZE;
            DrawLine(x, 0, x, tilemap.height * TILE_SIZE, BLACK);
        }
        for (size_t i = 0; i < tilemap.height;++i)
        {
            int y = (i+1)*TILE_SIZE;
            DrawLine(0, y, tilemap.width * TILE_SIZE, y, BLACK);
        }
        EndMode2D();
    EndTextureMode();

    Rectangle draw_rec = data->game_rec;
    draw_rec.x = 0;
    draw_rec.y = 0;
    draw_rec.height *= -1;
    BeginDrawing();
        ClearBackground(LIGHTGRAY);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );
    EndDrawing();
}
#define MAX_N_TILES 4096

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

static void simple_friction_system(Scene_t* scene)
{
    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        p_ctransform->fric_coeff = (Vector2){-4.5, -4.5};
        p_ctransform->accel = Vector2Add(
            p_ctransform->accel,
            Vector2Multiply((Vector2){-3.0,-3.0}, p_ctransform->velocity)
        );
    }
}

static void toggle_block_system(Scene_t* scene)
{
    // TODO: This system is not good as the interface between raw input and actions is broken
    static unsigned int last_tile_idx = MAX_N_TILES;
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    Vector2 raw_mouse_pos = {GetMouseX(), GetMouseY()};
    raw_mouse_pos = Vector2Subtract(raw_mouse_pos, (Vector2){data->game_rec.x, data->game_rec.y});

    if (
        raw_mouse_pos.x < data->game_rec.width
        && raw_mouse_pos.y < data->game_rec.height
    ) 
    {
        Vector2 mouse_pos = GetScreenToWorld2D(raw_mouse_pos, data->cam);
        unsigned int tile_idx = get_tile_idx(mouse_pos.x, mouse_pos.y, &tilemap);
        if (tile_idx >= MAX_N_TILES) return;
        if (tile_idx == last_tile_idx) return;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            TileType_t new_type = EMPTY_TILE;
            new_type = (tilemap.tiles[tile_idx].tile_type == SOLID_TILE)? EMPTY_TILE : SOLID_TILE;
            if (new_type == SOLID_TILE) tilemap.tiles[tile_idx].water_level = 0;
            change_a_tile(&tilemap, tile_idx, new_type);
            last_tile_idx = tile_idx;
        }
        else if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))
        {
            if (sc_map_size_64v(&tilemap.tiles[tile_idx].entities_set) == 0)
            {
                Entity_t* p_ent = create_water_runner(&scene->ent_manager, DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT, tile_idx);
                if (p_ent == NULL) return;

                CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
                p_ct->position.x = (tile_idx % tilemap.width) * tilemap.tile_size; 
                p_ct->position.y = (tile_idx / tilemap.width) * tilemap.tile_size; 
            }
            else
            {
                Entity_t* ent;
                unsigned int m_id;
                sc_map_foreach(&tilemap.tiles[tile_idx].entities_set, m_id, ent)
                {
                    if (ent->m_tag == PLAYER_ENT_TAG) continue;
                    CTileCoord_t* p_tilecoord = get_component(
                        ent, CTILECOORD_COMP_T
                    );

                    for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
                    {
                        // Use previously store tile position
                        // Clear from those positions
                        unsigned int tile_idx = p_tilecoord->tiles[i];
                        sc_map_del_64v(&(tilemap.tiles[tile_idx].entities_set), m_id);
                    }
                    CWaterRunner_t* p_crunner = get_component(ent, CWATERRUNNER_T);
                    if (p_crunner == NULL)
                    {
                        remove_entity(&scene->ent_manager, m_id);
                    }
                    else
                    {
                        free_water_runner(ent, &scene->ent_manager);
                    }
                }
            }
        }
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

static void player_simple_movement_system(Scene_t* scene)
{
    // Deal with player acceleration/velocity via inputs first
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);
    }

}

int main(void)
{
    sc_queue_init(&key_buffer);
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_memory_pools();

    init_assets(&engine.assets);
    load_from_infofile("res/assets.info", &engine.assets);
    init_player_creation("res/player_spr.info", &engine.assets);

    LevelScene_t scene;
    scene.scene.engine = &engine;
    init_scene(&scene.scene, &level_scene_render_func, &level_do_action);
    init_level_scene_data(&scene.data);

    scene.data.tilemap.width = DEFAULT_MAP_WIDTH;
    scene.data.tilemap.height = DEFAULT_MAP_HEIGHT;
    scene.data.tilemap.tile_size = TILE_SIZE;
    scene.data.tilemap.n_tiles = scene.data.tilemap.width * scene.data.tilemap.height;
    assert(scene.data.tilemap.n_tiles <= MAX_N_TILES);
    scene.data.tilemap.tiles = all_tiles;
    memset(scene.data.tile_sprites, 0, sizeof(scene.data.tile_sprites));
    for (size_t i = 0; i < scene.data.tilemap.n_tiles;i++)
    {
        all_tiles[i].solid = NOT_SOLID;
        all_tiles[i].tile_type = EMPTY_TILE;
        all_tiles[i].moveable = true;
        all_tiles[i].max_water_level = 1;
        sc_map_init_64v(&all_tiles[i].entities_set, 16, 0);
        all_tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
    }
    for (size_t i = 0; i < scene.data.tilemap.width; ++i)
    {
        unsigned int tile_idx = (scene.data.tilemap.height - 1) * scene.data.tilemap.width + i;
        change_a_tile(&scene.data.tilemap, tile_idx, SOLID_TILE);
    }
    create_player(&scene.scene.ent_manager, &scene.scene.engine->assets);
    update_entity_manager(&scene.scene.ent_manager);

    scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    scenes[0] = &scene.scene;
    change_scene(&engine, 0);

    sc_array_add(&scene.scene.systems, &player_simple_movement_system);
    //sc_array_add(&scene.scene.systems, &player_bbox_update_system);
    sc_array_add(&scene.scene.systems, &simple_friction_system);
    sc_array_add(&scene.scene.systems, &movement_update_system);
    sc_array_add(&scene.scene.systems, &update_tilemap_system);
    sc_array_add(&scene.scene.systems, &update_water_runner_system);
    sc_array_add(&scene.scene.systems, &toggle_block_system);
    sc_array_add(&scene.scene.systems, &camera_update_system);
    sc_array_add(&scene.scene.systems, &player_dir_reset_system);


    sc_map_put_64(&scene.scene.action_map, KEY_R, ACTION_RESTART);
    sc_map_put_64(&scene.scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene.scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene.scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene.scene.action_map, KEY_RIGHT, ACTION_RIGHT);


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

    unsigned int m_id;
    Entity_t* ent;
    sc_map_foreach(&scene.scene.ent_manager.entities_map[DYNMEM_ENT_TAG], m_id, ent)
    {
        free_water_runner(ent, &scene.scene.ent_manager);
    }
    free_scene(&scene.scene);
    for (size_t i = 0; i < scene.data.tilemap.n_tiles;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_term_64v(&all_tiles[i].entities_set);
    }
    term_level_scene_data(&scene.data);
    sc_queue_term(&key_buffer);
    term_assets(&engine.assets);
    CloseWindow();
}
