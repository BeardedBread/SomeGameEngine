#include "tracy/TracyC.h"

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
static RenderInfoNode all_tile_rendernodes[MAX_N_TILES] = {0};

#define GAME_LAYER 0
#define CONTROL_LAYER 1
static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    static char buffer[512];
    BeginTextureMode(scene->layers.render_layers[CONTROL_LAYER].layer_tex);
        ClearBackground(BLANK);

        if (data->camera.mode == CAMERA_RANGED_MOVEMENT)
        {
            // TL
            DrawLineEx((Vector2){32,32},
                (Vector2){64,32}, 5, WHITE);
            DrawLineEx((Vector2){32,32},
                (Vector2){32,64}, 5, WHITE);
            //TR
            DrawLineEx((Vector2){data->game_rec.width - 64,32},
                (Vector2){data->game_rec.width - 32, 32}, 5, WHITE);
            DrawLineEx((Vector2){data->game_rec.width - 32,32},
                (Vector2){data->game_rec.width - 32, 64}, 5, WHITE);
            //BL
            DrawLineEx((Vector2){32,data->game_rec.height-32},
                (Vector2){64,data->game_rec.height - 32}, 5, WHITE);
            DrawLineEx((Vector2){32,data->game_rec.height-64},
                (Vector2){32, data->game_rec.height - 32}, 5, WHITE);
            //BR
            DrawLineEx((Vector2){data->game_rec.width - 64,data->game_rec.height-32},
                (Vector2){data->game_rec.width - 32,data->game_rec.height - 32}, 5, WHITE);
            DrawLineEx((Vector2){data->game_rec.width - 32,data->game_rec.height-64},
                (Vector2){data->game_rec.width - 32, data->game_rec.height - 32}, 5, WHITE);
        }

        Entity_t* p_ent;
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            CAirTimer_t* p_air = get_component(p_ent, CAIRTIMER_T);

            Sprite_t* spr = get_sprite(&scene->engine->assets, "p_bigbubble");
            Vector2 air_pos = {data->game_rec.width - 32, data->game_rec.height - 32};
            for (uint8_t i = 0; i < p_air->curr_count; i++)
            {
                draw_sprite(spr, 0, air_pos, 0, false);
                //DrawCircleV(air_pos, 16, BLUE);
                air_pos.x -= 32;
            }
        }
        // For DEBUG
        int gui_x = 5;
        sprintf(buffer, "%u %u", sc_map_size_64v(&scene->ent_manager.entities), GetFPS());
        DrawText(buffer, gui_x, data->game_rec.height - 12, 12, WHITE);

        DrawRectangle(0, 0, data->game_rec.width, 32, (Color){0,0,0,128});
        DrawText("Z", 300, 5, 24, RED);
        if (data->camera.mode == CAMERA_RANGED_MOVEMENT)
        {
            DrawText("Eyes", 320, 5, 24, RED);
        }
        DrawText(data->level_pack->levels[data->current_level].level_name, 5, 5, 24, WHITE);
        sprintf(buffer, "Chests: %u / %u", data->coins.current, data->coins.total);
        gui_x = data->game_rec.width - MeasureText(buffer, 24) - 5;
        DrawText(buffer, gui_x, 5, 24, RED);
    EndTextureMode();
}

static void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    CPlayerState_t* p_playerstate;
    sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    {
        switch(action)
        {
            case ACTION_UP:
                p_playerstate->player_dir.y = (pressed)? -1 : 0;
                if (data->camera.mode == CAMERA_RANGED_MOVEMENT) data->camera.cam.target.y -= 20;
            break;
            case ACTION_DOWN:
                p_playerstate->player_dir.y = (pressed)? 1 : 0;
                if (data->camera.mode == CAMERA_RANGED_MOVEMENT) data->camera.cam.target.y += 20;
            break;
            case ACTION_LEFT:
                p_playerstate->player_dir.x = (pressed)? -1 : 0;
                if (data->camera.mode == CAMERA_RANGED_MOVEMENT) data->camera.cam.target.x -= 20;
            break;
            case ACTION_RIGHT:
                p_playerstate->player_dir.x = (pressed)? 1 : 0;
                if (data->camera.mode == CAMERA_RANGED_MOVEMENT) data->camera.cam.target.x += 20;
            break;
            case ACTION_JUMP:
                p_playerstate->jump_pressed = pressed;
            break;
            case ACTION_LOOKAHEAD:
                p_playerstate->locked = pressed;
                data->camera.mode = pressed ? CAMERA_RANGED_MOVEMENT : CAMERA_FOLLOW_PLAYER;
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
            case ACTION_EXIT:
                if(scene->engine != NULL)
                {
                    change_scene(scene->engine, MAIN_MENU_SCENE);
                }
            break;
            default:
            break;
        }
    }
}

