#include "scene_impl.h"
#include "game_systems.h"
#include "water_flow.h"
#include "constants.h"
#include "ent_impl.h"
#include "mempool.h"
#include "raylib.h"
#include "raymath.h"
#include "AABB.h"
#include <stdio.h>

static Tile_t all_tiles[MAX_N_TILES] = {0};

enum EntitySpawnSelection {
    TOGGLE_TILE = 0,
    TOGGLE_ONEWAY,
    TOGGLE_LADDER,
    TOGGLE_SPIKE,
    TOGGLE_WATER,
    TOGGLE_AIR_POCKET,
    SPAWN_CHEST,
    SPAWN_CRATE,
    SPAWN_CRATE_ARROW_L,
    SPAWN_CRATE_ARROW_R,
    SPAWN_CRATE_ARROW_U,
    SPAWN_CRATE_ARROW_D,
    SPAWN_CRATE_BOMB,
    SPAWN_BOULDER,
    SPAWN_WATER_RUNNER,
    SPAWN_LEVEL_END,
};

#define MAX_SPAWN_TYPE 16
static unsigned int current_spawn_selection = 0;
static bool metal_toggle = false;
static bool crate_activation = false;

#define SELECTION_TILE_SIZE 32
#define SELECTION_TILE_HALFSIZE (SELECTION_TILE_SIZE >> 1)
#define SELECTION_GAP 5
#define SELECTION_REGION_WIDTH (SELECTION_TILE_SIZE * MAX_SPAWN_TYPE)
#define SELECTION_REGION_HEIGHT SELECTION_TILE_SIZE

#define GAME_LAYER 0
#define SELECTION_LAYER 1
#define CONTROL_LAYER 2

static const uint8_t CONNECTIVITY_TILE_MAPPING[16] = {
    0,3,15,14,
    1,2,12,13,
    7,6,11,10,
    4,5,8 ,9 ,
};

#define N_SOLID_TILESETS 3
static const char* SOLID_TILE_SELECTIONS[N_SOLID_TILESETS] = {
    "stile0", "stile1", "stile2"
};

