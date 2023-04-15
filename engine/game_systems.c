#include "game_systems.h"
#include "AABB.h"
#include "constants.h"
#include "assets_maps.h"
#include <stdio.h>

static const Vector2 TILE_SZ = {TILE_SIZE, TILE_SIZE};
static const Vector2 GRAVITY = {0, GRAV_ACCEL};
static const Vector2 UPTHRUST = {0, -GRAV_ACCEL * 1.1};
typedef enum AnchorPoint {
    AP_TOP_LEFT,
    AP_TOP_CENTER,
    AP_TOP_RIGHT,
    AP_MID_LEFT,
    AP_MID_CENTER,
    AP_MID_RIGHT,
    AP_BOT_LEFT,
    AP_BOT_CENTER,
    AP_BOT_RIGHT,
} AnchorPoint_t;

static inline unsigned int get_tile_idx(int x, int y, unsigned int tilemap_width)
{
    unsigned int tile_x = x / TILE_SIZE;
    unsigned int tile_y = y / TILE_SIZE;
    return tile_y * tilemap_width + tile_x;
}

typedef struct TileArea {
    unsigned int tile_x1;
    unsigned int tile_y1;
    unsigned int tile_x2;
    unsigned int tile_y2;
} TileArea_t;

typedef struct CollideEntity {
    unsigned int idx;
    Rectangle bbox;
    TileArea_t area;
} CollideEntity_t;

