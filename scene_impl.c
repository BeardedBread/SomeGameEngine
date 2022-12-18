#include "scene_impl.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define TILE_SIZE 32
static const Vector2 TILE_SZ = {TILE_SIZE, TILE_SIZE};
#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

static const Vector2 GRAVITY = {0, 1500};

static bool find_1D_overlap(const Vector2 l1, const Vector2 l2, float* overlap)
{
   // No Overlap
    if (l1.y < l2.x || l2.y < l1.x) return false;

    if (l1.x >= l2.x && l1.y <= l2.y)
    {
        // Complete Overlap, not sure what to do tbh
        *overlap = l2.y-l2.x + l1.y-l1.x;
    }
    else
    {
        //Partial overlap
        // x is p1, y is p2
        *overlap =  (l2.y >= l1.y)? l2.x - l1.y : l2.y - l1.x;
    }
    if (fabs(*overlap) < 0.01)
    {
        *overlap = 0;
        return false;
    }
    return true;
}

static bool find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2 * const overlap)
{
    // Note that we include one extra pixel for checking
    // This avoid overlapping on the border
    Vector2 l1, l2;
    bool overlap_x, overlap_y;
    l1.x = tl1.x;
    l1.y = tl1.x + sz1.x;
    l2.x = tl2.x;
    l2.y = tl2.x + sz2.x;

    overlap_x = find_1D_overlap(l1, l2, &overlap->x);
    l1.x = tl1.y;
    l1.y = tl1.y + sz1.y;
    l2.x = tl2.y;
    l2.y = tl2.y + sz2.y;
    overlap_y = find_1D_overlap(l1, l2, &overlap->y);

    return overlap_x && overlap_y;
}

static bool check_on_ground(Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid)
{
    unsigned int tile_x1 = pos.x / TILE_SIZE;
    unsigned int tile_x2 = (pos.x + bbox_sz.x - 1) / TILE_SIZE;
    unsigned int tile_y = (pos.y + bbox_sz.y) / TILE_SIZE;

    for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
    {
        if (grid->tiles[tile_y*grid->width + tile_x].solid) return true;
    }
    return false;
}

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    for (size_t i=0; i<tilemap.n_tiles;++i)
    {
        char buffer[6] = {0};
        int x = (i % tilemap.width) * TILE_SIZE;
        int y = (i / tilemap.width) * TILE_SIZE;
        sprintf(buffer, "%u", sc_map_size_64(&tilemap.tiles[i].entities_set));

        if (tilemap.tiles[i].solid)
        {
            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLACK);
            DrawText(buffer, x, y, 10, WHITE);
        }
        else
        {
            DrawText(buffer, x, y, 10, BLACK);
        }
    }

    Entity_t *p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        DrawRectangle(p_ct->position.x, p_ct->position.y, p_bbox->size.x, p_bbox->size.y, RED);
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
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_ent, CJUMP_COMP_T);
        char buffer[64];
        sprintf(buffer, "Pos: %.3f\n %.3f", p_ct->position.x, p_ct->position.y);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 15, 12, BLACK);
        sprintf(buffer, "Jumps: %u", p_cjump->jumps);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 60, 12, BLACK);
    }
}

static void player_movement_input_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;

    // Deal with player acceleration/velocity via inputs first
    float mag = Vector2Length(data->player_dir);
    mag = (mag == 0)? 1 : mag;
    Entity_t * p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        p_ctransform->accel.x = 0;
        p_ctransform->accel.y = 0;
        p_ctransform->accel = Vector2Scale(Vector2Normalize(data->player_dir), 2000);

        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        bool on_ground = check_on_ground(p_ctransform->position, p_bbox->size, &data->tilemap);
        p_ctransform->on_ground = on_ground;
        if (p_cjump->jumped && p_ctransform->velocity.y < 0)
        {
            if (!data->jumped_pressed && !p_cjump->short_hop)
            {
                p_cjump->short_hop = true;
                p_ctransform->velocity.y /= 2;
            }
        }
        if (!on_ground)
        {
            p_ctransform->accel = Vector2Add(p_ctransform->accel, GRAVITY);
        }
        else
        {
            if (data->jumped_pressed && p_cjump->jumps > 0)
            {
                p_ctransform->velocity.y -= p_cjump->jump_speed;
                p_cjump->jumped = true;
                p_cjump->jumps--;
            }
        }

    }
    data->player_dir.x = 0;
    data->player_dir.y = 0;

}

static void movement_update_system(Scene_t* scene)
{
    // Update movement
    float delta_time = 0.017;
    CTransform_t * p_ctransform;
    sc_map_foreach_value(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], p_ctransform)
    {
        p_ctransform->velocity =
            Vector2Add(
                p_ctransform->velocity,
                Vector2Scale(p_ctransform->accel, delta_time)
            );
        p_ctransform->velocity.x *= 0.85;
        p_ctransform->velocity.y *= 0.98;
        float mag = Vector2Length(p_ctransform->velocity);
        p_ctransform->velocity = Vector2Scale(Vector2Normalize(p_ctransform->velocity), (mag > 1000)? 1000:mag);

        if (fabs(p_ctransform->velocity.x) < 1e-3) p_ctransform->velocity.x = 0;
        if (fabs(p_ctransform->velocity.y) < 1e-3) p_ctransform->velocity.y = 0;

        // Store previous position before update
        p_ctransform->prev_position.x = p_ctransform->position.x;
        p_ctransform->prev_position.y = p_ctransform->position.y;
        p_ctransform->position = Vector2Add(
            p_ctransform->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );
    }

}