static char* get_spawn_selection_string(enum EntitySpawnSelection sel)
{
    switch(sel)
    {
        case TOGGLE_TILE: return "solid tile";
        case TOGGLE_ONEWAY: return "wooden tile";
        case TOGGLE_LADDER: return "ladder";
        case TOGGLE_SPIKE: return "spike";
        case TOGGLE_WATER: return "water";
        case TOGGLE_AIR_POCKET: return "air pocket";
        case SPAWN_CHEST: return "chest";
        case SPAWN_CRATE: return (metal_toggle) ? "metal crate" : "wooden crate";
        case SPAWN_CRATE_ARROW_D: return (metal_toggle) ? "metal down arrow crate" : "wooden down arrow crate";
        case SPAWN_CRATE_ARROW_U: return (metal_toggle) ? "metal up arrow crate" : "wooden up arrow crate";
        case SPAWN_CRATE_ARROW_L: return (metal_toggle) ? "metal left arrow crate" : "wooden left arrow crate";
        case SPAWN_CRATE_ARROW_R: return (metal_toggle) ? "metal right arrow crate" : "wooden right arrow crate";
        case SPAWN_CRATE_BOMB: return (metal_toggle) ? "metal bomb crate" : "wooden bomb crate";
        case SPAWN_BOULDER: return "boulder";
        case SPAWN_WATER_RUNNER: return "water runner";
        case SPAWN_LEVEL_END: return "level end";
        default: return "unknown";
    }
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

// This means you might be able to have two editor scene without running into problems
#define SELECTION_RENDER_HEIGHT  (SELECTION_REGION_HEIGHT * 3)
static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    Entity_t* p_ent;
    static char buffer[512];

    Rectangle selection_rec = scene->layers.render_layers[SELECTION_LAYER].render_area;
    Rectangle game_rec = scene->layers.render_layers[GAME_LAYER].render_area;
    Vector2 draw_pos = {game_rec.x, game_rec.y + game_rec.height + SELECTION_GAP};
    BeginTextureMode(scene->layers.render_layers[CONTROL_LAYER].layer_tex);
        ClearBackground(BLANK);
        DrawRectangleLines(
            selection_rec.x + current_spawn_selection * SELECTION_TILE_SIZE, selection_rec.y,
            SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, GREEN
        );
        DrawText("R to reset the map, Q/E to cycle the selection,\nF to toggle metal crates. T to toggle crate spawn behaviour\nB to toggle grid, V to set spawn point", selection_rec.x, selection_rec.y + selection_rec.height + 2, 14, BLACK);

        draw_pos.x = game_rec.x + (MAX_SPAWN_TYPE + 1) * SELECTION_TILE_SIZE;
        sprintf(buffer, "Selection: %s", get_spawn_selection_string(current_spawn_selection));
        DrawText(buffer, draw_pos.x, draw_pos.y, 20, BLACK);
        draw_pos.y += SELECTION_TILE_SIZE + 5;
        sprintf(buffer, "Crate %s on spawn", crate_activation? "active" : "inactive");
        DrawText(buffer, draw_pos.x, draw_pos.y, 20, BLACK);
        draw_pos.y += SELECTION_TILE_SIZE + 5;
        sprintf(buffer, "Time scale: %.2f", scene->time_scale);
        DrawText(buffer, draw_pos.x, draw_pos.y, 20, BLACK);

        // For DEBUG
        const int gui_x = game_rec.x + game_rec.width + 10;
        int gui_y = 15;

        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
    #if !defined(PLATFORM_WEB)
            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
            CJump_t* p_cjump = get_component(p_ent, CJUMP_COMP_T);
            CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
            CMovementState_t* p_mstate = get_component(p_ent, CMOVEMENTSTATE_T);

            sprintf(buffer, "Pos: %.3f\n %.3f", p_ent->position.x, p_ent->position.y);
            DrawText(buffer, gui_x, gui_y, 12, BLACK);
            sprintf(buffer, "Vel: %.3f\n %.3f", p_ct->velocity.x, p_ct->velocity.y);
            DrawText(buffer, gui_x + 80, gui_y, 12, BLACK);

            gui_y += 45;
            sprintf(buffer, "Jumps: %u, %u, %u", p_cjump->jumps, p_cjump->jump_released, p_cjump->coyote_timer);
            DrawText(buffer, gui_x, gui_y, 12, BLACK);
            gui_y += 30;
            sprintf(buffer, "Crouch: %u", p_pstate->is_crouch);
            DrawText(buffer, gui_x, gui_y, 12, BLACK);
            gui_y += 30;
            sprintf(buffer, "Water: %s", p_mstate->water_state & 1? "YES":"NO");
            DrawText(buffer, gui_x, gui_y, 12, BLACK);
            gui_y += 30;
            sprintf(buffer, "Ladder: %u", p_pstate->ladder_state);
            DrawText(buffer, gui_x, gui_y, 12, BLACK);
            gui_y += 30;

#endif
            CAirTimer_t* p_air = get_component(p_ent, CAIRTIMER_T);

            Sprite_t* spr = get_sprite(&scene->engine->assets, "p_bigbubble");
            Vector2 air_pos = {game_rec.x + game_rec.width - 32, game_rec.y + game_rec.height - 32};
            for (uint8_t i = 0; i < p_air->curr_count; i++)
            {
                draw_sprite(spr, 0, air_pos, 0, false);
                //DrawCircleV(air_pos, 16, BLUE);
                air_pos.x -= 32;
            }
        }
        //sprintf(buffer, "Spawn Entity: %s", get_spawn_selection_string(current_spawn_selection));
        //DrawText(buffer, gui_x, 240, 12, BLACK);
        sprintf(buffer, "Number of Entities: %u", sc_map_size_64v(&scene->ent_manager.entities));
        DrawText(buffer, gui_x, gui_y, 12, BLACK);
        gui_y += 30;
        sprintf(buffer, "FPS: %u", GetFPS());
        DrawText(buffer, gui_x, gui_y, 12, BLACK);

        gui_y += 30;
        sprintf(buffer, "part sys free: %u", get_number_of_free_emitter(&scene->part_sys));
        DrawText(buffer, gui_x, gui_y, 12, BLACK);

        gui_y += 30;
        print_mempool_stats(buffer);
        DrawText(buffer, gui_x, gui_y, 12, BLACK);

        gui_y += 330;
        sprintf(buffer, "Chests: %u / %u", data->coins.current, data->coins.total);
        DrawText(buffer, gui_x, gui_y, 24, BLACK);
    EndTextureMode();
}