// ------------------------- Collision functions ------------------------------------
static bool check_collision(const CollideEntity_t* ent, TileGrid_t* grid, EntityManager_t* p_manager)
{
    for(unsigned int tile_y = ent->area.tile_y1; tile_y <= ent->area.tile_y2; tile_y++)
    {
        for(unsigned int tile_x = ent->area.tile_x1; tile_x <= ent->area.tile_x2; tile_x++)
        {
            unsigned int tile_idx = tile_y*grid->width + tile_x;
            if (grid->tiles[tile_idx].solid) return true;
            unsigned int ent_idx;
            sc_map_foreach_value(&grid->tiles[tile_idx].entities_set, ent_idx)
            {
                if (ent->idx == ent_idx) continue;
                Entity_t * p_ent = get_entity(p_manager, ent_idx);
                CTransform_t *p_ctransform = get_component(p_manager, p_ent, CTRANSFORM_COMP_T);
                CBBox_t *p_bbox = get_component(p_manager, p_ent, CBBOX_COMP_T);
                Vector2 overlap;
                if (p_bbox == NULL || p_ctransform == NULL) continue;
                //if (p_bbox->solid && !p_bbox->fragile)
                if (p_bbox->solid)
                {
                    if (
                        find_AABB_overlap(
                            (Vector2){ent->bbox.x, ent->bbox.y},
                            (Vector2){ent->bbox.width, ent->bbox.height},
                            p_ctransform->position, p_bbox->size, &overlap
                        )
                    )
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

static bool check_collision_at(
    unsigned int ent_idx, Vector2 pos, Vector2 bbox_sz,
    TileGrid_t* grid, Vector2 point, EntityManager_t* p_manager
)
{
    Vector2 new_pos = Vector2Add(pos, point);
    CollideEntity_t ent = {
        .idx = ent_idx,
        .bbox = (Rectangle){new_pos.x, new_pos.y, bbox_sz.x, bbox_sz.y},
        .area = (TileArea_t){
            .tile_x1 = (new_pos.x) / TILE_SIZE,
            .tile_y1 = (new_pos.y) / TILE_SIZE,
            .tile_x2 = (new_pos.x + bbox_sz.x - 1) / TILE_SIZE,
            .tile_y2 = (new_pos.y + bbox_sz.y - 1) / TILE_SIZE
        }
    };
    
    return check_collision(&ent, grid, p_manager);
}

static inline bool check_on_ground(
    unsigned int ent_idx, Vector2 pos, Vector2 bbox_sz,
    TileGrid_t* grid, EntityManager_t* p_manager
)
{
    return check_collision_at(ent_idx, pos, bbox_sz, grid, (Vector2){0, 1}, p_manager);
}

static bool check_collision_and_move(
    EntityManager_t* p_manager, TileGrid_t* tilemap,
    unsigned int ent_idx, CTransform_t* p_ct, Vector2 sz,
    Vector2 other_pos, Vector2 other_sz, bool other_solid,
    uint32_t* collision_value
)
{
    Vector2 overlap = {0,0};
    Vector2 prev_overlap = {0,0};
    if (find_AABB_overlap(p_ct->position, sz, other_pos, other_sz, &overlap))
    {
        // If there is collision, use previous overlap to determine direction
        find_AABB_overlap(p_ct->prev_position, sz, other_pos, other_sz, &prev_overlap);

        // Store collision event here
        // Check collision on x or y axis
        // Encode key and value, -ve is left and up, +ve is right and down
        Vector2 offset = {0, 0};
        if (fabs(prev_overlap.y) > fabs(prev_overlap.x))
        {
            offset.x = overlap.x;
            *collision_value = (((overlap.x > 0?1:0)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
        }
        else if (fabs(prev_overlap.x) > fabs(prev_overlap.y))
        {
            offset.y = overlap.y;
            *collision_value = (((overlap.y > 0?3:2)<< 14) | ( (uint16_t)(fabs(overlap.y)) ));
        }
        else if (fabs(overlap.x) < fabs(overlap.y))
        {
            offset.x = overlap.x;
            *collision_value = (((overlap.x > 0?1:0)<< 14) | ( (uint16_t)(fabs(overlap.x)) ));
        }
        else
        {
            offset.y = overlap.y;
            *collision_value = (((overlap.y > 0?3:2)<< 14) | ( (uint16_t)(fabs(overlap.y)) ));
        }

        // Resolve collision via moving player by the overlap amount only if other is solid
        // also check for empty to prevent creating new collision. Not fool-proof, but good enough
        //if (other_solid && !check_collision_at(ent_idx, p_ct->position, sz, tilemap, offset, p_manager))
        if (other_solid)
        {
            p_ct->position = Vector2Add(p_ct->position, offset);
        }
        return true;
    }
    return false;
}

static uint8_t check_bbox_edges(
    EntityManager_t* p_manager, TileGrid_t* tilemap,
    unsigned int ent_idx, Vector2 pos, Vector2 bbox
)
{
    uint8_t detected = 0;
    CollideEntity_t ent = 
    {
        .idx = ent_idx,
        .bbox = (Rectangle){pos.x - 1, pos.y, bbox.x, bbox.y},
        .area = (TileArea_t){
            .tile_x1 = (pos.x - 1) / TILE_SIZE,
            .tile_y1 = (pos.y) / TILE_SIZE,
            .tile_x2 = (pos.x - 1) / TILE_SIZE,
            .tile_y2 = (pos.y + bbox.y - 1) / TILE_SIZE,
        }
    };
    // Left
    detected |= (check_collision(&ent, tilemap, p_manager) ? 1 : 0) << 3;

    //Right
    ent.bbox.x += 2; // 2 to account for the previous subtraction
    ent.area.tile_x1 = (pos.x + bbox.x) / TILE_SIZE;
    ent.area.tile_x2 = ent.area.tile_x1;
    detected |= (check_collision(&ent, tilemap, p_manager) ? 1 : 0) << 2;

    // Up
    ent.bbox.x -= 2;
    ent.bbox.y--;
    ent.area.tile_x1 = (pos.x) / TILE_SIZE,
    ent.area.tile_x2 = (pos.x + bbox.x - 1) / TILE_SIZE,
    ent.area.tile_y1 = (pos.y - 1) / TILE_SIZE,
    ent.area.tile_y2 = ent.area.tile_y1;
    detected |= (check_collision(&ent, tilemap, p_manager) ? 1 : 0) << 1;

    // Down
    ent.bbox.y += 2;
    ent.area.tile_y1 = (pos.y + bbox.y) / TILE_SIZE,
    ent.area.tile_y2 = ent.area.tile_y1;
    detected |= (check_collision(&ent, tilemap, p_manager) ? 1 : 0);
    return detected;
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
    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    // Deal with player acceleration/velocity via inputs first
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);

        bool in_water = (p_mstate->water_state & 1);
        if (!in_water)
        {
            p_pstate->player_dir.y = 0;
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);
        }
        else
        {
            // Although this can be achieved via higher friction, i'll explain away as the player is not
            // good with swimming, resulting in lower movement acceleration
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL/1.2);
        }

        if (p_pstate->is_crouch & 1)
        {
            p_ctransform->accel = Vector2Scale(p_ctransform->accel, 0.5);
        }
        // Short Hop
        // Jumped check is needed to make sure it is applied on jumps, not generally
        // One issue caused is lower velocity in water
        if (p_cjump->jumped)
        {
            if (!p_pstate->jump_pressed)
            {
                if (!p_cjump->short_hop && p_ctransform->velocity.y < 0)
                {
                    p_cjump->short_hop = true;
                    p_ctransform->velocity.y /= 2;
                }
            }
        }

        bool hit = false;
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE;
        unsigned int tile_y = (p_ctransform->position.y) / TILE_SIZE;
        if (p_bbox->size.y < TILE_SIZE && tile_y > 0)  tile_y--; // hack to detect small bbox state

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

        // Jumps is possible as long as you have a jump

        // Check if possible to jump when jump is pressed
        if (p_pstate->jump_pressed && p_cjump->jumps > 0 && p_cjump->jump_ready)
        {
            p_cjump->jumps--;
            if (!in_water)
            {
                p_ctransform->velocity.y = -p_cjump->jump_speed;
            }
            else
            {
                p_ctransform->velocity.y = -p_cjump->jump_speed / 1.75;
            }

            p_cjump->jumped = true;
            p_cjump->jump_ready = false;
        }

        p_pstate->player_dir.x = 0;
        p_pstate->player_dir.y = 0;
    }
}

void player_bbox_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_player, CBBOX_COMP_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);

        Vector2 new_bbox;
        Vector2 offset;
        if (p_mstate->ground_state & 1)
        {
            AnchorPoint_t anchor = AP_BOT_CENTER;
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
            offset = shift_bbox(p_bbox->size, new_bbox, anchor);
        }
        else
        {
            if (p_mstate->water_state & 1)
            {
                new_bbox.x = PLAYER_C_WIDTH;
                new_bbox.y = PLAYER_C_HEIGHT;
                offset = shift_bbox(p_bbox->size, new_bbox, AP_MID_CENTER);
            }
            else
            {
                new_bbox.x = PLAYER_WIDTH;
                new_bbox.y = PLAYER_HEIGHT;
                offset = shift_bbox(p_bbox->size, new_bbox, AP_MID_CENTER);
            }
        }

        if (
            !check_collision_at(
                p_player->m_id, p_ctransform->position, new_bbox,
                &tilemap, offset, &scene->ent_manager
            )
        )
        {
            set_bbox(p_bbox, new_bbox.x, new_bbox.y);
            p_ctransform->position = Vector2Add(p_ctransform->position, offset);
        }

        CHitBoxes_t* p_hitbox = get_component(&scene->ent_manager, p_player, CHITBOXES_T);
        p_hitbox->boxes[0].height = p_bbox->size.y + 2;
        //p_hitbox->boxes[1].y = p_bbox->size.y / 4;
        p_hitbox->boxes[1].height = p_bbox->size.y - 1;
    }
}