static void player_ground_air_transition_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    Entity_t *p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        bool on_ground = check_on_ground(p_ctransform->position, p_bbox->size, &data->tilemap);

        // Handle Ground<->Air Transition
        if (!on_ground && p_ctransform->on_ground)
        {
            if (!p_cjump->jumped) p_cjump->jumps--;
        }
        else if (on_ground && !p_ctransform->on_ground)
        {
            p_cjump->jumps = p_cjump->max_jumps;
            p_cjump->jumped = false;
            p_cjump->short_hop = false;
        }
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
        // Extend the check by a little to avoid missing
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;

        for (unsigned int tile_y=tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x=tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                p_tilecoord->tiles[p_tilecoord->n_tiles++] = tile_idx;
                sc_map_put_64(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id, 0);
            }
        }
    }
}

static void player_collision_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_player;

    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CTileCoord_t* p_tilecoord = get_component(&scene->ent_manager, p_player, CTILECOORD_COMP_T);
        // Get the occupied tiles
        // For each tile, loop through the entities and find overlaps
        // exclude self
        // find also previous overlap
        // If there is collision, use previous overlap to determine direction
        // Resolve collision via moving player by the overlap amount
        Vector2 overlap = {0};
        Vector2 prev_overlap = {0};
        for (size_t i=0;i<p_tilecoord->n_tiles;++i)
        {
            unsigned int tile_idx = p_tilecoord->tiles[i];
            Vector2 other;
            if(tilemap.tiles[tile_idx].solid)
            {

                other.x = (tile_idx % tilemap.width) * TILE_SIZE;
                other.y = (tile_idx / tilemap.width) * TILE_SIZE; // Precision loss is intentional
                if (find_AABB_overlap(p_ctransform->position, p_bbox->size, other, TILE_SZ, &overlap))
                {
                    find_AABB_overlap(p_ctransform->prev_position, p_bbox->size, other, TILE_SZ, &prev_overlap);
                    if (fabs(prev_overlap.y) > fabs(prev_overlap.x))
                    {
                        p_ctransform->position.x += overlap.x;
                        p_ctransform->velocity.x = 0;
                    }
                    else if (fabs(prev_overlap.x) > fabs(prev_overlap.y))
                    {
                        p_ctransform->position.y += overlap.y;
                        p_ctransform->velocity.y = 0;
                    }
                    else if (fabs(overlap.x) < fabs(overlap.y))
                    {
                        p_ctransform->position.x += overlap.x;
                        p_ctransform->velocity.x = 0;
                    }
                    else
                    {
                        p_ctransform->position.y += overlap.y;
                        p_ctransform->velocity.y = 0;
                    }
                }
            }
            else
            {
                // TODO: check against the entities of each involved tiles
            }

            // Level boundary collision
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
        // Deal with float precision, by rounding when it is near to an integer enough
        float decimal;
        float fractional = modff(p_ctransform->position.x, &decimal);
        if (fractional > 0.99)
        {
            p_ctransform->position.x = decimal;
            (p_ctransform->position.x > 0) ? p_ctransform->position.x++ : p_ctransform->position.x--;
        }
        else if (fractional < 0.01)
        {
            p_ctransform->position.x = decimal;
        }

        fractional = modff(p_ctransform->position.y, &decimal);
        if (fractional > 0.99)
        {
            p_ctransform->position.y = decimal;
            (p_ctransform->position.y > 0) ? p_ctransform->position.y++ : p_ctransform->position.y--;
        }
        else if (fractional < 0.01)
        {
            p_ctransform->position.y = decimal;
        }
    }

}

static void toggle_block_system(Scene_t *scene)
{
    static unsigned int last_tile_idx = MAX_N_TILES;
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        int mouse_x = GetMouseX() / TILE_SIZE;
        int mouse_y = GetMouseY() / TILE_SIZE;
        unsigned int tile_idx = mouse_y * tilemap.width + mouse_x;
        if (tile_idx != last_tile_idx)
        {
            tilemap.tiles[tile_idx].solid = !tilemap.tiles[tile_idx].solid;
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
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    switch(action)
    {
        case ACTION_UP:
            data->jumped_pressed = pressed;
        break;
        case ACTION_DOWN:
        //    data->player_dir.y = 1;
        break;
        case ACTION_LEFT:
            data->player_dir.x = (pressed)? -1 : 0;
        break;
        case ACTION_RIGHT:
            data->player_dir.x = (pressed)? 1 : 0;
        break;
    }

}

void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    scene->scene.scene_data = &scene->data;
    scene->data.jumped_pressed = false;
    memset(&scene->data.player_dir, 0, sizeof(Vector2));

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
    sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &player_collision_system);
    sc_array_add(&scene->scene.systems, &toggle_block_system);

    // This avoid graphical glitch, not essential
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);

    scene->data.tilemap.width = 32;
    scene->data.tilemap.height = 16;
    assert(32*16 <= MAX_N_TILES);
    scene->data.tilemap.n_tiles = 32*16;
    scene->data.tilemap.tiles = all_tiles;
    for (size_t i=0; i<MAX_N_TILES;i++)
    {
        all_tiles[i].solid = 0;
        sc_map_init_64(&all_tiles[i].entities_set, 8, 0);
    }
    for (size_t i=0; i<32; ++i)
    {
        all_tiles[15*32+i].solid = true; // for testing
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