static void render_editor_game_scene(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    Rectangle game_rec = scene->layers.render_layers[GAME_LAYER].render_area;

    Entity_t* p_ent;
    Vector2 min = GetScreenToWorld2D((Vector2){0, 0}, data->camera.cam);
    Vector2 max = GetScreenToWorld2D(
        (Vector2){game_rec.width, game_rec.height},
        data->camera.cam
    );
    min = Vector2Scale(min, 1.0f/tilemap.tile_size);
    max = Vector2Scale(max, 1.0f/tilemap.tile_size);
    min.x = (int)fmax(0, min.x - 1);
    min.y = (int)fmax(0, min.y - 1);
    max.x = (int)fmin(tilemap.width, max.x + 1);
    max.y = (int)fmin(tilemap.height, max.y + 1);

    Texture2D* bg = get_texture(&scene->engine->assets, "bg_tex");
    BeginTextureMode(scene->layers.render_layers[GAME_LAYER].layer_tex);
        ClearBackground(WHITE);
        BeginMode2D(data->camera.cam);
        float rect_x = data->camera.target_pos.x;
        DrawTexturePro(*bg,
            //(Rectangle){0,0,64,64},
            (Rectangle){0,0,(tilemap.width+1)*tilemap.tile_size*2, (tilemap.height+1)*tilemap.tile_size*2},
            (Rectangle){-rect_x / 4,0,(tilemap.width+1)*tilemap.tile_size*2, (tilemap.height+1)*tilemap.tile_size*2},
            //(Rectangle){0,0,game_rec.width, game_rec.height},
            (Vector2){0,0}, 0.0f, WHITE
        );
        

        char buffer[64] = {0};
        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);

            // Draw the spawn point
            if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
                DrawCircleV(p_ent->spawn_pos, 6, PURPLE);
            }
            
            // Entity culling
            Vector2 box_size = {0};
            if (p_bbox != NULL) box_size = p_bbox->size;
            if (
                p_ent->position.x + box_size.x < min.x * tilemap.tile_size
                || p_ent->position.x > max.x * tilemap.tile_size
                || p_ent->position.y + box_size.y < min.y * tilemap.tile_size
                || p_ent->position.y > max.y * tilemap.tile_size
            ) continue;

            if (data->show_grid)
            {
                Color colour;
                switch(p_ent->m_tag)
                {
                    case PLAYER_ENT_TAG:
                        colour = RED;
                    break;
                    case CRATES_ENT_TAG:
                    {
                        CContainer_t* p_container = get_component(p_ent, CCONTAINER_T);
                        colour = p_container->material == WOODEN_CONTAINER ? BROWN : GRAY;
                    }
                    break;
                    case BOULDER_ENT_TAG:
                        colour = GRAY;
                    break;
                    case CHEST_ENT_TAG:
                        colour = YELLOW;
                    break;
                    default:
                        colour = BLACK;
                    break;
                }

                if (p_bbox != NULL)
                {
                    if (p_ent->m_tag == BOULDER_ENT_TAG)
                    {
                        DrawCircleV(Vector2Add(p_ent->position, p_bbox->half_size), p_bbox->half_size.x, colour);
                    }
                    else
                    {
                        DrawRectangle(p_ent->position.x, p_ent->position.y, p_bbox->size.x, p_bbox->size.y, colour);
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
                                        p_ent->position.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        BLACK
                                    );
                                break;
                                case CONTAINER_RIGHT_ARROW:
                                    DrawLine(
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        p_ent->position.x + p_bbox->size.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        BLACK
                                    );
                                break;
                                case CONTAINER_UP_ARROW:
                                    DrawLine(
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y,
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        BLACK
                                    );
                                break;
                                case CONTAINER_DOWN_ARROW:
                                    DrawLine(
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y + p_bbox->half_size.y,
                                        p_ent->position.x + p_bbox->half_size.x,
                                        p_ent->position.y + p_bbox->size.y,
                                        BLACK
                                    );
                                break;
                                case CONTAINER_BOMB:
                                    DrawCircleV(Vector2Add(p_ent->position, p_bbox->half_size), p_bbox->half_size.x, BLACK);
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
                            .x = p_ent->position.x + p_hitbox->boxes[i].x,
                            .y = p_ent->position.y + p_hitbox->boxes[i].y,
                            .width = p_hitbox->boxes[i].width,
                            .height = p_hitbox->boxes[i].height,
                        };
                        DrawRectangleLinesEx(rec, 1.5, ORANGE);
                    }
                }
                if (p_hurtbox != NULL)
                {
                    Rectangle rec = {
                        .x = p_ent->position.x + p_hurtbox->offset.x,
                        .y = p_ent->position.y + p_hurtbox->offset.y,
                        .width = p_hurtbox->size.x,
                        .height = p_hurtbox->size.y,
                    };
                    DrawRectangleLinesEx(rec, 1.5, PURPLE);
                }
            }

            CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
            if (p_cspr != NULL)
            {
                const SpriteRenderInfo_t spr = p_cspr->sprites[p_cspr->current_idx];
                if (spr.sprite != NULL)
                {
                    Vector2 pos = p_ent->position;
                    if (p_bbox != NULL)
                    {
                        pos = Vector2Add(
                            pos,
                            get_anchor_offset(p_bbox->size, spr.dest_anchor, p_cspr->flip_x)
                        );
                        pos = Vector2Subtract(
                            pos,
                            get_anchor_offset(spr.sprite->frame_size, spr.src_anchor, p_cspr->flip_x)
                        );
                    }

                    Vector2 offset = spr.offset;
                    if (p_cspr->flip_x) offset.x *= -1;

                    pos = Vector2Add(pos, offset);
                    draw_sprite(spr.sprite, p_cspr->current_frame, pos, 0.0f, p_cspr->flip_x);
                }
            }
        }

        sc_map_foreach_value(&scene->ent_manager.entities_map[LEVEL_END_TAG], p_ent)
        {
            DrawCircleV(p_ent->position, tilemap.tile_size >> 1, (data->coins.current < data->coins.total)? RED : GREEN);
        }

        for (int tile_y = min.y; tile_y < max.y; tile_y++)
        {
            for (int tile_x = min.x; tile_x < max.x; tile_x++)
            {
                int i = tile_x + tile_y * tilemap.width;
                int x = tile_x * TILE_SIZE;
                int y = tile_y * TILE_SIZE;



                //if (!tilemap.tiles[i].moveable)
                //{
                //    // Draw water tile
                //    Color water_colour = ColorAlpha(RED, 0.2);
                //    DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, water_colour);
                //}

                uint8_t tile_sprite_idx = tilemap.tiles[i].tile_type + tilemap.tiles[i].rotation;
                if (data->tile_sprites[tile_sprite_idx] != NULL)
                {
                    draw_sprite(data->tile_sprites[tile_sprite_idx], 0, (Vector2){x,y}, 0.0f, false);
                }
                else if (tilemap.tiles[i].tile_type == SOLID_TILE)
                {
                    draw_sprite(
                        data->solid_tile_sprites, CONNECTIVITY_TILE_MAPPING[tilemap.tiles[i].connectivity],
                        (Vector2){x,y}, 0.0f, false
                    );
                    //sprintf(buffer, "%u", tilemap.tiles[i].connectivity);
                    //DrawText(buffer, x + tilemap.tile_size / 2, y + tilemap.tile_size / 2, 12, WHITE);
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
#define     SURFACE_THICKNESS 4
                    int up = i - tilemap.width;
                    unsigned int bot = i + tilemap.width;
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

                if (tilemap.tiles[i].max_water_level < MAX_WATER_LEVEL)
                {
                    DrawRectangleLinesEx((Rectangle){x, y, TILE_SIZE, TILE_SIZE}, 2.0, ColorAlpha(BLUE, 0.5));
                }
            }
        }

        draw_particle_system(&scene->part_sys);

        // Draw water tile
        for (int tile_y = min.y; tile_y < max.y; tile_y++)
        {
            for (int tile_x = min.x; tile_x < max.x; tile_x++)
            {
                int i = tile_x + tile_y * tilemap.width;
                int x = tile_x * TILE_SIZE;
                int y = tile_y * TILE_SIZE;

                uint32_t water_height = tilemap.tiles[i].water_level * WATER_BBOX_STEP;
                Color water_colour = ColorAlpha(BLUE, 0.5);
                DrawRectangle(
                    x,
                    y + (TILE_SIZE - water_height),
                    TILE_SIZE,
                    water_height,
                    water_colour
                );
            }
        }

        sc_map_foreach_value(&scene->ent_manager.entities_map[DYNMEM_ENT_TAG], p_ent)
        {
            CWaterRunner_t* p_runner = get_component(p_ent, CWATERRUNNER_T);

            unsigned int x = ((p_runner->current_tile) % tilemap.width) * tilemap.tile_size;
            unsigned int y = ((p_runner->current_tile) / tilemap.width) * tilemap.tile_size;
            DrawCircle(x+16, y+16, 8, ColorAlpha(BLUE, 0.6));
        }


        if (data->show_grid)
        {
            for (int tile_y = min.y; tile_y < max.y; tile_y++)
            {
                for (int tile_x = min.x; tile_x < max.x; tile_x++)
                {
                    int i = tile_x + tile_y * tilemap.width;
                    int x = tile_x * TILE_SIZE;
                    int y = tile_y * TILE_SIZE;

                    sprintf(buffer, "%u", sc_map_size_64v(&tilemap.tiles[i].entities_set));

                    //if (tilemap.tiles[i].solid > 0)
                    {
                        DrawText(buffer, x, y, 10, WHITE);
                    }
                    //else
                    //{
                    //    // Draw water tile
                    //    DrawText(buffer, x, y, 10, BLACK);
                    //}
                }
            }

            // Draw tile grid
            for (size_t i = min.x; i < max.x; ++i)
            {
                int x = (i+1)*TILE_SIZE;
                DrawLine(x, 0, x, tilemap.height * TILE_SIZE, WHITE);
            }
            for (size_t i = min.y; i < max.y;++i)
            {
                int y = (i+1)*TILE_SIZE;
                DrawLine(0, y, tilemap.width * TILE_SIZE, y, WHITE);
            }
        }
        EndMode2D();
    EndTextureMode();
}