void tile_collision_system(Scene_t* scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};

    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;


    unsigned int ent_idx;
    CBBox_t* p_bbox;
    //sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    sc_map_foreach(&scene->ent_manager.component_map[CBBOX_COMP_T], ent_idx, p_bbox)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        // Get the occupied tiles
        // For each tile, loop through the entities, check collision and move
        // exclude self
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;

        uint32_t collision_value;
        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                if(tilemap.tiles[tile_idx].solid)
                {
                    Vector2 other;
                    other.x = (tile_idx % tilemap.width) * TILE_SIZE;
                    other.y = (tile_idx / tilemap.width) * TILE_SIZE; // Precision loss is intentional

                    check_collision_and_move(
                        &scene->ent_manager, &tilemap, ent_idx,
                        p_ctransform, p_bbox->size, other,
                        TILE_SZ, true, &collision_value
                    );

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
                        if (!p_other_ent->m_alive) continue; // To only allow one way collision check
                        if (p_other_ent->m_tag < p_ent->m_tag) continue; // To only allow one way collision check
                        CBBox_t *p_other_bbox = get_component(&scene->ent_manager, p_other_ent, CBBOX_COMP_T);
                        if (p_other_bbox == NULL) continue;

                        CTransform_t *p_other_ct = get_component(&scene->ent_manager, p_other_ent, CTRANSFORM_COMP_T);
                        if (
                            check_collision_and_move(
                                &scene->ent_manager, &tilemap, ent_idx,
                                p_ctransform, p_bbox->size, p_other_ct->position,
                                p_other_bbox->size, p_other_bbox->solid, &collision_value
                            )
                        )
                        {
                            uint32_t collision_key = ((ent_idx << 16) | other_ent_idx);
                            sc_map_put_32(&data->collision_events, collision_key, collision_value);
                        }
                    }
                }
            }
        }

        // Post movement edge check to zero out velocity
        uint8_t edges = check_bbox_edges(
            &scene->ent_manager, &data->tilemap, ent_idx,
            p_ctransform->position, p_bbox->size
        );
        if (edges & (1<<3))
        {
            if (p_ctransform->velocity.x < 0) p_ctransform->velocity.x = 0;
        }
        if (edges & (1<<2))
        {
            if (p_ctransform->velocity.x > 0) p_ctransform->velocity.x = 0;
        }
        if (edges & (1<<1))
        {
            if (p_ctransform->velocity.y < 0) p_ctransform->velocity.y = 0;
        }
        if (edges & (1))
        {
            if (p_ctransform->velocity.y > 0) p_ctransform->velocity.y = 0;
        }

        // TODO: Resolve all collision events
        //uint32_t collision_key;
        //sc_map_foreach(&data->collision_events, collision_key, collision_value)
        //{
        //    ent_idx = (collision_key >> 16);
        //    uint other_ent_idx = (collision_key & 0xFFFF);
        //    Entity_t *p_ent = get_entity(&scene->ent_manager, ent_idx);
        //    Entity_t *p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
        //    if (!p_ent->m_alive || !p_other_ent->m_alive) continue;
        //}
        sc_map_clear_32(&data->collision_events);

        // Level boundary collision
        unsigned int level_width = tilemap.width * TILE_SIZE;
        if(p_ctransform->position.x < 0 || p_ctransform->position.x + p_bbox->size.x > level_width)
        {
            p_ctransform->position.x = (p_ctransform->position.x < 0) ? 0 : p_ctransform->position.x;
            if (p_ctransform->position.x + p_bbox->size.x > level_width)
            {
                p_ctransform->position.x =  level_width - p_bbox->size.x;
            }
            else
            {
                p_ctransform->position.x = p_ctransform->position.x;
            }
            p_ctransform->velocity.x *= -1;
        }
        
        unsigned int level_height = tilemap.height * TILE_SIZE;
        if(p_ctransform->position.y < 0 || p_ctransform->position.y + p_bbox->size.y > level_height)
        {
            p_ctransform->position.y = (p_ctransform->position.y < 0) ? 0 : p_ctransform->position.y;
            if (p_ctransform->position.y + p_bbox->size.y > level_height)
            {
                p_ctransform->position.y = level_height - p_bbox->size.y;
            }
            else
            {
                p_ctransform->position.y = p_ctransform->position.y;
            }
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

void friction_coefficient_update_system(Scene_t* scene)
{
    CTransform_t* p_ct;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], ent_idx, p_ct)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_ent, CMOVEMENTSTATE_T);


        // Friction
        if (p_mstate->water_state & 1)
        {
            // Apply water friction
            // Consistent in all direction
            p_ct->fric_coeff = (Vector2){-5.5, -5.5};
        }
        else
        {
            // For game feel, y is set to air resistance only
            // x is set to ground resistance (even in air)
            // If not, then player is can go faster by bunny hopping
            // which is fun but not quite beneficial here
            p_ct->fric_coeff = (Vector2){-3.3, -1};
        }

        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
        if (p_pstate != NULL && (p_pstate->is_crouch & 1))
        {
            p_ct->fric_coeff.x -= 4;
        }
    }
}

