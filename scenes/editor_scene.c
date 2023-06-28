#include "scene_impl.h"
#include "game_systems.h"
#include "constants.h"
#include "ent_impl.h"
#include "mempool.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

enum EntitySpawnSelection {
    TOGGLE_TILE = 0,
    TOGGLE_ONEWAY,
    TOGGLE_LADDER,
    TOGGLE_SPIKE,
    TOGGLE_WATER,
    SPAWN_CRATE,
    SPAWN_CRATE_ARROW_L,
    SPAWN_CRATE_ARROW_R,
    SPAWN_CRATE_ARROW_U,
    SPAWN_CRATE_ARROW_D,
    SPAWN_BOULDER,
};

#define MAX_SPAWN_TYPE 11
static unsigned int current_spawn_selection = 0;
static bool metal_toggle = false;

#define SELECTION_TILE_SIZE 32
#define SELECTION_TILE_HALFSIZE (SELECTION_TILE_SIZE >> 1)
#define SELECTION_GAP 5
#define SELECTION_REGION_WIDTH (SELECTION_TILE_SIZE * MAX_SPAWN_TYPE)
#define SELECTION_REGION_HEIGHT SELECTION_TILE_SIZE


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

static char* get_spawn_selection_string(enum EntitySpawnSelection sel)
{
    switch(sel)
    {
        case TOGGLE_TILE: return "solid tile";
        case TOGGLE_ONEWAY: return "wooden tile";
        case TOGGLE_LADDER: return "ladder";
        case TOGGLE_SPIKE: return "spike";
        case TOGGLE_WATER: return "water";
        case SPAWN_CRATE: return "crate";
        case SPAWN_BOULDER: return "boulder";
        default: return "unknown";
    }
}

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

            if (tilemap.tiles[i].water_level > 0)
            {
                // Draw water tile
                Color water_colour = ColorAlpha(BLUE, 0.5);
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, water_colour);
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

        Vector2 draw_pos = {data->game_rec.x, data->game_rec.y + data->game_rec.height + SELECTION_GAP};
        const Color crate_colour = metal_toggle ? GRAY : BROWN;
        const Color draw_colour[MAX_SPAWN_TYPE] = {
            BLACK, MAROON, ORANGE, ColorAlpha(RAYWHITE, 0.5), ColorAlpha(BLUE, 0.5),
            crate_colour, crate_colour, crate_colour, crate_colour, crate_colour,
            ColorAlpha(RAYWHITE, 0.5)
        };
        for (uint8_t i = 0; i < MAX_SPAWN_TYPE; ++i)
        {
            if (i != current_spawn_selection)
            {
                DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                Vector2 half_size = {SELECTION_TILE_HALFSIZE, SELECTION_TILE_HALFSIZE};
                switch (i)
                {
                    case TOGGLE_SPIKE:
                        DrawRectangle(draw_pos.x, draw_pos.y + SELECTION_TILE_HALFSIZE, SELECTION_TILE_SIZE, SELECTION_TILE_HALFSIZE, RED);
                    break;
                    case SPAWN_BOULDER:
                        DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, GRAY);
                    break;
                    case SPAWN_CRATE_ARROW_L:
                        DrawLine(
                            draw_pos.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    break;
                    case SPAWN_CRATE_ARROW_R:
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x * 2,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    break;
                    case SPAWN_CRATE_ARROW_U:
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    break;
                    case SPAWN_CRATE_ARROW_D:
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y * 2,
                            BLACK
                        );
                    break;
                }
            }
            draw_pos.x += SELECTION_TILE_SIZE;
        }

        draw_pos.x = data->game_rec.x + current_spawn_selection * SELECTION_TILE_SIZE;
        DrawRectangle(
            draw_pos.x - 2, draw_pos.y - 2,
            SELECTION_TILE_SIZE + 4, SELECTION_TILE_SIZE + 4, GREEN
        );
        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[current_spawn_selection]);
        const Vector2 half_size = {SELECTION_TILE_HALFSIZE, SELECTION_TILE_HALFSIZE};
        switch (current_spawn_selection)
        {
            case TOGGLE_SPIKE:
                DrawRectangle(draw_pos.x, draw_pos.y + SELECTION_TILE_HALFSIZE, SELECTION_TILE_SIZE, SELECTION_TILE_HALFSIZE, RED);
            break;
            case SPAWN_BOULDER:
                DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, GRAY);
            break;
            case SPAWN_CRATE_ARROW_L:
                DrawLine(
                    draw_pos.x,
                    draw_pos.y + half_size.y,
                    draw_pos.x + half_size.x,
                    draw_pos.y + half_size.y,
                    BLACK
                );
            break;
            case SPAWN_CRATE_ARROW_R:
                DrawLine(
                    draw_pos.x + half_size.x,
                    draw_pos.y + half_size.y,
                    draw_pos.x + half_size.x * 2,
                    draw_pos.y + half_size.y,
                    BLACK
                );
            break;
            case SPAWN_CRATE_ARROW_U:
                DrawLine(
                    draw_pos.x + half_size.x,
                    draw_pos.y,
                    draw_pos.x + half_size.x,
                    draw_pos.y + half_size.y,
                    BLACK
                );
            break;
            case SPAWN_CRATE_ARROW_D:
                DrawLine(
                    draw_pos.x + half_size.x,
                    draw_pos.y + half_size.y,
                    draw_pos.x + half_size.x,
                    draw_pos.y + half_size.y * 2,
                    BLACK
                );
            break;
        }

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
        sprintf(buffer, "Spawn Entity: %s", get_spawn_selection_string(current_spawn_selection));
        DrawText(buffer, gui_x, 240, 12, BLACK);
        sprintf(buffer, "Number of Entities: %u", sc_map_size_64v(&scene->ent_manager.entities));
        DrawText(buffer, gui_x, 270, 12, BLACK);
        sprintf(buffer, "FPS: %u", GetFPS());
        DrawText(buffer, gui_x, 320, 12, BLACK);

        static char mempool_stats[512];
        print_mempool_stats(mempool_stats);
        DrawText(mempool_stats, gui_x, 350, 12, BLACK);
    EndDrawing();
}