static void spawn_chest(Scene_t* scene, unsigned int tile_idx)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_crate = create_chest(&scene->ent_manager);
    if (p_crate == NULL) return;

    p_crate->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_crate->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;

    CTransform_t* p_ctransform = get_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->active = true;
}

static void spawn_crate(Scene_t* scene, unsigned int tile_idx, bool metal, ContainerItem_t item, bool active)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_crate = create_crate(&scene->ent_manager, metal, item);
    if (p_crate == NULL) return;
    p_crate->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_crate->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;

    CTransform_t* p_ctransform = get_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->active = active;
}

static void spawn_boulder(Scene_t* scene, unsigned int tile_idx)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_boulder = create_boulder(&scene->ent_manager);
    if (p_boulder == NULL) return;

    p_boulder->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_boulder->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;
}

static void toggle_block_system(Scene_t* scene, ActionType_t action, bool pressed)
{
    static unsigned int last_tile_idx = MAX_N_TILES;
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;


    if (action == ACTION_SPAWN_TILE && !pressed)
    {
        last_tile_idx = MAX_N_TILES;
    }

    if (point_in_AABB(scene->mouse_pos, scene->layers.render_layers[GAME_LAYER].render_area))
    {
        Vector2 raw_mouse_pos = Vector2Subtract(scene->mouse_pos,
            (Vector2){
                scene->layers.render_layers[GAME_LAYER].render_area.x,
                scene->layers.render_layers[GAME_LAYER].render_area.y
            }
        );
        Vector2 mouse_pos = GetScreenToWorld2D(raw_mouse_pos, data->camera.cam);
        unsigned int tile_idx = get_tile_idx(mouse_pos.x, mouse_pos.y, &tilemap);
        if (tile_idx >= (tilemap.n_tiles - tilemap.width)) return;
        if (tile_idx == last_tile_idx) return;

        if (action == ACTION_SPAWN_TILE && pressed)
        {
            enum EntitySpawnSelection sel = (enum EntitySpawnSelection)current_spawn_selection;
            TileType_t new_type = EMPTY_TILE;
            switch (sel)
            {
                case TOGGLE_TILE:
                    new_type = (tilemap.tiles[tile_idx].tile_type == SOLID_TILE)? EMPTY_TILE : SOLID_TILE;
                    if (new_type == SOLID_TILE)
                    {
                        tilemap.tiles[tile_idx].water_level = 0;
                        tilemap.tiles[tile_idx].wet = false;
                    }
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
                    if (tilemap.tiles[tile_idx].water_level < MAX_WATER_LEVEL)
                    {
                        tilemap.tiles[tile_idx].water_level = MAX_WATER_LEVEL;
                        tilemap.tiles[tile_idx].wet = false;
                    }
                break;
                case TOGGLE_AIR_POCKET:
                    new_type = tilemap.tiles[tile_idx].tile_type;
                    if (tilemap.tiles[tile_idx].max_water_level < MAX_WATER_LEVEL)
                    {
                        tilemap.tiles[tile_idx].max_water_level = MAX_WATER_LEVEL;
                    }
                    else
                    {
                        tilemap.tiles[tile_idx].max_water_level = 0;
                        tilemap.tiles[tile_idx].water_level = 0;
                    }
                break;
                case SPAWN_CRATE:
                {
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_EMPTY, crate_activation);
                }
                break;
                case SPAWN_BOULDER:
                    spawn_boulder(scene, tile_idx);
                break;
                case SPAWN_CRATE_ARROW_L:
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_LEFT_ARROW, crate_activation);
                break;
                case SPAWN_CRATE_ARROW_R:
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_RIGHT_ARROW, crate_activation);
                break;
                case SPAWN_CRATE_ARROW_U:
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_UP_ARROW, crate_activation);
                break;
                case SPAWN_CRATE_ARROW_D:
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_DOWN_ARROW, crate_activation);
                break;
                case SPAWN_CRATE_BOMB:
                    spawn_crate(scene, tile_idx, metal_toggle, CONTAINER_BOMB, crate_activation);
                break;
                case SPAWN_WATER_RUNNER:
                {
                    Entity_t* p_ent = create_water_runner(&scene->ent_manager, DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT, tile_idx);
                    if (p_ent != NULL)
                    {
                        p_ent->position.x = (tile_idx % tilemap.width) * tilemap.tile_size;
                        p_ent->position.y = (tile_idx / tilemap.width) * tilemap.tile_size;
                    }
                }
                break;
                case SPAWN_LEVEL_END:
                {
                    Entity_t* p_ent = create_level_end(&scene->ent_manager);
                    if (p_ent != NULL)
                    {
                        p_ent->position.x = (tile_idx % tilemap.width) * tilemap.tile_size + (tilemap.tile_size >> 1);
                        p_ent->position.y = (tile_idx / tilemap.width) * tilemap.tile_size + (tilemap.tile_size >> 1);;
                    }
                }
                break;
                case SPAWN_CHEST:
                    if (data->coins.total < 65535)
                    {
                        spawn_chest(scene, tile_idx);
                        data->coins.total++;
                    }
                break;
            }
            change_a_tile(&tilemap, tile_idx, new_type);
            last_tile_idx = tile_idx;
            CWaterRunner_t* p_crunner;
            sc_map_foreach_value(&scene->ent_manager.component_map[CWATERRUNNER_T], p_crunner)
            {
                p_crunner->state = BFS_RESET;
            }
        }
        else if (action == ACTION_REMOVE_TILE && pressed)
        {
            change_a_tile(&tilemap, tile_idx, EMPTY_TILE);
            tilemap.tiles[tile_idx].water_level = 0;

            Entity_t* ent;
            unsigned int m_id;
            sc_map_foreach(&tilemap.tiles[tile_idx].entities_set, m_id, ent)
            {
                if (ent->m_tag == PLAYER_ENT_TAG) continue;

                if (!ent->m_alive) continue;

                if (ent->m_tag == CHEST_ENT_TAG)
                {
                    data->coins.total--;
                }
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
                //remove_entity(&scene->ent_manager, m_id);
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
        else
        {
            last_tile_idx = MAX_N_TILES;
        }
        return;
    }

    if (point_in_AABB(scene->mouse_pos, scene->layers.render_layers[SELECTION_LAYER].render_area))
    {
        Vector2 raw_mouse_pos = Vector2Subtract(scene->mouse_pos,
            (Vector2){
                scene->layers.render_layers[SELECTION_LAYER].render_area.x,
                scene->layers.render_layers[SELECTION_LAYER].render_area.y
            }
        );
        if (action == ACTION_SPAWN_TILE && !pressed)
        {
            current_spawn_selection = ((int)raw_mouse_pos.x / SELECTION_TILE_SIZE);
        }
    }
}

