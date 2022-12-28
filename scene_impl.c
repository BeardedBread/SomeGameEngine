#include "scene_impl.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

static const Vector2 TILE_SZ = {TILE_SIZE, TILE_SIZE};
#define MAX_N_TILES 4096
static Tile_t all_tiles[MAX_N_TILES] = {0};

static const Vector2 GRAVITY = {0, GRAV_ACCEL};
static const Vector2 UPTHRUST = {0, -GRAV_ACCEL * 0.7};

static inline unsigned int get_tile_idx(int x, int y, unsigned int tilemap_width)
{
    unsigned int tile_x = x / TILE_SIZE;
    unsigned int tile_y = y / TILE_SIZE;
    return tile_y * tilemap_width + tile_x;
}

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
    if (fabs(*overlap) < 0.01) // Use 2 dp precision
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

static bool check_collision_at(Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid, Vector2 point)
{
    Vector2 new_pos = Vector2Add(pos, point);
    unsigned int tile_x1 = (new_pos.x) / TILE_SIZE;
    unsigned int tile_x2 = (new_pos.x + bbox_sz.x - 1) / TILE_SIZE;
    unsigned int tile_y1 = (new_pos.y) / TILE_SIZE;
    unsigned int tile_y2 = (new_pos.y + bbox_sz.y - 1) / TILE_SIZE;

    for(unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
    {
        for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
        {
            if (grid->tiles[tile_y*grid->width + tile_x].solid) return true;
        }
    }
    return false;
}

static inline bool check_on_ground(Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid)
{
    unsigned int tile_x1 = (pos.x) / TILE_SIZE;
    unsigned int tile_x2 = (pos.x + bbox_sz.x - 1) / TILE_SIZE;
    unsigned int tile_y = (pos.y + bbox_sz.y) / TILE_SIZE;

    for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
    {
        if (grid->tiles[tile_y*grid->width + tile_x].solid) return true;
    }
    return false;
}

typedef enum AnchorPoint
{
    AP_TOP_LEFT,
    AP_TOP_CENTER,
    AP_TOP_RIGHT,
    AP_MID_LEFT,
    AP_MID_CENTER,
    AP_MID_RIGHT,
    AP_BOT_LEFT,
    AP_BOT_CENTER,
    AP_BOT_RIGHT,
}AnchorPoint_t;

static Vector2 shift_bbox(Vector2 bbox, Vector2 new_bbox, AnchorPoint_t anchor)
{
    Vector2 p1;
    Vector2 p2;

    Vector2 offset = {0};
    switch (anchor)
    {
        case AP_TOP_LEFT:
            // When resizing bbox, it is implicitly assumed that to be already in topleft
            // due to the coordindate system (+ve towards right and downwards)
            // So do nothing
        return offset;
        case AP_TOP_CENTER:
            p1.x = bbox.x / 2;
            p1.y = 0;
            p2.x = new_bbox.x / 2;
            p2.y = 0;
        break;
        case AP_TOP_RIGHT:
            p1.x = bbox.x;
            p1.y = 0;
            p2.x = new_bbox.x;
            p2.y = 0;
        break;
        case AP_MID_LEFT:
            p1.x = 0;
            p1.y = bbox.y / 2;
            p2.x = 0;
            p2.y = new_bbox.y / 2;
        break;
        case AP_MID_CENTER:
            p1.x = bbox.x / 2;
            p1.y = bbox.y / 2;
            p2.x = new_bbox.x / 2;
            p2.y = new_bbox.y / 2;
        break;
        case AP_MID_RIGHT:
            p1.x = bbox.x;
            p1.y = bbox.y / 2;
            p2.x = new_bbox.x;
            p2.y = new_bbox.y / 2;
        break;
        case AP_BOT_LEFT:
            p1.x = 0;
            p1.y = bbox.y;
            p2.x = 0;
            p2.y = new_bbox.y;
        break;
        case AP_BOT_CENTER:
            p1.x = bbox.x / 2;
            p1.y = bbox.y;
            p2.x = new_bbox.x / 2;
            p2.y = new_bbox.y;
        break;
        case AP_BOT_RIGHT:
            p1.x = bbox.x;
            p1.y = bbox.y;
            p2.x = new_bbox.x;
            p2.y = new_bbox.y;
        break;
    }
    offset.x = p1.x - p2.x;
    offset.y = p1.y - p2.y;
    return offset;
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
            // Draw water tile
            if (tilemap.tiles[i].water_level > 0)
            {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, BLUE);
            }
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

    // For DEBUG
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_ent, CJUMP_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
        char buffer[64];
        sprintf(buffer, "Pos: %.3f\n %.3f", p_ct->position.x, p_ct->position.y);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 15, 12, BLACK);
        sprintf(buffer, "Jumps: %u", p_cjump->jumps);
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 60, 12, BLACK);
        sprintf(buffer, "Crouch: %s", p_pstate->is_crouch? "YES":"NO");
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 90, 12, BLACK);
        sprintf(buffer, "Water: %s", p_pstate->in_water? "YES":"NO");
        DrawText(buffer, tilemap.width * TILE_SIZE + 1, 120, 12, BLACK);
    }
}

