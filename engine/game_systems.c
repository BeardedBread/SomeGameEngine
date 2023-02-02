#include "game_systems.h"
#include "AABB.h"
#include "constants.h"

static const Vector2 TILE_SZ = {TILE_SIZE, TILE_SIZE};
static const Vector2 GRAVITY = {0, GRAV_ACCEL};
static const Vector2 UPTHRUST = {0, -GRAV_ACCEL * 1.1};
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

static inline unsigned int get_tile_idx(int x, int y, unsigned int tilemap_width)
{
    unsigned int tile_x = x / TILE_SIZE;
    unsigned int tile_y = y / TILE_SIZE;
    return tile_y * tilemap_width + tile_x;
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

static inline bool check_on_ground(unsigned int ent_idx, Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid, Scene_t* scene)
{
    pos.y += 1;
    unsigned int tile_x1 = (pos.x) / TILE_SIZE;
    unsigned int tile_x2 = (pos.x + bbox_sz.x - 1) / TILE_SIZE;
    unsigned int tile_y = (pos.y + bbox_sz.y - 1) / TILE_SIZE;

    for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
    {
        unsigned int tile_idx = tile_y*grid->width + tile_x;
        if (grid->tiles[tile_idx].solid) return true;

        unsigned int other_ent_idx;
        sc_map_foreach_key(&grid->tiles[tile_idx].entities_set, other_ent_idx)
        {
            Vector2 overlap;
            if (other_ent_idx == ent_idx) continue;
            Entity_t *p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
            CBBox_t *p_other_bbox = get_component(&scene->ent_manager, p_other_ent, CBBOX_COMP_T);
            if (p_other_bbox == NULL || !p_other_bbox->solid || p_other_bbox->fragile) continue;

            CTransform_t *p_other_ct = get_component(&scene->ent_manager, p_other_ent, CTRANSFORM_COMP_T);
            if (find_AABB_overlap(pos, bbox_sz, p_other_ct->position, p_other_bbox->size, &overlap)) return true;
        }
    }
    return false;
}
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

void player_movement_input_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    // Deal with player acceleration/velocity via inputs first
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        float mag = Vector2Length(p_pstate->player_dir);
        mag = (mag == 0)? 1 : mag;
        Entity_t * p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);

        bool in_water = (p_mstate->water_state & 1);
        if (!in_water)
        {
            p_pstate->player_dir.y = 0;
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL/1.2);
        }
        else
        {
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);
        }

        // Short Hop
        if (p_cjump->jumped && p_ctransform->velocity.y < 0)
        {
            if (!p_pstate->jump_pressed && !p_cjump->short_hop)
            {
                p_cjump->short_hop = true;
                p_ctransform->velocity.y /= 2;
            }
        }


        //else
        {
            bool hit = false;
            unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
            unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE;
            unsigned int tile_y = (p_ctransform->position.y) / TILE_SIZE;
            if (p_pstate->is_crouch == 0b01 && tile_y > 0)  tile_y--;

            for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                hit |= tilemap.tiles[tile_y * tilemap.width + tile_x].solid;
            }
            if (hit)
            {
                p_pstate->is_crouch |= 0b10;
            }
            if (!(p_mstate->ground_state & 1)) p_pstate->is_crouch &= 0b01;
            p_pstate->is_crouch >>= 1;

            if (p_cjump->cooldown_timer > 0) p_cjump->cooldown_timer--;
            // Jumps is possible as long as you have a jump

            // Check if possible to jump when jump is pressed
            if (p_pstate->jump_pressed && p_cjump->jumps > 0 && p_cjump->cooldown_timer == 0)
            {
                bool jump_valid = true;

                // Check Jump from crouch
                if(p_pstate->is_crouch & 1)
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
                    if (!in_water)
                    {
                        p_ctransform->velocity.y -= p_cjump->jump_speed;
                    }
                    else
                    {
                        p_ctransform->velocity.y -= p_cjump->jump_speed / 1.75;
                    }

                    p_cjump->jumped = true;
                    p_cjump->cooldown_timer = 15;
                }
            }

        }

        p_pstate->player_dir.x = 0;
        p_pstate->player_dir.y = 0;
    }
}