void global_external_forces_system(Scene_t* scene)
{
    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);

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
        p_ctransform->accel = Vector2Add(
            p_ctransform->accel,
            Vector2Multiply(p_ctransform->fric_coeff, p_ctransform->velocity)
        );


        // Zero out acceleration for contacts with sturdy entites and tiles
        uint8_t edges = check_bbox_edges(
            &scene->ent_manager, &data->tilemap, ent_idx,
            p_ctransform->position, p_bbox->size
        );
        if (edges & (1<<3))
        {
            if (p_ctransform->accel.x < 0) p_ctransform->accel.x = 0;
        }
        if (edges & (1<<2))
        {
            if (p_ctransform->accel.x > 0) p_ctransform->accel.x = 0;
        }
        if (edges & (1<<1))
        {
            if (p_ctransform->accel.y < 0) p_ctransform->accel.y = 0;
        }
        if (edges & (1))
        {
            if (p_ctransform->accel.y > 0) p_ctransform->accel.y = 0;
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
        p_ctransform->velocity = Vector2Scale(
            Vector2Normalize(p_ctransform->velocity),
            (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag
        );

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
    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CJump_t* p_cjump = get_component(&scene->ent_manager, p_player, CJUMP_COMP_T);
        CMovementState_t* p_mstate = get_component(&scene->ent_manager, p_player, CMOVEMENTSTATE_T);
        CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_player, CPLAYERSTATE_T);

        // Handle Ground<->Air Transition
        bool in_water = (p_mstate->water_state & 1);
        // Landing or in water
        if ((p_mstate->ground_state & 1 || in_water) && !p_pstate->jump_pressed )
        {
            // Recover jumps
            p_cjump->jumps = p_cjump->max_jumps;
            p_cjump->jumped = false;
            p_cjump->short_hop = false;
            p_cjump->jump_ready = true;
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

void state_transition_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    //Entity_t* p_ent;

    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        if (p_ctransform == NULL || p_bbox == NULL) continue;

        bool on_ground = check_on_ground(
            ent_idx, p_ctransform->position, p_bbox->size,
            &data->tilemap, &scene->ent_manager
        );
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
            for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
            {
                for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
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

void update_tilemap_system(Scene_t* scene)
{
    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    CTileCoord_t* p_tilecoord;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTILECOORD_COMP_T], ent_idx, p_tilecoord)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        if (p_ctransform == NULL) continue;
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        if (p_bbox == NULL) continue;

        // Update tilemap position
        for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
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
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y - 1) / TILE_SIZE;

        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                p_tilecoord->tiles[p_tilecoord->n_tiles++] = tile_idx;
                sc_map_put_64(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id, p_ent->m_id);
            }
        }
    }
}