static void player_movement_input_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    // Deal with player acceleration/velocity via inputs first
    float mag = Vector2Length(data->player_dir);
    mag = (mag == 0)? 1 : mag;
    Entity_t * p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);
        p_pstate->is_crouch = data->crouch_pressed;
        p_ctransform->accel.x = 0;
        p_ctransform->accel.y = 0;
        if (!p_pstate->in_water)
        {
            data->player_dir.y = 0;
            p_ctransform->accel = Vector2Scale(Vector2Normalize(data->player_dir), MOVE_ACCEL/1.2);
        }
        else
        {
            p_ctransform->accel = Vector2Scale(Vector2Normalize(data->player_dir), MOVE_ACCEL);
        }

        // Short Hop
        if (p_cjump->jumped && p_ctransform->velocity.y < 0)
        {
            if (!data->jumped_pressed && !p_cjump->short_hop)
            {
                p_cjump->short_hop = true;
                p_ctransform->velocity.y /= 2;
            }
        }


        if (!p_ctransform->on_ground)
        {
            if (p_pstate->in_water)
            {
                p_ctransform->accel = Vector2Add(p_ctransform->accel, UPTHRUST);
            }
            p_ctransform->accel = Vector2Add(p_ctransform->accel, GRAVITY);
        }
        else
        {
            // All these actions can only be done on ground

            // Check if possible to jump when jump is pressed
            if (data->jumped_pressed && p_cjump->jumps > 0)
            {
                bool jump_valid = true;

                // Check Jump from crouch
                if (p_pstate->is_crouch)
                {
                    Vector2 test_pos = p_ctransform->position;
                    Vector2 test_bbox = {PLAYER_WIDTH, PLAYER_HEIGHT};
                    Vector2 top = {0, -1};
                    test_pos.x += PLAYER_C_XOFFSET;
                    test_pos.y -= PLAYER_C_YOFFSET;
                    jump_valid = !check_collision_at(test_pos, test_bbox, &tilemap, top);
                }

                // Jump okay
                if (jump_valid)
                {
                    p_ctransform->velocity.y -= p_cjump->jump_speed;
                    p_cjump->jumped = true;
                }
            }

        }

        // Friction
        if (p_pstate->in_water)
        {
            // Apply water friction
            // Consistent in all direction
            p_ctransform->accel = Vector2Add(
                p_ctransform->accel,
                Vector2Scale(p_ctransform->velocity, -5.5)
            );
        }
        else
        {
            // For game feel, y is set to air resistance only
            // x is set to ground resistance (even in air)
            // If not, then player is can go faster by bunny hopping
            // which is fun but not quite beneficial here
            p_ctransform->accel.x += p_ctransform->velocity.x * -3.3;
            p_ctransform->accel.y += p_ctransform->velocity.y * -1;
        }
    }
    data->player_dir.x = 0;
    data->player_dir.y = 0;

}