static void restart_editor_level(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    memset(&data->coins, 0, sizeof(data->coins));

    TileGrid_t tilemap = data->tilemap;
    for (size_t i = 0; i < tilemap.n_tiles;i++)
    {
        tilemap.tiles[i].solid = NOT_SOLID;
        tilemap.tiles[i].tile_type = EMPTY_TILE;
        tilemap.tiles[i].rotation = TILE_NOROTATE;
        tilemap.tiles[i].moveable = true;
        tilemap.tiles[i].max_water_level = 4;
        tilemap.tiles[i].water_level = 0;
        tilemap.tiles[i].wet = false;
        tilemap.tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
        Entity_t* ent;
        unsigned int m_id;
        sc_map_foreach(&tilemap.tiles[i].entities_set, m_id, ent)
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
            //remove_entity(&scene->ent_manager, m_id);
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
    Entity_t* p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities_map[LEVEL_END_TAG], p_ent)
    {
        remove_entity(&scene->ent_manager, p_ent->m_id);
    }
    update_entity_manager(&scene->ent_manager);
    for (size_t i = 0; i < tilemap.width; ++i)
    {
        unsigned int tile_idx = (tilemap.height - 1) * tilemap.width + i;
        change_a_tile(&tilemap, tile_idx, SOLID_TILE);
    }
}

