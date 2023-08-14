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
            
            if (tilemap.tiles[i].wet)
            {
#define SURFACE_THICKNESS 4
                int up = i - tilemap.width;
                int bot = i + tilemap.width;
                int right = i + 1;
                int left = i - 1;
                int bot_line = y + TILE_SIZE - tilemap.tiles[i].water_level * WATER_BBOX_STEP - SURFACE_THICKNESS / 2;
                if (up >= 0 && tilemap.tiles[up].wet)
                {
                    DrawLineEx((Vector2){x + TILE_SIZE / 2, y}, (Vector2){x + TILE_SIZE / 2, y + TILE_SIZE - tilemap.tiles[i].water_level * WATER_BBOX_STEP}, SURFACE_THICKNESS, ColorAlpha(BLUE, 0.7));
                }


                if (
                    bot <= tilemap.n_tiles
                    && tilemap.tiles[bot].water_level < MAX_WATER_LEVEL
                    && tilemap.tiles[i].water_level == 0
                    )
                {
                    if (i % tilemap.width != 0 && tilemap.tiles[left].wet)
                    {
                        DrawLineEx((Vector2){x, bot_line}, (Vector2){x + TILE_SIZE / 2, bot_line}, SURFACE_THICKNESS, ColorAlpha(BLUE, 0.7));
                    }
                    if (right % tilemap.width != 0 && tilemap.tiles[right].wet)
                    {
                        DrawLineEx((Vector2){x + TILE_SIZE / 2, bot_line}, (Vector2){x + TILE_SIZE, bot_line}, SURFACE_THICKNESS, ColorAlpha(BLUE, 0.7));
                    }
                }
            }

            uint32_t water_height = tilemap.tiles[i].water_level * WATER_BBOX_STEP;
            // Draw water tile
            Color water_colour = ColorAlpha(BLUE, 0.5);
            DrawRectangle(
                x,
                y + (TILE_SIZE - water_height),
                TILE_SIZE,
                water_height,
                water_colour
            );
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

            if (p_runner->state == LOWEST_POINT_MOVEMENT)
            {
                int curr_idx = p_runner->current_tile;
                int next_idx = p_runner->bfs_tilemap.tilemap[curr_idx].to;
                while(curr_idx != p_runner->target_tile || curr_idx == next_idx)
                while(next_idx >= 0)
                {
                    unsigned int x1 = (curr_idx % tilemap.width) * tilemap.tile_size + tilemap.tile_size / 2; 
                    unsigned int y1 = (curr_idx / tilemap.width) * tilemap.tile_size + tilemap.tile_size / 2; 
                    unsigned int x2 = (next_idx % tilemap.width) * tilemap.tile_size + tilemap.tile_size / 2; 
                    unsigned int y2 = (next_idx / tilemap.width) * tilemap.tile_size + tilemap.tile_size / 2; 
                    DrawLine(x1, y1, x2, y2, BLACK);
                    curr_idx = next_idx;
                    next_idx = p_runner->bfs_tilemap.tilemap[curr_idx].to;
                }
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
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );
    EndDrawing();
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
    init_level_scene_data(&scene.data, MAX_N_TILES, all_tiles);

    LevelPack_t* pack = get_level_pack(&engine.assets, "TestLevels");
    assert(pack != NULL);

    scene.data.level_pack = pack;
    scene.data.current_level = 0;
    assert(load_level_tilemap(&scene, 0) == true);

    update_entity_manager(&scene.scene.ent_manager);

    scene.data.tile_sprites[ONEWAY_TILE] = get_sprite(&engine.assets, "tl_owp");
    scene.data.tile_sprites[LADDER] = get_sprite(&engine.assets, "tl_ldr");
    scenes[0] = &scene.scene;
    change_scene(&engine, 0);

    sc_array_add(&scene.scene.systems, &player_simple_movement_system);
    sc_array_add(&scene.scene.systems, &simple_friction_system);
    sc_array_add(&scene.scene.systems, &movement_update_system);
    sc_array_add(&scene.scene.systems, &update_tilemap_system);
    sc_array_add(&scene.scene.systems, &update_water_runner_system);
    sc_array_add(&scene.scene.systems, &camera_update_system);
    sc_array_add(&scene.scene.systems, &player_dir_reset_system);


    sc_map_put_64(&scene.scene.action_map, KEY_R, ACTION_RESTART);
    sc_map_put_64(&scene.scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene.scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene.scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene.scene.action_map, KEY_RIGHT, ACTION_RIGHT);
    sc_map_put_64(&scene.scene.action_map, KEY_P, ACTION_METAL_TOGGLE);

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
    term_level_scene_data(&scene.data);
    sc_queue_term(&key_buffer);
    term_assets(&engine.assets);
    CloseWindow();
}
