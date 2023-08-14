#include "scene_impl.h"
#include "game_systems.h"
#include "water_flow.h"
#include "constants.h"
#include "ent_impl.h"
#include "mempool.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

static Tile_t all_tiles[MAX_N_TILES] = {0};

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

            if (data->tile_sprites[tilemap.tiles[i].tile_type] != NULL)
            {
                draw_sprite(data->tile_sprites[tilemap.tiles[i].tile_type], (Vector2){x,y}, false);
            }
            else if (tilemap.tiles[i].tile_type == SOLID_TILE)
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
            }
            else if (tilemap.tiles[i].tile_type == ONEWAY_TILE)
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, MAROON);
            }
            else if (tilemap.tiles[i].tile_type == LADDER)
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, ORANGE);
            }
            else if (tilemap.tiles[i].tile_type == SPIKES)
            {
                DrawRectangle(
                    x + tilemap.tiles[i].offset.x, y + tilemap.tiles[i].offset.y,
                    tilemap.tiles[i].size.x, tilemap.tiles[i].size.y, RED
                );
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
                    && tilemap.tiles[i].water_level == 0
                    )
                {
                    if (i % tilemap.width != 0 && tilemap.tiles[left].wet && (tilemap.tiles[bot].solid == SOLID || tilemap.tiles[bot-1].solid == SOLID))
                    {
                        DrawLineEx((Vector2){x, bot_line}, (Vector2){x + TILE_SIZE / 2, bot_line}, SURFACE_THICKNESS, ColorAlpha(BLUE, 0.7));
                    }
                    if (right % tilemap.width != 0 && tilemap.tiles[right].wet && (tilemap.tiles[bot].solid == SOLID || tilemap.tiles[bot+1].solid == SOLID))
                    {
                        DrawLineEx((Vector2){x + TILE_SIZE / 2, bot_line}, (Vector2){x + TILE_SIZE, bot_line}, SURFACE_THICKNESS, ColorAlpha(BLUE, 0.7));
                    }
                }
            }

            // Draw water tile
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
            if (tilemap.tiles[i].max_water_level < MAX_WATER_LEVEL)
            {
                DrawRectangleLinesEx((Rectangle){x, y, TILE_SIZE, TILE_SIZE}, 2.0, ColorAlpha(BLUE, 0.5));
            }
        }

        char buffer[64] = {0};
        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
            CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
            Color colour;
            switch(p_ent->m_tag)
            {
                case PLAYER_ENT_TAG:
                    colour = RED;
                break;
                case CRATES_ENT_TAG:
                    colour = p_bbox->fragile? BROWN : GRAY;
                break;
                case BOULDER_ENT_TAG:
                    colour = GRAY;
                break;
                default:
                    colour = BLACK;
                break;
            }

            if (p_bbox != NULL)
            {
                if (p_ent->m_tag == BOULDER_ENT_TAG)
                {
                    DrawCircleV(Vector2Add(p_ct->position, p_bbox->half_size), p_bbox->half_size.x, colour);
                }
                else
                {
                    DrawRectangle(p_ct->position.x, p_ct->position.y, p_bbox->size.x, p_bbox->size.y, colour);
                }

                if (p_ent->m_tag == CRATES_ENT_TAG)
                {
                    CContainer_t* p_container = get_component(p_ent, CCONTAINER_T);
                    if (p_container != NULL)
                    {
                        switch (p_container->item)
                        {
                            case CONTAINER_LEFT_ARROW:
                                DrawLine(
                                    p_ct->position.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    BLACK
                                );
                            break;
                            case CONTAINER_RIGHT_ARROW:
                                DrawLine(
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    p_ct->position.x + p_bbox->size.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    BLACK
                                );
                            break;
                            case CONTAINER_UP_ARROW:
                                DrawLine(
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y,
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    BLACK
                                );
                            break;
                            case CONTAINER_DOWN_ARROW:
                                DrawLine(
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y + p_bbox->half_size.y,
                                    p_ct->position.x + p_bbox->half_size.x,
                                    p_ct->position.y + p_bbox->size.y,
                                    BLACK
                                );
                            break;
                            case CONTAINER_BOMB:
                                DrawCircleV(Vector2Add(p_ct->position, p_bbox->half_size), p_bbox->half_size.x, BLACK);
                            break;
                            default:
                            break;
                        }
                    }
                }
            }

            CHurtbox_t* p_hurtbox = get_component(p_ent, CHURTBOX_T);
            CHitBoxes_t* p_hitbox = get_component(p_ent, CHITBOXES_T);
            if (p_hitbox != NULL)
            {
                for (uint8_t i = 0;i < p_hitbox->n_boxes; ++i)
                {
                    Rectangle rec = {
                        .x = p_ct->position.x + p_hitbox->boxes[i].x,
                        .y = p_ct->position.y + p_hitbox->boxes[i].y,
                        .width = p_hitbox->boxes[i].width,
                        .height = p_hitbox->boxes[i].height,
                    };
                    DrawRectangleLinesEx(rec, 1.5, ORANGE);
                }
            }
            if (p_hurtbox != NULL)
            {
                Rectangle rec = {
                    .x = p_ct->position.x + p_hurtbox->offset.x,
                    .y = p_ct->position.y + p_hurtbox->offset.y,
                    .width = p_hurtbox->size.x,
                    .height = p_hurtbox->size.y,
                };
                DrawRectangleLinesEx(rec, 1.5, PURPLE);
            }
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
            DrawCircle(x+16, y+16, 8, ColorAlpha(BLUE, 0.6));
        }
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

        // For DEBUG
        const int gui_x = data->game_rec.x + data->game_rec.width + 10;
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
            CJump_t* p_cjump = get_component(p_ent, CJUMP_COMP_T);
            CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
            CMovementState_t* p_mstate = get_component(p_ent, CMOVEMENTSTATE_T);
            sprintf(buffer, "Pos: %.3f\n %.3f", p_ct->position.x, p_ct->position.y);
            DrawText(buffer, gui_x, 15, 12, BLACK);
            sprintf(buffer, "Vel: %.3f\n %.3f", p_ct->velocity.x, p_ct->velocity.y);
            DrawText(buffer, gui_x + 80, 15, 12, BLACK);
            //sprintf(buffer, "Accel: %.3f\n %.3f", p_ct->accel.x, p_ct->accel.y);
            //DrawText(buffer, tilemap.width * TILE_SIZE + 128, 60, 12, BLACK);
            sprintf(buffer, "Jumps: %u", p_cjump->jumps);
            DrawText(buffer, gui_x, 60, 12, BLACK);
            sprintf(buffer, "Crouch: %u", p_pstate->is_crouch);
            DrawText(buffer, gui_x, 90, 12, BLACK);
            sprintf(buffer, "Water: %s", p_mstate->water_state & 1? "YES":"NO");
            DrawText(buffer, gui_x, 120, 12, BLACK);
            sprintf(buffer, "Ladder: %u", p_pstate->ladder_state);
            DrawText(buffer, gui_x, 150, 12, BLACK);
        }
        //sprintf(buffer, "Spawn Entity: %s", get_spawn_selection_string(current_spawn_selection));
        //DrawText(buffer, gui_x, 240, 12, BLACK);
        sprintf(buffer, "Number of Entities: %u", sc_map_size_64v(&scene->ent_manager.entities));
        DrawText(buffer, gui_x, 270, 12, BLACK);
        sprintf(buffer, "FPS: %u", GetFPS());
        DrawText(buffer, gui_x, 320, 12, BLACK);

        static char mempool_stats[512];
        print_mempool_stats(mempool_stats);
        DrawText(mempool_stats, gui_x, 350, 12, BLACK);
    EndDrawing();
}