static void render_regular_game_scene(Scene_t* scene)
{
    TracyCZoneN(ctx, "GameRender", true)
    // This function will render the game scene outside of the intended draw function
    // Just for clarity and separation of logic
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_ent;
    Vector2 min = GetScreenToWorld2D((Vector2){0, 0}, data->camera.cam);
    Vector2 max = GetScreenToWorld2D(
        (Vector2){
            data->game_rec.width,
            data->game_rec.height
        },
        data->camera.cam
    );
    min = Vector2Scale(min, 1.0f/tilemap.tile_size);
    max = Vector2Scale(max, 1.0f/tilemap.tile_size);
    min.x = (int)fmax(0, min.x - 1);
    min.y = (int)fmax(0, min.y - 1);
    max.x = (int)fmin(tilemap.width, max.x + 1);
    max.y = (int)fmin(tilemap.height, max.y + 1);

    // Queue TileMap rendering
    for (int tile_y = min.y; tile_y < max.y; tile_y++)
    {
        for (int tile_x = min.x; tile_x < max.x; tile_x++)
        {

            int i = tile_x + tile_y * tilemap.width;
            // Tile render nodes is static on load and is not dynamically updated.
            // We exploit the fact that all tiles can only change into an empty
            // tile.
            // This won't work if a tile can change into something else
            if (tilemap.tiles[i].tile_type == EMPTY_TILE) continue;

            uint8_t depth = 2;
            if (tilemap.tiles[i].tile_type == LADDER)
            {
                depth = 0;
            }
            add_render_node(&data->render_manager, all_tile_rendernodes + i, depth);
        }
    }

    // Queue Sprite rendering
    unsigned int ent_idx;
    CSprite_t* p_cspr;
    sc_map_foreach(&scene->ent_manager.component_map[CSPRITE_T], ent_idx, p_cspr)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;

        const SpriteRenderInfo_t spr = p_cspr->sprites[p_cspr->current_idx];
        if (spr.sprite == NULL) continue;

        Vector2 pos = p_ent->position;
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
        if (p_bbox != NULL)
        {
            pos = Vector2Add(
                pos,
                get_anchor_offset(p_bbox->size, spr.dest_anchor, p_cspr->node.flip & 1)
            );
        }
        pos = Vector2Subtract(
            pos,
            get_anchor_offset(spr.sprite->frame_size, spr.src_anchor, p_cspr->node.flip & 1)
        );

        Vector2 offset = spr.offset;
        if (p_cspr->node.flip & 1) offset.x *= -1;

        pos = Vector2Add(pos, offset);
        // Entity culling
        if (
            pos.x + spr.sprite->frame_size.x < min.x * tilemap.tile_size
            || pos.x > max.x * tilemap.tile_size
            || pos.y + spr.sprite->frame_size.y < min.y * tilemap.tile_size
            || pos.y > max.y * tilemap.tile_size
        )
        {
            continue;
        }

        if (p_ent->m_tag == LEVEL_END_TAG)
        {
            p_cspr->current_frame = 2 * data->selected_solid_tilemap + (
                            (data->coins.current < data->coins.total) ? 0 : 1);
        }

        p_cspr->node.spr = spr.sprite;
        p_cspr->node.frame_num = p_cspr->current_frame;
        p_cspr->node.pos = pos;
        add_render_node(&data->render_manager, &p_cspr->node, p_cspr->depth);
    }

    Texture2D* bg = get_texture(&scene->engine->assets, "bg_tex");

    BeginTextureMode(scene->layers.render_layers[GAME_LAYER].layer_tex);
        ClearBackground(WHITE);
        DrawTexturePro(*bg,
            //(Rectangle){0,0,64,64},
            (Rectangle){min.x,0, data->game_rec.width, data->game_rec.height},
            (Rectangle){0,0, data->game_rec.width, data->game_rec.height},
            //(Rectangle){0,0,game_rec.width, game_rec.height},
            (Vector2){0,0}, 0.0f, WHITE
        );

        BeginMode2D(data->camera.cam);

