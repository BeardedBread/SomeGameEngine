#include "scene_impl.h"
#include "game_systems.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

enum EntitySpawnSelection
{
    TOGGLE_TILE = 0,
    SPAWN_CRATE,
    SPAWN_METAL_CRATE,
};
#define MAX_SPAWN_TYPE 3
static unsigned int current_spawn_selection = 0;

static inline unsigned int get_tile_idx(int x, int y, unsigned int tilemap_width)
{
    unsigned int tile_x = x / TILE_SIZE;
    unsigned int tile_y = y / TILE_SIZE;
    return tile_y * tilemap_width + tile_x;
}

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_ent;

    for (size_t i=0; i<tilemap.n_tiles;++i)
    {
        char buffer[6] = {0};
        int x = (i % tilemap.width) * TILE_SIZE;
        int y = (i / tilemap.width) * TILE_SIZE;
        sprintf(buffer, "%u", sc_map_size_64(&tilemap.tiles[i].entities_set));

        if (tilemap.tiles[i].solid)
        {
            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
        }
        else if (tilemap.tiles[i].water_level > 0)
        {
            // Draw water tile
            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLUE);
        }
    }

    char buffer[64] = {0};
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        Color colour;
        switch(p_ent->m_tag)
        {
            case PLAYER_ENT_TAG:
                colour = RED;
            break;
            case CRATES_ENT_TAG:
                colour = p_bbox->fragile? BROWN : GRAY;
            break;
            default:
                colour = BLACK;
        }
        DrawRectangle(p_ct->position.x, p_ct->position.y, p_bbox->size.x, p_bbox->size.y, colour);
        CHurtbox_t* p_hurtbox = get_component(&scene->ent_manager, p_ent, CHURTBOX_T);
        CHitBox_t* p_hitbox = get_component(&scene->ent_manager, p_ent, CHITBOX_T);
        if (p_hitbox != NULL)
        {
            Rectangle rec = {
                .x = p_ct->position.x + p_hitbox->offset.x,
                .y = p_ct->position.y + p_hitbox->offset.y,
                .width = p_hitbox->size.x,
                .height = p_hitbox->size.y,
            };
            DrawRectangleLinesEx(rec, 1.5, ORANGE);
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
    }

    for (size_t i=0; i<tilemap.n_tiles;++i)
    {
        int x = (i % tilemap.width) * TILE_SIZE;
        int y = (i / tilemap.width) * TILE_SIZE;
        sprintf(buffer, "%u", sc_map_size_64(&tilemap.tiles[i].entities_set));

        if (tilemap.tiles[i].solid)
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
    for (size_t i=0; i<tilemap.width;++i)
    {
        int x = (i+1)*TILE_SIZE;
        DrawLine(x, 0, x, tilemap.height * TILE_SIZE, BLACK);
    }
    for (size_t i=0; i<tilemap.height;++i)
    {
        int y = (i+1)*TILE_SIZE;
        DrawLine(0, y, tilemap.width * TILE_SIZE, y, BLACK);
    }

    // For DEBUG
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_ent, CJUMP_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_ent, CMOVEMENTSTATE_T);
        sprintf(buffer, "Pos: %.3f\n %.3f", p_ct->position.x, p_ct->position.y);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 15, 12, BLACK);
        sprintf(buffer, "Vel: %.3f\n %.3f", p_ct->velocity.x, p_ct->velocity.y);
        DrawText(buffer, tilemap.width * TILE_SIZE + 128, 15, 12, BLACK);
        //sprintf(buffer, "Accel: %.3f\n %.3f", p_ct->accel.x, p_ct->accel.y);
        //DrawText(buffer, tilemap.width * TILE_SIZE + 128, 60, 12, BLACK);
        sprintf(buffer, "Jumps: %u", p_cjump->jumps);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 60, 12, BLACK);
        sprintf(buffer, "Crouch: %u", p_pstate->is_crouch);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 90, 12, BLACK);
        sprintf(buffer, "Water: %s", p_mstate->water_state & 1? "YES":"NO");
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 120, 12, BLACK);
    }
    sprintf(buffer, "Spawn Entity: %u", current_spawn_selection);
    DrawText(buffer, tilemap.width * TILE_SIZE + 1, 240, 12, BLACK);
    sprintf(buffer, "Number of Entities: %u", sc_map_size_64v(&scene->ent_manager.entities));
    DrawText(buffer, tilemap.width * TILE_SIZE + 1, 270, 12, BLACK);
    sprintf(buffer, "FPS: %u", GetFPS());
    DrawText(buffer, tilemap.width * TILE_SIZE + 1, 320, 12, BLACK);

    static char mempool_stats[512];
    print_mempool_stats(mempool_stats);
    DrawText(mempool_stats, tilemap.width * TILE_SIZE + 1, 350, 12, BLACK);
}