static void spawn_crate(Scene_t* scene, unsigned int tile_idx, bool metal, ContainerItem_t item)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_crate = create_crate(&scene->ent_manager, &scene->engine->assets, metal, item);

    CTransform_t* p_ctransform = get_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_ctransform->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;
}

static void spawn_boulder(Scene_t* scene, unsigned int tile_idx)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_boulder = create_boulder(&scene->ent_manager, &scene->engine->assets);

    CTransform_t* p_ctransform = get_component(p_boulder, CTRANSFORM_COMP_T);
    p_ctransform->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_ctransform->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;
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
            enum EntitySpawnSelection sel = (enum EntitySpawnSelection)current_spawn_selection;
            TileType_t new_type = EMPTY_TILE;
            switch (sel)
            {
            case TOGGLE_TILE:
                new_type = (tilemap.tiles[tile_idx].tile_type == SOLID_TILE)? EMPTY_TILE : SOLID_TILE;
                if (new_type == SOLID_TILE) tilemap.tiles[tile_idx].water_level = 0;
            break;
            case TOGGLE_ONEWAY:
                new_type = (tilemap.tiles[tile_idx].tile_type == ONEWAY_TILE)? EMPTY_TILE : ONEWAY_TILE;
            break;
            case TOGGLE_LADDER:
                new_type = (tilemap.tiles[tile_idx].tile_type == LADDER)? EMPTY_TILE : LADDER;
            break;
            case TOGGLE_SPIKE:
                new_type = (tilemap.tiles[tile_idx].tile_type == SPIKES)? EMPTY_TILE : SPIKES;
            break;
            case TOGGLE_WATER:
                new_type = tilemap.tiles[tile_idx].tile_type;
                if (tilemap.tiles[tile_idx].water_level == 0)
                {
                    tilemap.tiles[tile_idx].water_level = MAX_WATER_LEVEL;
                }
                else
                {
                    tilemap.tiles[tile_idx].water_level = 0;
                }
            break;
            case SPAWN_CRATE:
                spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_EMPTY);
            break;
            case SPAWN_BOULDER:
                spawn_boulder(scene, tile_idx);
            break;
            case SPAWN_CRATE_ARROW_L:
                spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_LEFT_ARROW);
            break;
            case SPAWN_CRATE_ARROW_R:
                spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_RIGHT_ARROW);
            break;
            case SPAWN_CRATE_ARROW_U:
                spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_UP_ARROW);
            break;
            case SPAWN_CRATE_ARROW_D:
                spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_DOWN_ARROW);
            break;
            }
            change_a_tile(&tilemap, tile_idx, new_type);
            last_tile_idx = tile_idx;
        }
        else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
        {
            change_a_tile(&tilemap, tile_idx, EMPTY_TILE);
            tilemap.tiles[tile_idx].water_level = 0;

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
                remove_entity(&scene->ent_manager, m_id);
            }
        }
        else
        {
            last_tile_idx = MAX_N_TILES;
        }
        return;
    }

    raw_mouse_pos = Vector2Subtract(raw_mouse_pos, (Vector2){0, data->game_rec.height});
    if (
        raw_mouse_pos.x < SELECTION_REGION_WIDTH
        && raw_mouse_pos.y < SELECTION_REGION_HEIGHT
    )
    {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            current_spawn_selection = ((int)raw_mouse_pos.x / SELECTION_TILE_SIZE);
        }
    }
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
            case ACTION_NEXT_SPAWN:
                if (!pressed)
                {
                    current_spawn_selection++;
                    current_spawn_selection %= MAX_SPAWN_TYPE;
                }
            break;
            case ACTION_PREV_SPAWN:
                if (!pressed)
                {
                    if (current_spawn_selection == 0)
                    {
                        current_spawn_selection = MAX_SPAWN_TYPE;
                    }
                    current_spawn_selection--;
                }
            break;
            case ACTION_METAL_TOGGLE:
                if (!pressed) metal_toggle = !metal_toggle;
            break;
            case ACTION_EXIT:
                if(scene->engine != NULL)
                {
                    change_scene(scene->engine, 0);
                }
            break;
            default:
            break;
        }
    }
}