#ifdef ENABLE_RENDER_FALLBACK
        for (int tile_y = min.y; tile_y < max.y; tile_y++)
        {
            for (int tile_x = min.x; tile_x < max.x; tile_x++)
            {
                int i = tile_x + tile_y * tilemap.width;
                int x = tile_x * TILE_SIZE;
                int y = tile_y * TILE_SIZE;

                if (tilemap.tiles[i].tile_type == LADDER)
                {
                    if (data->tile_sprites[tilemap.tiles[i].tile_type] == NULL)
                    {
                        DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, ORANGE);
                    }
                }
            }
        }
        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);

            // Entity culling
            Vector2 box_size = {0};
            if (p_bbox != NULL) box_size = p_bbox->size;
            if (
                p_ent->position.x + box_size.x < min.x * tilemap.tile_size
                || p_ent->position.x > max.x * tilemap.tile_size
                || p_ent->position.y + box_size.y < min.y * tilemap.tile_size
                || p_ent->position.y > max.y * tilemap.tile_size
            ) continue;

            CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
            if (p_cspr != NULL) continue;

            // Continue here only if no sprite
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

            if (p_ent->m_tag == LEVEL_END_TAG)
            {
                DrawCircleV(p_ent->position, tilemap.tile_size >> 1, (data->coins.current < data->coins.total)? RED : GREEN);
                continue;
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
        }
        for (int tile_y = min.y; tile_y < max.y; tile_y++)
        {
            for (int tile_x = min.x; tile_x < max.x; tile_x++)
            {
                int i = tile_x + tile_y * tilemap.width;
                int x = tile_x * TILE_SIZE;
                int y = tile_y * TILE_SIZE;

                if (tilemap.tiles[i].tile_type != LADDER)
                {
                    uint8_t tile_sprite_idx = tilemap.tiles[i].tile_type + tilemap.tiles[i].rotation;
                    if (tilemap.tiles[i].tile_type == SOLID_TILE)
                    {
                        if (data->solid_tile_sprites == NULL) {
                            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
                        }
                    }
                    else if (data->tile_sprites[tile_sprite_idx] == NULL)
                    {
                        if (tilemap.tiles[i].tile_type == ONEWAY_TILE)
                        {
                            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, MAROON);
                        }
                        else if (tilemap.tiles[i].tile_type == SPIKES)
                        {
                            DrawRectangle(
                                x + tilemap.tiles[i].offset.x, y + tilemap.tiles[i].offset.y,
                                tilemap.tiles[i].size.x, tilemap.tiles[i].size.y, RED
                            );
                        }
                    }
                }
            }
        }