static void spawn_crate(Scene_t *scene, unsigned int tile_idx, bool metal)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    Entity_t *p_crate = add_entity(&scene->ent_manager, CRATES_ENT_TAG);
    CBBox_t *p_bbox = add_component(&scene->ent_manager, p_crate, CBBOX_COMP_T);
    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = !metal;
    CTransform_t *p_ctransform = add_component(&scene->ent_manager, p_crate, CTRANSFORM_COMP_T);
    p_ctransform->position.x = (tile_idx % data->tilemap.width) * TILE_SIZE;
    p_ctransform->position.y = (tile_idx / data->tilemap.width) * TILE_SIZE;
    add_component(&scene->ent_manager, p_crate, CMOVEMENTSTATE_T);
    add_component(&scene->ent_manager, p_crate, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(&scene->ent_manager, p_crate, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->fragile = !metal;
}

static void spawn_player(Scene_t *scene)
{
    Entity_t *p_ent = add_entity(&scene->ent_manager, PLAYER_ENT_TAG);

    CBBox_t *p_bbox = add_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
    set_bbox(p_bbox, PLAYER_WIDTH, PLAYER_HEIGHT);
    add_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
    CJump_t *p_cjump = add_component(&scene->ent_manager, p_ent, CJUMP_COMP_T);
    p_cjump->jump_speed = 680;
    p_cjump->jumps = 1;
    p_cjump->max_jumps = 1;
    p_cjump->jump_ready = true;
    add_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
    add_component(&scene->ent_manager, p_ent, CTILECOORD_COMP_T);
    add_component(&scene->ent_manager, p_ent, CMOVEMENTSTATE_T);
    CHitBox_t* p_hitbox = add_component(&scene->ent_manager, p_ent, CHITBOX_T);
    p_hitbox->size = Vector2Add(p_bbox->size, (Vector2){2,2});
    p_hitbox->offset = (Vector2){-1, -1};
}

static void toggle_block_system(Scene_t *scene)
{
    // TODO: This system is not good as the interface between raw input and actions is broken
    static unsigned int last_tile_idx = MAX_N_TILES;
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        unsigned int tile_idx = get_tile_idx(GetMouseX(), GetMouseY(), tilemap.width);
        enum EntitySpawnSelection sel = (enum EntitySpawnSelection)current_spawn_selection;
        if (tile_idx != last_tile_idx)
        {
            switch (sel)
            {
                case TOGGLE_TILE:
                        tilemap.tiles[tile_idx].solid = !tilemap.tiles[tile_idx].solid;
                        tilemap.tiles[tile_idx].water_level = 0;
                break;
                case SPAWN_CRATE:
                    spawn_crate(scene, tile_idx, false);
                break;
                case SPAWN_METAL_CRATE:
                    spawn_crate(scene, tile_idx, true);
                break;
            }
            last_tile_idx = tile_idx;
        }
    }
    // TODO: Check for right click to change to water, also update above
    else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
        unsigned int tile_idx = get_tile_idx(GetMouseX(), GetMouseY(), tilemap.width);
        if (tile_idx != last_tile_idx)
        {
            if (tilemap.tiles[tile_idx].water_level == 0)
            {
                tilemap.tiles[tile_idx].water_level = MAX_WATER_LEVEL;
            }
            else
            {
                tilemap.tiles[tile_idx].water_level = 0;
            }
            last_tile_idx = tile_idx;
        }
    }
    else
    {
        last_tile_idx = MAX_N_TILES;
    }
}

void level_do_action(Scene_t *scene, ActionType_t action, bool pressed)
{
    CPlayerState_t *p_playerstate;
    sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    {
        switch(action)
        {
            case ACTION_UP:
                p_playerstate->player_dir.y = (pressed)? -1 : 0;
            break;
            case ACTION_DOWN:
                p_playerstate->player_dir.y = (pressed)? 1 : 0;
                p_playerstate->is_crouch |= (pressed)? 0b10 : 0;
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

void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    scene->scene.scene_data = &scene->data;

    init_level_scene_data(&scene->data);
    // insert level scene systems
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &player_bbox_update_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &tile_collision_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &state_transition_update_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
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
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);

    scene->data.tilemap.width = DEFAULT_MAP_WIDTH;
    scene->data.tilemap.height = DEFAULT_MAP_HEIGHT;
    scene->data.tilemap.n_tiles = scene->data.tilemap.width * scene->data.tilemap.height;
    assert(scene->data.tilemap.n_tiles <= MAX_N_TILES);
    scene->data.tilemap.tiles = all_tiles;
    for (size_t i=0; i<MAX_N_TILES;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_init_64(&all_tiles[i].entities_set, 16, 0);
    }
    for (size_t i=0; i<scene->data.tilemap.width; ++i)
    {
        unsigned int tile_idx = (scene->data.tilemap.height - 1) * scene->data.tilemap.width + i;
        all_tiles[tile_idx].solid = true; // for testing
    }

    spawn_player(&scene->scene);
    update_entity_manager(&scene->scene.ent_manager);
}

void free_level_scene(LevelScene_t *scene)
{
    free_scene(&scene->scene);
    for (size_t i=0; i<MAX_N_TILES;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_term_64(&all_tiles[i].entities_set);
    }
    term_level_scene_data(&scene->data);
}

void reload_level_scene(LevelScene_t *scene)
{
}