void player_bbox_update_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t *p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);
        if (p_mstate->ground_state & 1)
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
            if(p_pstate->is_crouch & 1)
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
            if (p_mstate->water_state & 1)
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

static struct sc_map_32 collision_events;
void tile_collision_system(Scene_t *scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};

    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;


    unsigned int ent_idx;
    CBBox_t* p_bbox;
    //sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    sc_map_foreach(&scene->ent_manager.component_map[CBBOX_COMP_T], ent_idx, p_bbox)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
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
                    unsigned int other_ent_idx;
                    memset(checked_entities, 0, sizeof(checked_entities));
                    sc_map_foreach_key(&tilemap.tiles[tile_idx].entities_set, other_ent_idx)
                    {
                        if (other_ent_idx == ent_idx) continue;
                        if (checked_entities[other_ent_idx]) continue;

                        checked_entities[other_ent_idx] = true;
                        Entity_t *p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
                        if (p_other_ent->m_tag < p_ent->m_tag) continue; // To only allow one way collision check
                        CBBox_t *p_other_bbox = get_component(&scene->ent_manager, p_other_ent, CBBOX_COMP_T);
                        if (p_other_bbox == NULL) continue;

                        CTransform_t *p_other_ct = get_component(&scene->ent_manager, p_other_ent, CTRANSFORM_COMP_T);
                        if (find_AABB_overlap(p_ctransform->position, p_bbox->size, p_other_ct->position, p_other_bbox->size, &overlap))
                        {
                            find_AABB_overlap(p_ctransform->prev_position, p_bbox->size, p_other_ct->prev_position, p_other_bbox->size, &prev_overlap);
                            // TODO: Store collision event here
                            // Check collision on x or y axis
                            // Encode key and value, -ve is left and up, +ve is right and down
                            // Move only if solid
                            uint8_t dir_to_move = 0; // x-axis, y-axis
                            uint32_t collision_key = ((ent_idx << 16) | other_ent_idx);
                            uint32_t collision_value = 0;
                            if (fabs(prev_overlap.y) > fabs(prev_overlap.x))
                            {
                                collision_value = (((overlap.x > 0?1:0)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
                                dir_to_move = 0;
                            }
                            else if (fabs(prev_overlap.x) > fabs(prev_overlap.y))
                            {
                                collision_value = (((overlap.y > 0?3:2)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
                                dir_to_move = 1;
                            }
                            else if (fabs(overlap.x) < fabs(overlap.y))
                            {
                                collision_value = (((overlap.x > 0?1:0)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
                                dir_to_move = 0;
                            }
                            else
                            {
                                collision_value = (((overlap.y > 0?3:2)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
                                dir_to_move = 1;
                            }
                            sc_map_put_32(&collision_events, collision_key, collision_value);

                            if (!p_other_bbox->solid) continue;
                            if (dir_to_move == 1)
                            {
                                p_ctransform->position.y += overlap.y;
                                p_ctransform->velocity.y = 0;
                            }
                            else
                            {
                                p_ctransform->position.x += overlap.x;
                                p_ctransform->velocity.x = 0;
                            }
                        }
                    }
                }
            }
        }

        // TODO: Resolve all collision events
        uint32_t collision_key, collision_value;
        sc_map_foreach(&collision_events, collision_key, collision_value)
        {
            ent_idx = (collision_key >> 16);
            uint other_ent_idx = (collision_key & 0xFFFF);
            Entity_t *p_ent = get_entity(&scene->ent_manager, ent_idx);
            Entity_t *p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
            if (!p_ent->m_alive || !p_other_ent->m_alive) continue;
            if (p_ent->m_tag == PLAYER_ENT_TAG && p_other_ent->m_tag == CRATES_ENT_TAG)
            {
                CTransform_t *p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
                CBBox_t * p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
                CPlayerState_t * p_pstate = get_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
                CTransform_t *p_other_ct = get_component(&scene->ent_manager, p_other_ent, CTRANSFORM_COMP_T);
                CBBox_t *p_other_bbox = get_component(&scene->ent_manager, p_other_ent, CBBOX_COMP_T);
                if (
                    // TODO: Check Material of the crates
                    p_ctransform->position.y + p_bbox->size.y <= p_other_ct->position.y
                )
                {
                    p_ctransform->velocity.y = -400;
                    if (p_pstate->jump_pressed)
                    {
                        p_ctransform->velocity.y = -600;
                        CJump_t * p_cjump = get_component(&scene->ent_manager, p_ent, CJUMP_COMP_T);
                        p_cjump->short_hop = false;
                        p_cjump->jumped = true;
                    }
                }
                if (p_other_bbox->fragile)
                {
                    CTileCoord_t *p_tilecoord = get_component(&scene->ent_manager, p_other_ent, CTILECOORD_COMP_T);
                    for (size_t i=0;i<p_tilecoord->n_tiles;++i)
                    {
                        // Use previously store tile position
                        // Clear from those positions
                        unsigned int tile_idx = p_tilecoord->tiles[i];
                        sc_map_del_64(&(tilemap.tiles[tile_idx].entities_set), other_ent_idx);
                    }
                    remove_entity(&scene->ent_manager, other_ent_idx);
                }
            }
        }
        sc_map_clear_32(&collision_events);

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

void global_external_forces_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t * p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t * p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        if (!(p_mstate->ground_state & 1))
        {
            // Only apply upthrust if center is in water
            // If need more accuracy, need to find area of overlap
            if (p_mstate->water_state & 1)
            {
                unsigned int tile_idx = get_tile_idx(
                    p_ctransform->position.x + p_bbox->half_size.x,
                    p_ctransform->position.y + p_bbox->half_size.y,
                    data->tilemap.width
                );

                if (data->tilemap.tiles[tile_idx].water_level > 0)
                {
                    p_ctransform->accel = Vector2Add(p_ctransform->accel, UPTHRUST);
                }
            }
            p_ctransform->accel = Vector2Add(p_ctransform->accel, GRAVITY);
        }
        // Friction
        if (p_mstate->water_state & 1)
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
}

void movement_update_system(Scene_t* scene)
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
        p_ctransform->accel.x = 0;
        p_ctransform->accel.y = 0;
    }
}

void player_ground_air_transition_system(Scene_t* scene)
{
    Entity_t *p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);

        // Handle Ground<->Air Transition
        bool in_water = (p_mstate->water_state & 1);
        // Landing or in water
        if (p_mstate->ground_state == 0b01 || in_water)
        {
            // Recover jumps
            p_cjump->jumps = p_cjump->max_jumps;
            p_cjump->jumped = false;
            p_cjump->short_hop = false;
            if (!in_water)
                p_cjump->cooldown_timer = 0;
        }
        else if (p_mstate->water_state == 0b10 || p_mstate->ground_state == 0b10)
        {
            p_cjump->jumps -= (p_cjump->jumps > 0)? 1:0; 
            if (p_mstate->ground_state & 1)
            {
                p_cjump->jumps++;
            }
        }
    }
}