static void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_player;
    //sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CPlayerState_t* p_playerstate = get_component(p_player, CPLAYERSTATE_T);
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
                const Color crate_colour = metal_toggle ? GRAY : BROWN;
                Vector2 draw_pos = {SPAWN_CRATE * SELECTION_TILE_SIZE , 0};
                BeginTextureMode(scene->layers.render_layers[SELECTION_LAYER].layer_tex);
                for (uint8_t i = SPAWN_CRATE; i <= SPAWN_CRATE_BOMB; ++i)
                {
                    DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, crate_colour);
                    Sprite_t* spr;
                    Vector2 half_size = {SELECTION_TILE_HALFSIZE, SELECTION_TILE_HALFSIZE};
                    switch (i)
                    {
                        case SPAWN_CRATE:
                            spr = get_sprite(&scene->engine->assets, metal_toggle? "m_crate":"w_crate");
                            if (spr != NULL)
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        break;
                        case SPAWN_CRATE_ARROW_L:
                            spr = get_sprite(&scene->engine->assets, metal_toggle? "m_la_crate":"w_la_crate");
                            if (spr == NULL)
                            {
                                DrawLine(
                                    draw_pos.x,
                                    draw_pos.y + half_size.y,
                                    draw_pos.x + half_size.x,
                                    draw_pos.y + half_size.y,
                                    BLACK
                                );
                            }
                            else
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        break;
                        case SPAWN_CRATE_ARROW_R:
                        {
                            Sprite_t* spr = get_sprite(&scene->engine->assets, metal_toggle? "m_ra_crate":"w_ra_crate");
                            if (spr == NULL)
                            {
                                DrawLine(
                                    draw_pos.x + half_size.x,
                                    draw_pos.y + half_size.y,
                                    draw_pos.x + half_size.x * 2,
                                    draw_pos.y + half_size.y,
                                    BLACK
                                );
                            }
                            else
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        }
                        break;
                        case SPAWN_CRATE_ARROW_U:
                        {
                            Sprite_t* spr = get_sprite(&scene->engine->assets, metal_toggle? "m_ua_crate":"w_ua_crate");
                            if (spr == NULL)
                            {
                                DrawLine(
                                    draw_pos.x + half_size.x,
                                    draw_pos.y,
                                    draw_pos.x + half_size.x,
                                    draw_pos.y + half_size.y,
                                    BLACK
                                );
                            }
                            else
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        }
                        break;
                        case SPAWN_CRATE_ARROW_D:
                        {
                            Sprite_t* spr = get_sprite(&scene->engine->assets, metal_toggle? "m_da_crate":"w_da_crate");
                            if (spr == NULL)
                            {
                                DrawLine(
                                    draw_pos.x + half_size.x,
                                    draw_pos.y + half_size.y,
                                    draw_pos.x + half_size.x,
                                    draw_pos.y + half_size.y * 2,
                                    BLACK
                                );
                            }
                            else
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        }
                        break;
                        case SPAWN_CRATE_BOMB:
                        {
                            Sprite_t* spr = get_sprite(&scene->engine->assets, metal_toggle? "m_b_crate":"w_b_crate");
                            if (spr == NULL)
                            {
                                DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, BLACK);
                            }
                            else
                            {
                                draw_sprite(spr, 0, draw_pos, 0.0, false);
                            }
                        }
                        break;
                    }
                    draw_pos.x += SELECTION_TILE_SIZE;
                }
                EndTextureMode();
            break;
            case ACTION_CRATE_ACTIVATION:
                if (!pressed) crate_activation = !crate_activation;
            break;
            case ACTION_EXIT:
                if(scene->engine != NULL)
                {
                    change_scene(scene->engine, 0);
                }
            break;
            case ACTION_NEXTLEVEL:
            case ACTION_RESTART:
                restart_editor_level(scene);
            break;
            case ACTION_TOGGLE_GRID:
                if (!pressed)
                {
                    data->show_grid = !data->show_grid;
                }
            break;
            case ACTION_SET_SPAWNPOINT:
                p_player->spawn_pos = p_player->position;
            break;
            case ACTION_TOGGLE_TIMESLOW:
                if (!pressed)
                {
                    if (scene->time_scale < 1.0f)
                    {
                        scene->time_scale = 1.0f;
                    }
                    else
                    {
                        scene->time_scale = 0.25f;
                    }
                }
            break;
            case ACTION_SPAWN_TILE:
            case ACTION_REMOVE_TILE:
                toggle_block_system(scene, action, pressed);
                update_entity_manager(&scene->ent_manager);
            default:
            break;
            case ACTION_SWITCH_TILESET:
                if (!pressed)
                {
                    data->selected_solid_tilemap++;
                    if (data->selected_solid_tilemap >= N_SOLID_TILESETS) data->selected_solid_tilemap = 0;

                    data->solid_tile_sprites = get_sprite(&scene->engine->assets, SOLID_TILE_SELECTIONS[data->selected_solid_tilemap]);
                }
            break;
        }
    }
}