void hitbox_update_system(Scene_t* scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};

    LevelSceneData_t* data = (LevelSceneData_t*)scene->scene_data;
    TileGrid_t tilemap = data->tilemap;

    unsigned int ent_idx;
    CHitBoxes_t* p_hitbox;

    sc_map_foreach(&scene->ent_manager.component_map[CHITBOXES_T], ent_idx, p_hitbox)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        for (uint8_t i = 0; i < p_hitbox->n_boxes; ++i)
        {
            Vector2 hitbox_pos = {
                .x = p_ctransform->position.x + p_hitbox->boxes[i].x,
                .y = p_ctransform->position.y + p_hitbox->boxes[i].y,
            };

            unsigned int tile_x1 = (hitbox_pos.x) / TILE_SIZE;
            unsigned int tile_y1 = (hitbox_pos.y) / TILE_SIZE;
            unsigned int tile_x2 = (hitbox_pos.x + p_hitbox->boxes[i].width - 1) / TILE_SIZE;
            unsigned int tile_y2 = (hitbox_pos.y + p_hitbox->boxes[i].height - 1) / TILE_SIZE;

            for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
            {
                for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
                {
                    unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                    unsigned int other_ent_idx;
                    memset(checked_entities, 0, sizeof(checked_entities));
                    sc_map_foreach_key(&tilemap.tiles[tile_idx].entities_set, other_ent_idx)
                    {
                        if (other_ent_idx == ent_idx) continue;
                        if (checked_entities[other_ent_idx]) continue;

                        Entity_t* p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
                        if (!p_other_ent->m_alive) continue; // To only allow one way collision check
                        if (p_other_ent->m_tag < p_ent->m_tag) continue; // To only allow one way collision check

                        CHurtbox_t* p_other_hurtbox = get_component(&scene->ent_manager, p_other_ent, CHURTBOX_T);
                        if (p_other_hurtbox == NULL) continue;
                        CTransform_t* p_other_ct = get_component(&scene->ent_manager, p_other_ent, CTRANSFORM_COMP_T);
                        Vector2 hurtbox_pos = Vector2Add(p_other_ct->position, p_other_hurtbox->offset);

                        Vector2 overlap;
                        if (
                            find_AABB_overlap(
                                hitbox_pos, (Vector2){p_hitbox->boxes[i].width, p_hitbox->boxes[i].height},
                                hurtbox_pos, p_other_hurtbox->size, &overlap
                            )
                        )
                        {
                            if (!p_other_hurtbox->fragile) continue;
                            if (p_other_ent->m_tag == CRATES_ENT_TAG)
                            {

                                CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
                                CPlayerState_t* p_pstate = get_component(&scene->ent_manager, p_ent, CPLAYERSTATE_T);
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

                                CTileCoord_t* p_tilecoord = get_component(
                                    &scene->ent_manager, p_other_ent, CTILECOORD_COMP_T
                                );

                                for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
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
                }
            }
        }
    }
}

void sprite_animation_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CSprite_t* p_cspr;
    sc_map_foreach(&scene->ent_manager.component_map[CSPRITE_T], ent_idx, p_cspr)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        // Update animation state
        if (p_cspr->transition_func != NULL)
        {
            unsigned int spr_idx = p_cspr->transition_func(p_ent);
            if (p_cspr->current_idx != spr_idx)
            {
                Sprite_t* new_spr = get_sprite(&scene->engine->assets, p_cspr->sprites_map[spr_idx]);
                if (new_spr != NULL)
                {
                    p_cspr->sprite = new_spr;
                    p_cspr->current_idx = spr_idx;
                    p_cspr->sprite->current_frame = 0;
                }
            }
        }
        // Animate it (handle frame count)
        p_cspr->sprite->elapsed++;
        if (p_cspr->sprite->elapsed == p_cspr->sprite->speed)
        {
            p_cspr->sprite->current_frame++;
            p_cspr->sprite->current_frame %= p_cspr->sprite->frame_count;
            p_cspr->sprite->elapsed = 0;
        }
    }
}

void init_level_scene_data(LevelSceneData_t* data)
{
    sc_map_init_32(&data->collision_events, 128, 0);
}

void term_level_scene_data(LevelSceneData_t* data)
{
    sc_map_term_32(&data->collision_events);
}

unsigned int player_sprite_transition_func(Entity_t* ent)
{
    return SPR_PLAYER_RUN;
}