void state_transition_update_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    //Entity_t* p_ent;

    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t *p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t *p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        if (p_ctransform == NULL || p_bbox == NULL) continue;

        bool on_ground = check_on_ground(ent_idx, p_ctransform->position, p_bbox->size, &data->tilemap, scene);
        bool in_water = false;
        if (!(p_mstate->water_state & 1))
        {
            unsigned int tile_idx = get_tile_idx(
                p_ctransform->position.x + p_bbox->half_size.x,
                p_ctransform->position.y + p_bbox->half_size.y,
                data->tilemap.width
            );

            in_water =  (data->tilemap.tiles[tile_idx].water_level > 0);
        }
        else
        {
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
        }
        p_mstate->ground_state <<= 1;
        p_mstate->ground_state |= on_ground? 1:0;
        p_mstate->ground_state &= 3;
        p_mstate->water_state <<= 1;
        p_mstate->water_state |= in_water? 1:0;
        p_mstate->water_state &= 3;
    }
}

void update_tilemap_system(Scene_t *scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    CTileCoord_t *p_tilecoord;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTILECOORD_COMP_T], ent_idx, p_tilecoord)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
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

void init_collision_system(void)
{
    sc_map_init_32(&collision_events, 128, 0);
}

void term_collision_system(void)
{
    sc_map_term_32(&collision_events);
}