void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
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
            case ACTION_JUMP:
                p_playerstate->jump_pressed = pressed;
            break;
            default:
            break;
        }
        
    }

    if (!pressed)
    {
        switch (action)
        {
            case ACTION_RESTART:
                reload_level_tilemap((LevelScene_t*)scene);
            break;
            case ACTION_NEXTLEVEL:
                load_next_level_tilemap((LevelScene_t*)scene);
            break;
            case ACTION_PREVLEVEL:
                load_prev_level_tilemap((LevelScene_t*)scene);
            break;
            default:
            break;
        }
    }
}

void init_game_scene(LevelScene_t* scene)
{
    //init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);

    scene->data.tilemap.tiles = all_tiles;
    init_level_scene_data(&scene->data, MAX_N_TILES, all_tiles);

    create_player(&scene->scene.ent_manager, &scene->scene.engine->assets);
    update_entity_manager(&scene->scene.ent_manager);

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &player_bbox_update_system);
    sc_array_add(&scene->scene.systems, &player_pushing_system);
    sc_array_add(&scene->scene.systems, &friction_coefficient_update_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);

    sc_array_add(&scene->scene.systems, &moveable_update_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &boulder_destroy_wooden_tile_system);
    sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &tile_collision_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    sc_array_add(&scene->scene.systems, &player_crushing_system);
    sc_array_add(&scene->scene.systems, &spike_collision_system);
    sc_array_add(&scene->scene.systems, &state_transition_update_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
    sc_array_add(&scene->scene.systems, &lifetimer_update_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
    sc_array_add(&scene->scene.systems, &camera_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &player_respawn_system);
    sc_array_add(&scene->scene.systems, &update_water_runner_system);

    // This avoid graphical glitch, not essential
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, KEY_SPACE, ACTION_JUMP);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);
    sc_map_put_64(&scene->scene.action_map, KEY_R, ACTION_RESTART);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT_BRACKET, ACTION_NEXTLEVEL);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT_BRACKET, ACTION_PREVLEVEL);

}

void free_game_scene(LevelScene_t* scene)
{
    free_scene(&scene->scene);
    term_level_scene_data(&scene->data);
}