void init_level_scene(LevelScene_t* scene)
{
    //init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);

    init_level_scene_data(&scene->data);
    // insert level scene systems
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
    sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    sc_array_add(&scene->scene.systems, &player_crushing_system);
    sc_array_add(&scene->scene.systems, &spike_collision_system);
    sc_array_add(&scene->scene.systems, &state_transition_update_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
    sc_array_add(&scene->scene.systems, &camera_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &player_respawn_system);
    sc_array_add(&scene->scene.systems, &toggle_block_system);

    // This avoid graphical glitch, not essential
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, KEY_SPACE, ACTION_JUMP);
    sc_map_put_64(&scene->scene.action_map, KEY_O, ACTION_PREV_SPAWN);
    sc_map_put_64(&scene->scene.action_map, KEY_P, ACTION_NEXT_SPAWN);
    sc_map_put_64(&scene->scene.action_map, KEY_M, ACTION_METAL_TOGGLE);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);

    scene->data.tilemap.width = DEFAULT_MAP_WIDTH;
    scene->data.tilemap.height = DEFAULT_MAP_HEIGHT;
    scene->data.tilemap.n_tiles = scene->data.tilemap.width * scene->data.tilemap.height;
    assert(scene->data.tilemap.n_tiles <= MAX_N_TILES);
    scene->data.tilemap.tiles = all_tiles;
    memset(scene->data.tile_sprites, 0, sizeof(scene->data.tile_sprites));
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        all_tiles[i].solid = NOT_SOLID;
        all_tiles[i].tile_type = EMPTY_TILE;
        all_tiles[i].moveable = true;
        sc_map_init_64v(&all_tiles[i].entities_set, 16, 0);
        all_tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
    }
    for (size_t i = 0; i < scene->data.tilemap.width; ++i)
    {
        unsigned int tile_idx = (scene->data.tilemap.height - 1) * scene->data.tilemap.width + i;
        change_a_tile(&scene->data.tilemap, tile_idx, SOLID_TILE);
    }

    create_player(&scene->scene.ent_manager, &scene->scene.engine->assets);
    update_entity_manager(&scene->scene.ent_manager);
}

void free_level_scene(LevelScene_t* scene)
{
    free_scene(&scene->scene);
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_term_64v(&all_tiles[i].entities_set);
    }
    term_level_scene_data(&scene->data);
}

void reload_level_scene(LevelScene_t* scene)
{
}
