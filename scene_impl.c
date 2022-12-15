#include "scene_impl.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define TILE_SIZE 32
#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        CTileCoord_t* p_tilecoord = get_component(&scene->ent_manager, p_ent, CTILECOORD_COMP_T);

        for (size_t i=0;i<p_tilecoord->n_tiles;++i)
        {
            // Use previously store tile position
            // Clear from those positions
            int x = (p_tilecoord->tiles[i] % tilemap.width) * TILE_SIZE;
            int y = (p_tilecoord->tiles[i] / tilemap.width) * TILE_SIZE;
            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, GREEN);
        }

        DrawRectangle(p_ct->position.x, p_ct->position.y, p_bbox->size.x, p_bbox->size.y, RED);

    }

    //Draw tile grid
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
    for (size_t i=0; i<tilemap.n_tiles;++i)
    {
        char buffer[6] = {0};
        int x = (i % tilemap.width) * TILE_SIZE;
        int y = (i / tilemap.width) * TILE_SIZE;
        sprintf(buffer, "%u", sc_map_size_64(&tilemap.tiles[i].entities_set));

        DrawText(buffer, x, y, 10, BLACK);
    }
}

static void movement_update_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;

    // Deal with player acceleration/velocity via inputs first
    float mag = Vector2Length(data->player_dir);
    mag = (mag == 0)? 1 : mag;
    Entity_t * p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        p_ctransform->accel.x = data->player_dir.x * 200 * 1.0 / mag;
        p_ctransform->accel.y = data->player_dir.y * 200 * 1.0 / mag;
    }
    data->player_dir.x = 0;
    data->player_dir.y = 0;

    // Update movement
    float delta_time = GetFrameTime();
    CTransform_t * p_ctransform;
    sc_map_foreach_value(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], p_ctransform)
    {
        p_ctransform->velocity = Vector2Add(
            p_ctransform->velocity,
            Vector2Scale(p_ctransform->accel, delta_time)
        );

        // Store previous position before update
        p_ctransform->prev_position.x = p_ctransform->position.x;
        p_ctransform->prev_position.y = p_ctransform->position.y;
        p_ctransform->position = Vector2Add(
            p_ctransform->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );

    }

}

static void update_tilemap_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CTileCoord_t * p_tilecoord = get_component(&scene->ent_manager, p_ent, CTILECOORD_COMP_T);
        if (p_tilecoord == NULL) continue;
        CTransform_t * p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        if (p_ctransform == NULL) continue;
        CBBox_t * p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        if (p_bbox == NULL) continue;

        // Update tilemap position
        for (size_t i=0;i<p_tilecoord->n_tiles;++i)
        {
            // Use previously store tile position
            // Clear from those positions
            unsigned int tile_idx = p_tilecoord->tiles[i];
            sc_map_del_64(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id);
        }
        p_tilecoord->n_tiles = 0;

        // Compute new occupied tile positions and add
        unsigned int tile_x = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y = (p_ctransform->position.y) / TILE_SIZE;
        p_tilecoord->tiles[0] = tile_y * tilemap.width + tile_x;
        p_tilecoord->n_tiles++;

        bool check_x = (tile_x + 1) < tilemap.width;
        bool check_y = (tile_y + 1) < tilemap.height;
        if (check_x)
        {
            p_tilecoord->tiles[p_tilecoord->n_tiles++] = p_tilecoord->tiles[0] + 1;
        }
        if (check_y)
        {
            p_tilecoord->tiles[p_tilecoord->n_tiles++] = p_tilecoord->tiles[0] + tilemap.width;
        }
        if (check_x && check_y)
        {
            p_tilecoord->tiles[p_tilecoord->n_tiles++] = p_tilecoord->tiles[0] + tilemap.width + 1;
        }

        for (size_t i=0;i<p_tilecoord->n_tiles;++i)
        {
            unsigned int tile_idx = p_tilecoord->tiles[i];
            sc_map_put_64(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id, 0);
        }
    }
}

static void player_collision_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_player;
    size_t tiles_to_check[8] = {0};
    size_t n_tiles = 0;

    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        // Get the occupied tiles
        // For each tile, loop through the entities and find overlaps
        // exclude self
        // find also previous overlap
        // If there is collision, use previous overlap to determine direction
        // Resolve collision via moving player by the overlap amount
    }

}

static void screen_bounce_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;
    Entity_t* p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        if (p_bbox == NULL || p_ctransform == NULL) continue;

        unsigned int level_width = tilemap.width * TILE_SIZE;
        if(p_ctransform->position.x < 0 || p_ctransform->position.x + p_bbox->size.x > level_width)
        {
            p_ctransform->position.x = (p_ctransform->position.x < 0) ? 0 : p_ctransform->position.x;
            p_ctransform->position.x = (p_ctransform->position.x + p_bbox->size.x > level_width) ? level_width - p_bbox->size.x : p_ctransform->position.x;
            p_ctransform->velocity.x *= -1;
        }
        
        unsigned int level_height = tilemap.height * TILE_SIZE;
        if(p_ctransform->position.y < 0 || p_ctransform->position.y + p_bbox->size.y > level_height)
        {
            p_ctransform->position.y = (p_ctransform->position.y < 0) ? 0 : p_ctransform->position.y;
            p_ctransform->position.y = (p_ctransform->position.y + p_bbox->size.y > level_height) ? level_height - p_bbox->size.y : p_ctransform->position.y;
            p_ctransform->velocity.y *= -1;
        }
    }

}

void level_do_action(Scene_t *scene, ActionType_t action, bool pressed)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    if (pressed)
    {
        switch(action)
        {
            case ACTION_UP:
                data->player_dir.y = -1;
            break;
            case ACTION_DOWN:
                data->player_dir.y = 1;
            break;
            case ACTION_LEFT:
                data->player_dir.x = -1;
            break;
            case ACTION_RIGHT:
                data->player_dir.x = 1;
            break;
        }
    }

}

void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    scene->scene.scene_data = &scene->data;
    memset(&scene->data.player_dir, 0, sizeof(Vector2));

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &screen_bounce_system);
    sc_array_add(&scene->scene.systems, &update_tilemap_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);

    scene->data.tilemap.width = 16;
    scene->data.tilemap.height = 16;
    scene->data.tilemap.n_tiles = 16*16;
    scene->data.tilemap.tiles = all_tiles;
    for (size_t i=0; i<MAX_N_TILES;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_init_64(&all_tiles[i].entities_set, 8, 0);
    }
}

void free_level_scene(LevelScene_t *scene)
{
    free_scene(&scene->scene);
    for (size_t i=0; i<MAX_N_TILES;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_term_64(&all_tiles[i].entities_set);
    }
}

void reload_level_scene(LevelScene_t *scene)
{
}