#endif //ENABLE_RENDER_FALLBACK

        execute_render(&data->render_manager);

        // Particle effect will always be in front of everything except water
        draw_particle_system(&scene->part_sys);

        // Render water
        for (int tile_y = min.y; tile_y < max.y; tile_y++)
        {
            for (int tile_x = min.x; tile_x < max.x; tile_x++)
            {
                int i = tile_x + tile_y * tilemap.width;
                if (!tilemap.tiles[i].wet && tilemap.tiles[i].water_level == 0) {
                    continue;
                }

                int x = tile_x * TILE_SIZE;
                int y = tile_y * TILE_SIZE;

                // Draw water flow
#define SURFACE_THICKNESS 4
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

                //if (tilemap.tiles[i].max_water_level < MAX_WATER_LEVEL)
                //{
                //    DrawRectangleLinesEx((Rectangle){x, y, TILE_SIZE, TILE_SIZE}, 2.0, ColorAlpha(BLUE, 0.5));
                //}

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
        EndMode2D();
    EndTextureMode();
    TracyCZoneEnd(ctx)
}

static void at_level_start(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    data->sm.state = LEVEL_STATE_RUNNING;
}

static void at_level_complete(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    data->sm.fractional += scene->delta_time;
    if (data->sm.fractional > 1.0f)
    {
        data->sm.fractional -= 1.0f;
        data->sm.counter++;
    }
    if (data->sm.counter >= 1)
    {
        do_action(scene, ACTION_NEXTLEVEL, false);
        data->sm.state = LEVEL_STATE_STARTING;
    }
}

void init_game_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_do_action, ENABLE_ENTITY_MANAGEMENT_SYSTEM | ENABLE_PARTICLE_SYSTEM);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);
    init_entity_tag_map(&scene->scene.ent_manager, BOULDER_ENT_TAG, MAX_COMP_POOL_SIZE);
    init_entity_tag_map(&scene->scene.ent_manager, LEVEL_END_TAG, 16);
    init_entity_tag_map(&scene->scene.ent_manager, DYNMEM_ENT_TAG, 16);

    scene->data.tilemap.tiles = all_tiles;
    scene->data.tilemap.render_nodes = all_tile_rendernodes;
    init_level_scene_data(
        &scene->data, MAX_N_TILES, all_tiles,
        (Rectangle){
            0,0,
            VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE
        }
    );
    for (size_t i = 0; i < MAX_N_TILES; i++)
    {
        memset(all_tile_rendernodes + i, 0, sizeof(RenderInfoNode));
        all_tile_rendernodes[i].pos = (Vector2){
            (i % scene->data.tilemap.width) * TILE_SIZE,
            (i / scene->data.tilemap.width) * TILE_SIZE,
        };
        all_tile_rendernodes[i].scale = (Vector2){1,1};
        all_tile_rendernodes[i].colour = WHITE;
    }
    scene->data.sm.state_functions[LEVEL_STATE_STARTING] = at_level_start;
    scene->data.sm.state_functions[LEVEL_STATE_RUNNING] = NULL;
    scene->data.sm.state_functions[LEVEL_STATE_DEAD] = NULL;
    scene->data.sm.state_functions[LEVEL_STATE_COMPLETE] = at_level_complete;

    scene->scene.bg_colour = LIGHTGRAY;
    add_scene_layer(
        &scene->scene, scene->data.game_rec.width, scene->data.game_rec.height,
        scene->data.game_rec
    );
    add_scene_layer(
        &scene->scene,
        scene->data.game_rec.width, scene->data.game_rec.height,
        scene->data.game_rec
    );

    create_player(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    // Set up textures
    scene->data.solid_tile_sprites = get_sprite(&scene->scene.engine->assets, "stile0");

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
    sc_array_add(&scene->scene.systems, &update_entity_emitter_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
    sc_array_add(&scene->scene.systems, &lifetimer_update_system);
    sc_array_add(&scene->scene.systems, &airtimer_update_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
    sc_array_add(&scene->scene.systems, &camera_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &update_water_runner_system);
    sc_array_add(&scene->scene.systems, &check_player_dead_system);
    sc_array_add(&scene->scene.systems, &level_end_detection_system);
    sc_array_add(&scene->scene.systems, &level_state_management_system);
    sc_array_add(&scene->scene.systems, &render_regular_game_scene);
    sc_array_add(&scene->scene.systems, &level_scene_render_func);
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
    sc_map_put_64(&scene->scene.action_map, KEY_Z, ACTION_LOOKAHEAD);

}

void free_game_scene(LevelScene_t* scene)
{
    clear_all_game_entities(scene);
    free_scene(&scene->scene);
    term_level_scene_data(&scene->data);
}