static void player_bbox_update_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);
        if (p_ctransform->on_ground)
        {
            AnchorPoint_t anchor = AP_BOT_CENTER;
            int dx[3] = {1, 0, 2};
            for (size_t i=0; i<3; i++)
            {
                unsigned int tile_idx = get_tile_idx
                (
                    p_ctransform->position.x + p_bbox->half_size.x * dx[i],
                    p_ctransform->position.y + p_bbox->size.y,
                    tilemap.width
                );
                if (tilemap.tiles[tile_idx].solid)
                {
                    switch(i)
                    {
                        case 1: anchor = AP_BOT_LEFT;break;
                        case 2: anchor = AP_BOT_RIGHT;break;
                    }
                    break;
                }
            }
            Vector2 new_bbox;
            if (p_pstate->is_crouch)
            {
                new_bbox.x = PLAYER_C_WIDTH;
                new_bbox.y = PLAYER_C_HEIGHT;
            }
            else
            {
                new_bbox.x = PLAYER_WIDTH;
                new_bbox.y = PLAYER_HEIGHT;
            }
            Vector2 offset = shift_bbox(p_bbox->size, new_bbox, anchor);
            set_bbox(p_bbox, new_bbox.x, new_bbox.y);
            p_ctransform->position = Vector2Add(p_ctransform->position, offset);
        }
        else
        {
            if (p_pstate->in_water)
            {
                Vector2 new_bbox = {PLAYER_C_WIDTH, PLAYER_C_HEIGHT};
                Vector2 offset = shift_bbox(p_bbox->size, new_bbox, AP_MID_CENTER);
                set_bbox(p_bbox, PLAYER_C_WIDTH, PLAYER_C_HEIGHT);
                p_ctransform->position = Vector2Add(p_ctransform->position, offset);
            }
            else
            {
                Vector2 new_bbox = {PLAYER_WIDTH, PLAYER_HEIGHT};
                Vector2 offset = shift_bbox(p_bbox->size, new_bbox, AP_MID_CENTER);
                set_bbox(p_bbox, PLAYER_WIDTH, PLAYER_HEIGHT);
                p_ctransform->position = Vector2Add(p_ctransform->position, offset);
            }
        }
    }
}

static void movement_update_system(Scene_t* scene)
{
    // Update movement
    float delta_time = 0.017; // TODO: Will need to think about delta time handling
    CTransform_t * p_ctransform;
    sc_map_foreach_value(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], p_ctransform)
    {
        p_ctransform->velocity =
            Vector2Add(
                p_ctransform->velocity,
                Vector2Scale(p_ctransform->accel, delta_time)
            );

        float mag = Vector2Length(p_ctransform->velocity);
        p_ctransform->velocity = Vector2Scale(Vector2Normalize(p_ctransform->velocity), (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag);

        // 3 dp precision
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
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);

        // State checks
        bool on_ground = check_on_ground(p_ctransform->position, p_bbox->size, &data->tilemap);
        bool in_water = false;
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;
        for (unsigned int tile_y=tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x=tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * data->tilemap.width + tile_x;
                in_water |= data->tilemap.tiles[tile_idx].water_level > 0;
            }
        }
                
        // Handle Ground<->Air Transition
        if (!on_ground && p_ctransform->on_ground)
        {
            p_cjump->jumps--;
        }
        else if ((on_ground && !p_ctransform->on_ground) || in_water)
        {
            p_cjump->jumps = p_cjump->max_jumps;
            p_cjump->jumped = false;
            p_cjump->short_hop = false;
        }

        // TODO: Handle in water <-> out of water transition

        p_ctransform->on_ground = on_ground;
        p_pstate->in_water = in_water;
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
        // Get the occupied tiles
        // For each tile, loop through the entities and find overlaps
        // exclude self
        // find also previous overlap
        // If there is collision, use previous overlap to determine direction
        // Resolve collision via moving player by the overlap amount
        Vector2 overlap = {0};
        Vector2 prev_overlap = {0};
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;
        for (unsigned int tile_y=tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x=tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
        //for (size_t i=0;i<p_tilecoord->n_tiles;++i)
        //{
            //unsigned int tile_idx = p_tilecoord->tiles[i];
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
        }
        // Deal with float precision, by rounding when it is near to an integer enough by 2 dp
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
        unsigned int tile_idx = get_tile_idx(GetMouseX(), GetMouseY(), tilemap.width);
        if (tile_idx != last_tile_idx)
        {
            tilemap.tiles[tile_idx].solid = !tilemap.tiles[tile_idx].solid;
            tilemap.tiles[tile_idx].water_level = 0;
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
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    switch(action)
    {
        case ACTION_UP:
            data->player_dir.y = (pressed)? -1 : 0;
            data->jumped_pressed = pressed;
        break;
        case ACTION_DOWN:
            data->player_dir.y = (pressed)? 1 : 0;
            data->crouch_pressed = pressed;
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
    scene->data.crouch_pressed = false;
    memset(&scene->data.player_dir, 0, sizeof(Vector2));

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &player_bbox_update_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    //sc_array_add(&scene->scene.systems, &update_tilemap_system);
    sc_array_add(&scene->scene.systems, &player_collision_system);
    sc_array_add(&scene->scene.systems, &player_ground_air_transition_system);
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