void init_sandbox_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);
    init_entity_tag_map(&scene->scene.ent_manager, BOULDER_ENT_TAG, MAX_COMP_POOL_SIZE);
    init_entity_tag_map(&scene->scene.ent_manager, LEVEL_END_TAG, 16);
    init_entity_tag_map(&scene->scene.ent_manager, DYNMEM_ENT_TAG, 16);

    scene->data.tilemap.tiles = all_tiles;
    init_level_scene_data(
        &scene->data, MAX_N_TILES, all_tiles,
        (Rectangle){25, 25, VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE}
    );
    scene->data.show_grid = true;
    scene->data.tile_sprites[ONEWAY_TILE] = get_sprite(&scene->scene.engine->assets, "tl_owp");
    scene->data.tile_sprites[LADDER] = get_sprite(&scene->scene.engine->assets, "tl_ldr");
    scene->data.tile_sprites[SPIKES] = get_sprite(&scene->scene.engine->assets, "d_spikes");
    scene->data.tile_sprites[SPIKES + TILE_90CWROT] = get_sprite(&scene->scene.engine->assets, "l_spikes");
    scene->data.tile_sprites[SPIKES + TILE_90CCWROT] = get_sprite(&scene->scene.engine->assets, "r_spikes");
    scene->data.tile_sprites[SPIKES + TILE_180ROT] = get_sprite(&scene->scene.engine->assets, "u_spikes");
    scene->data.selected_solid_tilemap = 0;
    scene->data.solid_tile_sprites = get_sprite(&scene->scene.engine->assets, SOLID_TILE_SELECTIONS[0]);
    Texture2D* tex = get_texture(&scene->scene.engine->assets, "bg_tex");
    SetTextureWrap(*tex, TEXTURE_WRAP_REPEAT);

    for (size_t i = 0; i < scene->data.tilemap.width; ++i)
    {
        unsigned int tile_idx = (scene->data.tilemap.height - 1) * scene->data.tilemap.width + i;
        change_a_tile(&scene->data.tilemap, tile_idx, SOLID_TILE);
    }

    scene->scene.bg_colour = LIGHTGRAY;
    add_scene_layer(
        &scene->scene, scene->data.game_rec.width, scene->data.game_rec.height,
        scene->data.game_rec
    );
    add_scene_layer(
        &scene->scene, SELECTION_REGION_WIDTH, SELECTION_REGION_HEIGHT,
        (Rectangle){
            scene->data.game_rec.x, scene->data.game_rec.y + scene->data.game_rec.height + SELECTION_GAP,
            SELECTION_REGION_WIDTH, SELECTION_REGION_HEIGHT
        }
    );
    add_scene_layer(
        &scene->scene, scene->scene.engine->intended_window_size.x,
        scene->scene.engine->intended_window_size.y,
        (Rectangle){
            0, 0,
            scene->scene.engine->intended_window_size.x,
            scene->scene.engine->intended_window_size.y
        }
    );

    BeginTextureMode(scene->scene.layers.render_layers[SELECTION_LAYER].layer_tex);
        ClearBackground(LIGHTGRAY);
        Vector2 draw_pos = {0, 0};
        const Color crate_colour = metal_toggle ? GRAY : BROWN;
        const Color draw_colour[MAX_SPAWN_TYPE] = {
            BLACK, MAROON, ORANGE, ColorAlpha(RAYWHITE, 0.5), ColorAlpha(BLUE, 0.5), 
            ColorAlpha(RAYWHITE, 0.5), YELLOW, crate_colour, crate_colour, crate_colour,
            crate_colour, crate_colour, crate_colour,
            ColorAlpha(RAYWHITE, 0.5), ColorAlpha(RAYWHITE, 0.5), ColorAlpha(RAYWHITE, 0.5) 
        };
        for (uint8_t i = 0; i < MAX_SPAWN_TYPE; ++i)
        {
            DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
            Sprite_t* spr;
            Vector2 half_size = {SELECTION_TILE_HALFSIZE, SELECTION_TILE_HALFSIZE};
            switch (i)
            {
                case TOGGLE_ONEWAY:
                    if (scene->data.tile_sprites[ONEWAY_TILE] != NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, LIGHTGRAY);
                        draw_sprite(scene->data.tile_sprites[ONEWAY_TILE], 0, draw_pos, 0.0, false);
                    }
                break;
                case TOGGLE_LADDER:
                    if (scene->data.tile_sprites[LADDER] != NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, LIGHTGRAY);
                        draw_sprite(scene->data.tile_sprites[LADDER], 0, draw_pos, 0.0, false);
                    }
                break;
                case TOGGLE_SPIKE:
                    if (scene->data.tile_sprites[SPIKES] != NULL)
                    {
                        draw_sprite(scene->data.tile_sprites[SPIKES], 0, draw_pos, 0.0, false);
                    }
                    else
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y + SELECTION_TILE_HALFSIZE, SELECTION_TILE_SIZE, SELECTION_TILE_HALFSIZE, RED);
                    }
                break;
                case SPAWN_BOULDER:
                    DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, GRAY);
                break;
                case SPAWN_CHEST:
                    spr = get_sprite(&scene->scene.engine->assets, "chest");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                break;
                case SPAWN_CRATE:
                    spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_crate":"w_crate");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                break;
                case SPAWN_CRATE_ARROW_L:
                    spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_la_crate":"w_la_crate");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                        DrawLine(
                            draw_pos.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                break;
                case SPAWN_CRATE_ARROW_R:
                {
                    Sprite_t* spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_ra_crate":"w_ra_crate");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x * 2,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                }
                break;
                case SPAWN_CRATE_ARROW_U:
                {
                    Sprite_t* spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_ua_crate":"w_ua_crate");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            BLACK
                        );
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                }
                break;
                case SPAWN_CRATE_ARROW_D:
                {
                    Sprite_t* spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_da_crate":"w_da_crate");
                    if (spr == NULL)
                    {
                        DrawRectangle(draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE, draw_colour[i]);
                        DrawLine(
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y,
                            draw_pos.x + half_size.x,
                            draw_pos.y + half_size.y * 2,
                            BLACK
                        );
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                }
                break;
                case SPAWN_CRATE_BOMB:
                {
                    Sprite_t* spr = get_sprite(&scene->scene.engine->assets, metal_toggle? "m_b_crate":"w_b_crate");
                    if (spr == NULL)
                    {
                        DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, BLACK);
                    }
                    else
                    {
                        draw_sprite(spr, 0, draw_pos, 0.0, false);
                    }
                }
                break;
                case SPAWN_WATER_RUNNER:
                    DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, ColorAlpha(BLUE, 0.2));
                break;
                case TOGGLE_AIR_POCKET:
                    DrawRectangleLinesEx((Rectangle){draw_pos.x, draw_pos.y, SELECTION_TILE_SIZE, SELECTION_TILE_SIZE}, 2.0, ColorAlpha(BLUE, 0.5));
                break;
                case SPAWN_LEVEL_END:
                    DrawCircleV(Vector2Add(draw_pos, half_size), half_size.x, GREEN);
                break;

            }
            draw_pos.x += SELECTION_TILE_SIZE;
        }



    EndTextureMode();

    create_player(&scene->scene.ent_manager);
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
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    sc_array_add(&scene->scene.systems, &player_crushing_system);
    sc_array_add(&scene->scene.systems, &spike_collision_system);
    sc_array_add(&scene->scene.systems, &edge_velocity_check_system);
    sc_array_add(&scene->scene.systems, &state_transition_update_system);
    sc_array_add(&scene->scene.systems, &update_entity_emitter_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
    sc_array_add(&scene->scene.systems, &lifetimer_update_system);
    sc_array_add(&scene->scene.systems, &airtimer_update_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
    sc_array_add(&scene->scene.systems, &camera_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &update_water_runner_system);
    sc_array_add(&scene->scene.systems, &player_respawn_system);
    sc_array_add(&scene->scene.systems, &level_end_detection_system);
    sc_array_add(&scene->scene.systems, &render_editor_game_scene);
    sc_array_add(&scene->scene.systems, &level_scene_render_func);

    // This avoid graphical glitch, not essential
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, KEY_W, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_RIGHT);

    sc_map_put_64(&scene->scene.action_map, KEY_SPACE, ACTION_JUMP);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_PREV_SPAWN);
    sc_map_put_64(&scene->scene.action_map, KEY_E, ACTION_NEXT_SPAWN);
    sc_map_put_64(&scene->scene.action_map, KEY_F, ACTION_METAL_TOGGLE);
    sc_map_put_64(&scene->scene.action_map, KEY_T, ACTION_CRATE_ACTIVATION);
    sc_map_put_64(&scene->scene.action_map, KEY_L, ACTION_EXIT);
    sc_map_put_64(&scene->scene.action_map, KEY_R, ACTION_RESTART);
    sc_map_put_64(&scene->scene.action_map, KEY_B, ACTION_TOGGLE_GRID);
    sc_map_put_64(&scene->scene.action_map, KEY_Z, ACTION_SWITCH_TILESET);
    sc_map_put_64(&scene->scene.action_map, KEY_V, ACTION_SET_SPAWNPOINT);
    sc_map_put_64(&scene->scene.action_map, KEY_U, ACTION_TOGGLE_TIMESLOW);
    sc_map_put_64(&scene->scene.action_map, MOUSE_LEFT_BUTTON, ACTION_SPAWN_TILE);
    sc_map_put_64(&scene->scene.action_map, MOUSE_RIGHT_BUTTON, ACTION_REMOVE_TILE);

}

void free_sandbox_scene(LevelScene_t* scene)
{
    free_scene(&scene->scene);
    term_level_scene_data(&scene->data);
}
