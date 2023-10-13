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

Scene_t* scenes[1];
static GameEngine_t engine =
{
    .scenes = scenes,
    .max_scenes = 1,
    .curr_scene = 0,
    .assets = {0}
};

static bool water_toggle = false;

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_ent;

    BeginTextureMode(data->game_viewport);
        ClearBackground(WHITE);
        BeginMode2D(data->camera.cam);
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
        ClearBackground( water_toggle? ColorAlpha(BLUE, 0.2) : LIGHTGRAY);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );
    EndDrawing();
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

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        last_tile_idx = MAX_N_TILES;
    }
    if (
        raw_mouse_pos.x < data->game_rec.width
        && raw_mouse_pos.y < data->game_rec.height
    ) 
    {
        Vector2 mouse_pos = GetScreenToWorld2D(raw_mouse_pos, data->camera.cam);
        unsigned int tile_idx = get_tile_idx(mouse_pos.x, mouse_pos.y, &tilemap);
        if (tile_idx >= MAX_N_TILES) return;
        if (tile_idx == last_tile_idx) return;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            if (!water_toggle)
            {
                TileType_t new_type = EMPTY_TILE;
                new_type = (tilemap.tiles[tile_idx].tile_type == SOLID_TILE)? EMPTY_TILE : SOLID_TILE;
                if (new_type == SOLID_TILE)
                {
                    tilemap.tiles[tile_idx].water_level = 0;
                    tilemap.tiles[tile_idx].wet = false;
                }
                change_a_tile(&tilemap, tile_idx, new_type);
            }
            else
            {
                if (tilemap.tiles[tile_idx].tile_type != SOLID_TILE)
                {
                    if (tilemap.tiles[tile_idx].water_level > 0)
                    {
                        tilemap.tiles[tile_idx].water_level = 0;
                    }
                    else
                    {
                        tilemap.tiles[tile_idx].water_level = tilemap.tiles[tile_idx].max_water_level;
                    }
                }
            
            }
            last_tile_idx = tile_idx;
            CWaterRunner_t* p_crunner;
            sc_map_foreach_value(&scene->ent_manager.component_map[CWATERRUNNER_T], p_crunner)
            {
                //if (tilemap.tiles[tile_idx].solid)
                //{
                //    int curr_idx = p_crunner->current_tile;
                //    //Tile_t* curr_tile = tilemap.tiles + p_crunner->current_tile;
                //    bool blocked = false;
                //    puts("Retracing");
                //    while(curr_idx >= 0)
                //    {
                //        printf("%u\n", curr_idx);
                //        if (curr_idx == tile_idx)
                //        {
                //            blocked = true;
                //            break;
                //        }
                //        curr_idx = p_crunner->bfs_tilemap.tilemap[curr_idx].from;
                //    }
                //    if (blocked)
                //    {
                //        int return_idx = p_crunner->current_tile;
                //        if (p_crunner->bfs_tilemap.tilemap[curr_idx].from >= 0)
                //        {
                //            p_crunner->current_tile = p_crunner->bfs_tilemap.tilemap[curr_idx].from;
                //        }
                //        puts("Blocking...");
                //        while(curr_idx != return_idx)
                //        {
                //            tilemap.tiles[curr_idx].wet = false;
                //            curr_idx = p_crunner->bfs_tilemap.tilemap[curr_idx].to;
                //        }
                //    }
                //}
                p_crunner->state = BFS_RESET;

            }
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
            case ACTION_METAL_TOGGLE:
                 if (!pressed) water_toggle = !water_toggle;
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
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_engine(&engine);

    LevelScene_t scene;
    scene.scene.engine = &engine;
    init_scene(&scene.scene, &level_scene_render_func, &level_do_action);
    init_level_scene_data(
        &scene.data, MAX_N_TILES, all_tiles,
        (Rectangle){25, 25, VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE}
    );
    assert(scene.data.tilemap.n_tiles <= MAX_N_TILES);

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
    sc_map_put_64(&scene.scene.action_map, KEY_P, ACTION_METAL_TOGGLE);


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
    CloseWindow();
}
