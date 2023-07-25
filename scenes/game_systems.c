#include "game_systems.h"
#include "ent_impl.h"
#include "AABB.h"
#include "EC.h"
#include "constants.h"
#include <stdio.h>

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

void change_a_tile(TileGrid_t* tilemap, unsigned int tile_idx, TileType_t new_type)
{
    TileType_t last_type = tilemap->tiles[tile_idx].tile_type;
    tilemap->tiles[tile_idx].tile_type = new_type;

    switch (new_type)
    {
        case EMPTY_TILE:
            tilemap->tiles[tile_idx].solid = NOT_SOLID;
        break;
        case ONEWAY_TILE:
            tilemap->tiles[tile_idx].solid = ONE_WAY;
        break;
        case LADDER:
        {
            int up_tile = tile_idx - tilemap->width;
            if (up_tile > 0 && tilemap->tiles[up_tile].tile_type != LADDER)
            {
                tilemap->tiles[tile_idx].solid = ONE_WAY;
            }
            else
            {
                tilemap->tiles[tile_idx].solid = NOT_SOLID;
            }
            int down_tile = tile_idx + tilemap->width;
            if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
            {
                tilemap->tiles[down_tile].solid = (tilemap->tiles[tile_idx].tile_type != LADDER)? ONE_WAY : NOT_SOLID;
            }
        }
        break;
        case SPIKES:
            tilemap->tiles[tile_idx].solid = NOT_SOLID;
        break;
        case SOLID_TILE:
            tilemap->tiles[tile_idx].solid = SOLID;
        break;
    }

    if (last_type == LADDER && new_type != LADDER)
    {
        int down_tile = tile_idx + tilemap->width;
        if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
        {
            tilemap->tiles[down_tile].solid = ONE_WAY;
        }

    }

    if (new_type == SPIKES)
    {
        // Priority: Down, Up, Left, Right
        if (tile_idx + tilemap->width < tilemap->n_tiles && tilemap->tiles[tile_idx + tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,16};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
        }
        else if (tile_idx - tilemap->width >= 0 && tilemap->tiles[tile_idx - tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
        }
        else if (tile_idx % tilemap->width != 0 && tilemap->tiles[tile_idx - 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
        }
        else if ((tile_idx + 1) % tilemap->width != 0 && tilemap->tiles[tile_idx + 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){16,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
        }
        else
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,16};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
        }
    }
    else if (new_type == ONEWAY_TILE)
    {
        tilemap->tiles[tile_idx].offset = (Vector2){0,0};
        tilemap->tiles[tile_idx].size = (Vector2){32,10};
    }
    else
    {
        tilemap->tiles[tile_idx].offset = (Vector2){0,0};
        tilemap->tiles[tile_idx].size = (Vector2){32,32};
    }

    tilemap->tiles[tile_idx].moveable = (
        tilemap->tiles[tile_idx].tile_type == EMPTY_TILE
        || tilemap->tiles[tile_idx].tile_type == SPIKES
    );
    tilemap->tiles[tile_idx].def = (tilemap->tiles[tile_idx].tile_type == SOLID_TILE) ? 5: 2;

}

// ------------------------- Collision functions ------------------------------------
// Do not subtract one for the size for any collision check, just pass normally. The extra one is important for AABB test

static bool check_collision_and_move(
    TileGrid_t* tilemap,
    Entity_t* ent, Vector2* other_pos, Vector2 other_bbox,
    SolidType_t other_solid
)
{
    CTransform_t* p_ct = get_component(ent, CTRANSFORM_COMP_T);
    CBBox_t* p_bbox = get_component(ent, CBBOX_COMP_T);
    Vector2 overlap = {0,0};
    Vector2 prev_overlap = {0,0};
    uint8_t overlap_mode = find_AABB_overlap(p_ct->position, p_bbox->size, *other_pos, other_bbox, &overlap);
    if (overlap_mode == 1)
    {
        // If there is collision, use previous overlap to determine direction
        find_AABB_overlap(p_ct->prev_position, p_bbox->size, *other_pos, other_bbox, &prev_overlap);

        // Store collision event here
        // Check collision on x or y axis
        // Encode key and value, -ve is left and up, +ve is right and down
        Vector2 offset = {0, 0};
        if (fabs(prev_overlap.y) > fabs(prev_overlap.x))
        {
            offset.x = overlap.x;
        }
        else if (fabs(prev_overlap.x) > fabs(prev_overlap.y))
        {
            offset.y = overlap.y;
        }
        else if (fabs(overlap.x) < fabs(overlap.y))
        {
            offset.x = overlap.x;
        }
        else
        {
            offset.y = overlap.y;
        }

        // Resolve collision via moving player by the overlap amount only if other is solid
        // also check for empty to prevent creating new collision. Not fool-proof, but good enough
        //if (other_solid && !check_collision_at(ent_idx, p_ct->position, sz, tilemap, offset, p_manager))
        if ( other_solid == SOLID
            || (
                other_solid == ONE_WAY && offset.y  < 0
                && (p_ct->prev_position.y + p_bbox->size.y - 1 < other_pos->y))
        )
        {
            //if (!check_collision_offset(ent, p_ct->position, p_bbox->size, tilemap, offset))
            {
                p_ct->position = Vector2Add(p_ct->position, offset);
            }
            //else 
            {
            
                //*other_pos = Vector2Subtract(*other_pos, offset);
            }
        }
    }
    else if (overlap_mode == 2)
    {
        if ( other_solid != SOLID ) goto collision_end;
        // On complete overlap, find a free space in this order: top, left, right, bottom
        Vector2 point_to_test = {0};
        point_to_test.x = p_ct->position.x;
        point_to_test.y = other_pos->y - p_bbox->size.y + 1;
        if (!check_collision_offset(ent, point_to_test, p_bbox->size, tilemap, (Vector2){0}))
        {
            p_ct->position = point_to_test;
            goto collision_end;
        }

        point_to_test.x = other_pos->x - p_bbox->size.x + 1;
        point_to_test.y = p_ct->position.y;
        if (!check_collision_offset(ent, point_to_test, p_bbox->size, tilemap, (Vector2){0}))
        {
            p_ct->position = point_to_test;
            goto collision_end;
        }

        point_to_test.x = other_pos->x + other_bbox.x - 1;
        point_to_test.y = p_ct->position.y;
        if (!check_collision_offset(ent, point_to_test, p_bbox->size, tilemap, (Vector2){0}))
        {
            p_ct->position = point_to_test;
            goto collision_end;
        }

        point_to_test.x = p_ct->position.x;
        point_to_test.y = other_pos->y + other_bbox.y - 1;
        if (!check_collision_offset(ent, point_to_test, p_bbox->size, tilemap, (Vector2){0}))
        {
            p_ct->position = point_to_test;
            goto collision_end;
        }
        // If no free space, Move up no matter what
        p_ct->position.x = p_ct->position.x;
        p_ct->position.y = other_pos->y - p_bbox->size.y + 1;
    }
collision_end:
    return overlap_mode > 0;
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

void player_respawn_system(Scene_t* scene)
{
    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        if (!p_player->m_alive)
        {
            p_player->m_alive = true;
            CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
            memset(&p_ctransform->position, 0, sizeof(p_ctransform->position));
            memset(&p_ctransform->velocity, 0, sizeof(p_ctransform->velocity));
            memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));
        }
    }
}

void player_dir_reset_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        p_pstate->player_dir.x = 0;
        p_pstate->player_dir.y = 0;
    }
}

void player_movement_input_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    // Deal with player acceleration/velocity via inputs first
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);
        CJump_t* p_cjump = get_component(p_player, CJUMP_COMP_T);
        CMovementState_t* p_mstate = get_component(p_player, CMOVEMENTSTATE_T);

        // Ladder handling
        if (!p_pstate->ladder_state)
        {
            if (p_pstate->player_dir.y < 0)
            {
                unsigned int tile_idx = get_tile_idx(
                    p_ctransform->position.x + p_bbox->half_size.x,
                    p_ctransform->position.y + p_bbox->half_size.y,
                    data->tilemap.width
                );
                if (tilemap.tiles[tile_idx].tile_type == LADDER && p_ctransform->velocity.y >= 0)
                {
                    p_pstate->ladder_state = true;
                    p_ctransform->position.y--;
                }
            }
            else if (p_pstate->player_dir.y > 0)
            {
                unsigned int tile_idx;

                if (p_mstate->ground_state & 1)
                {
                    tile_idx = get_tile_idx(
                        p_ctransform->position.x + p_bbox->half_size.x,
                        p_ctransform->position.y + p_bbox->size.y,
                        data->tilemap.width
                    );
                }
                else
                {
                    tile_idx = get_tile_idx(
                        p_ctransform->position.x + p_bbox->half_size.x,
                        p_ctransform->position.y + p_bbox->half_size.y,
                        data->tilemap.width
                    );
                }
                if (tile_idx < tilemap.n_tiles && tilemap.tiles[tile_idx].tile_type == LADDER)
                {
                    p_pstate->ladder_state = true;
                    p_ctransform->position.y++;
                }
            }
        }
        else
        {
            unsigned int tile_x = (p_ctransform->position.x + p_bbox->half_size.x) / TILE_SIZE;
            unsigned int tile_y1 = (p_ctransform->position.y + p_bbox->half_size.y) / TILE_SIZE;
            unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;
            
            p_pstate->ladder_state = false;
            if (!(p_mstate->ground_state & 1))
            {
                for(unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
                {
                    unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                    p_pstate->ladder_state |= tilemap.tiles[tile_idx].tile_type == LADDER;
                }
            }
            if (p_pstate->ladder_state)
            {
                p_ctransform->velocity.y = p_pstate->player_dir.y * 150;
                p_ctransform->velocity.x = p_pstate->player_dir.x * 40;
                if (p_pstate->player_dir.y != 0)
                {
                    p_ctransform->position.x = tile_x * TILE_SIZE + 1;
                }
            }
        }

        bool in_water = (p_mstate->water_state & 1);
        p_pstate->is_crouch |= (p_pstate->player_dir.y > 0)? 0b10 : 0;
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

        uint8_t collide_type = check_collision_offset(
                p_player, p_ctransform->position, p_bbox->size,
                &tilemap, (Vector2){0, p_bbox->size.y - PLAYER_HEIGHT}
        );
        if (collide_type == 1)
        {
            p_pstate->is_crouch |= 0b10;
        }
        if (!(p_mstate->ground_state & 1)) p_pstate->is_crouch &= 0b01;
        p_pstate->is_crouch >>= 1;

        // Jumps is possible as long as you have a jump

        // Check if possible to jump when jump is pressed
        if (p_pstate->jump_pressed && p_cjump->jumps > 0 && p_cjump->jump_ready)
        {
            p_pstate->ladder_state = false;
            p_cjump->jumps--;
            if (!in_water)
            {
                if (p_mstate->ground_state & 1)
                {
                    p_ctransform->velocity.y = -p_cjump->jump_speed;
                }
                else
                {
                    p_ctransform->velocity.y = -p_cjump->jump_speed / 1.4;
                }
            }
            else
            {
                p_ctransform->velocity.y = -p_cjump->jump_speed / 1.75;
            }

            p_cjump->jumped = true;
            p_cjump->jump_ready = false;
        }

    }
}

void player_bbox_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);
        CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);
        CMovementState_t* p_mstate = get_component(p_player, CMOVEMENTSTATE_T);

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
            if ((p_mstate->water_state & 1) && !p_pstate->ladder_state)
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
            check_collision_offset(
                p_player, p_ctransform->position, new_bbox,
                &tilemap, offset 
            ) != 1
        )
        {
            set_bbox(p_bbox, new_bbox.x, new_bbox.y);
            p_ctransform->position = Vector2Add(p_ctransform->position, offset);
        }

        CHitBoxes_t* p_hitbox = get_component(p_player, CHITBOXES_T);
        p_hitbox->boxes[0].height = p_bbox->size.y + 2;
        //p_hitbox->boxes[1].y = p_bbox->size.y / 4;
        p_hitbox->boxes[1].height = p_bbox->size.y - 1;
    }
}

void player_crushing_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);

        uint8_t edges = check_bbox_edges(
            &data->tilemap, p_player,
            p_ctransform->position, p_ctransform->prev_position, p_bbox->size, true
        );

        // There is a second check for to ensure that there is an solid entity/tile overlapping the player bbox
        // This is to prevent crushing by perfectly fitting in a gap (imagine a size 32 player in between two tiles)
        // or any scenario where the edge check is just fringing, not overlapping
        if ((edges & 0b0011) == 0b0011)
        {
            uint8_t collide = 0;
            CollideEntity_t ent = 
            {
                .p_ent = p_player,
                .bbox = (Rectangle){p_ctransform->position.x, p_ctransform->position.y, p_bbox->size.x, 1},
                .prev_bbox = (Rectangle){p_ctransform->position.x, p_ctransform->position.y, p_bbox->size.x, p_bbox->size.y},
                .area = (TileArea_t){
                    .tile_x1 = (p_ctransform->position.x) / TILE_SIZE,
                    .tile_y1 = (p_ctransform->position.y) / TILE_SIZE,
                    .tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE,
                    .tile_y2 = (p_ctransform->position.y + p_bbox->size.y - 1) / TILE_SIZE,
                }
            };
            
            
            uint8_t collide_type = check_collision_line(&ent, &data->tilemap, false);
            if (collide_type == 1)
            {
                collide |= 1 << 1;
            }
            
            ent.bbox.y = p_ctransform->position.y + p_bbox->size.y;
            collide_type = check_collision_line(&ent, &data->tilemap, true);
            if (collide_type == 1)
            {
                collide |= 1;
            }

            if (collide != 0)
            {
                p_player->m_alive = false;
                return;
            }
        }

        if ((edges & 0b1100) == 0b1100)
        {
            uint8_t collide = 0;
            CollideEntity_t ent = 
            {
                .p_ent = p_player,
                .bbox = (Rectangle){p_ctransform->position.x, p_ctransform->position.y, 1, p_bbox->size.y},
                .prev_bbox = (Rectangle){p_ctransform->position.x, p_ctransform->position.y, p_bbox->size.x, p_bbox->size.y},
                .area = (TileArea_t){
                    .tile_x1 = (p_ctransform->position.x) / TILE_SIZE,
                    .tile_y1 = (p_ctransform->position.y) / TILE_SIZE,
                    .tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE,
                    .tile_y2 = (p_ctransform->position.y + p_bbox->size.y - 1) / TILE_SIZE,
                }
            };

            // Left
            uint8_t collide_type = check_collision_line(&ent, &data->tilemap, false);
            if (collide_type == 1)
            {
                collide |= 1 << 1;
            }

            //Right
            ent.bbox.x = p_ctransform->position.x + p_bbox->size.x; // 2 to account for the previous subtraction
            collide_type = check_collision_line(&ent, &data->tilemap, false);
            if (collide_type == 1)
            {
                collide |= 1;
            }

            if (collide != 0)
            {
                p_player->m_alive = false;
                return;
            }
        }
    }
}

void spike_collision_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    //Entity_t* p_player;
    CBBox_t* p_bbox;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CBBOX_COMP_T], ent_idx, p_bbox)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y - 1) / TILE_SIZE;

        tile_x1 = (tile_x1 < 0) ? 0 : tile_x1;
        tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
        tile_y1 = (tile_y1 < 0) ? 0 : tile_y1;
        tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.height - 1 : tile_y2;

        Vector2 overlap;
        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                if(tilemap.tiles[tile_idx].tile_type == SPIKES)
                {
                    uint8_t collide = find_AABB_overlap(
                        p_ctransform->position, p_bbox->size, 
                        (Vector2){
                            tile_x * TILE_SIZE + tilemap.tiles[tile_idx].offset.x,
                            tile_y * TILE_SIZE + tilemap.tiles[tile_idx].offset.y
                        },
                    tilemap.tiles[tile_idx].size,
                    &overlap);

                    if (collide)
                    {
                        if (p_ent->m_tag == PLAYER_ENT_TAG)
                        {
                            p_ent->m_alive = false;
                            return;
                        }
                        else
                        {
                            tilemap.tiles[tile_idx].tile_type = EMPTY_TILE;
                        }
                    }
                }
            }
        }
    }
}

void tile_collision_system(Scene_t* scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};

    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;


    unsigned int ent_idx;
    CBBox_t* p_bbox;
    //sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    sc_map_foreach(&scene->ent_manager.component_map[CBBOX_COMP_T], ent_idx, p_bbox)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        if (!p_ctransform->active) continue;
        // Get the occupied tiles
        // For each tile, loop through the entities, check collision and move
        // exclude self
        // This has an extra pixel when gathering potential collision, just to avoid missing any
        // This is only done here, collision methods do not have this
        unsigned int tile_x1 = (p_ctransform->position.x - 1) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y - 1) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y + p_bbox->size.y) / TILE_SIZE;

        tile_x1 = (tile_x1 < 0) ? 0 : tile_x1;
        tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
        tile_y1 = (tile_y1 < 0) ? 0 : tile_y1;
        tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.height - 1 : tile_y2;

        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                if(tilemap.tiles[tile_idx].tile_type != EMPTY_TILE)
                {
                    Vector2 other;
                    other.x = (tile_idx % tilemap.width) * TILE_SIZE + tilemap.tiles[tile_idx].offset.x;
                    other.y = (tile_idx / tilemap.width) * TILE_SIZE + tilemap.tiles[tile_idx].offset.y; // Precision loss is intentional

                    check_collision_and_move(
                        &tilemap, p_ent,
                        &other, tilemap.tiles[tile_idx].size, tilemap.tiles[tile_idx].solid
                    );

                }
                else
                {
                    unsigned int other_ent_idx;
                    Entity_t* p_other_ent;
                    memset(checked_entities, 0, sizeof(checked_entities));
                    sc_map_foreach(&tilemap.tiles[tile_idx].entities_set, other_ent_idx, p_other_ent)
                    {
                        if (other_ent_idx == ent_idx) continue;
                        if (checked_entities[other_ent_idx]) continue;

                        checked_entities[other_ent_idx] = true;
                        if (!p_other_ent->m_alive) continue; // No need to move if other is dead
                        CBBox_t *p_other_bbox = get_component(p_other_ent, CBBOX_COMP_T);
                        if (p_other_bbox == NULL) continue;

                        CTransform_t *p_other_ct = get_component(p_other_ent, CTRANSFORM_COMP_T);
                        check_collision_and_move(
                            &tilemap, p_ent,
                            &p_other_ct->position, p_other_bbox->size,
                            p_other_bbox->solid? SOLID : NOT_SOLID
                        );
                    }
                }
            }
        }

        // Post movement edge check to zero out velocity
        uint8_t edges = check_bbox_edges(
            &data->tilemap, p_ent,
            p_ctransform->position, p_ctransform->prev_position, p_bbox->size, false
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
        CMovementState_t* p_mstate = get_component(p_ent, CMOVEMENTSTATE_T);


        // Friction
        if (p_mstate != NULL && p_mstate->water_state & 1)
        {
            // Apply water friction
            // Consistent in all direction
            p_ct->fric_coeff = (Vector2){-WATER_FRICTION, -WATER_FRICTION};
        }
        else
        {
            // For game feel, y is set to air resistance only
            // x is set to ground resistance (even in air)
            // If not, then player is can go faster by bunny hopping
            // which is fun but not quite beneficial here
            p_ct->fric_coeff = (Vector2){-GROUND_X_FRICTION, -GROUND_Y_FRICTION};
        }

        CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
        if (p_pstate != NULL && (p_pstate->is_crouch & 1))
        {
            p_ct->fric_coeff.x -= 4;
        }
    }
}

void global_external_forces_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);

        if (p_ctransform->grav_timer > 0)
        {
            p_ctransform->grav_timer--;
            continue;
        }

        if (!(p_mstate->ground_state & 1))
        {
            // Only apply upthrust if center is in water
            // If need more accuracy, need to find area of overlap
            if (p_mstate->water_state & 1)
            {
                Vector2 player_center = (Vector2){
                    .x = p_ctransform->position.x + p_bbox->half_size.x,
                    .y = p_ctransform->position.y + p_bbox->half_size.y,
                };
                unsigned int tile_idx = get_tile_idx(
                    player_center.x,
                    player_center.y,
                    data->tilemap.width
                );
                if (data->tilemap.tiles[tile_idx].water_level > 0)
                {
                    uint32_t water_height = data->tilemap.tiles[tile_idx].water_level * WATER_BBOX_STEP;
                    Rectangle box = {
                        .x = (tile_idx % data->tilemap.width) * data->tilemap.tile_size,
                        .y = (tile_idx / data->tilemap.width + 1) * data->tilemap.tile_size - water_height,
                        .width = data->tilemap.tile_size,
                        .height = water_height,
                    };

                    if (point_in_AABB(player_center, box))
                    {
                        p_ctransform->accel = Vector2Multiply(Vector2Add(p_ctransform->accel, UPTHRUST), p_ctransform->shape_factor);
                    }
                }
            }
            p_ctransform->accel = Vector2Add(p_ctransform->accel, GRAVITY);
        }

        // Friction
        p_ctransform->accel = Vector2Add(
            p_ctransform->accel,
            Vector2Multiply(
                Vector2Multiply(p_ctransform->fric_coeff, p_ctransform->velocity),
                p_ctransform->shape_factor
            )
        );
        
        if (p_bbox == NULL) continue; //Do not proceed if no bbox


        // Zero out acceleration for contacts with sturdy entites and tiles
        uint8_t edges = check_bbox_edges(
            &data->tilemap, p_ent,
            p_ctransform->position, p_ctransform->prev_position, p_bbox->size, false
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
    CPlayerState_t* p_pstate;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        if (p_pstate->ladder_state)
        {
            p_ctransform->accel = (Vector2){0,0};
        }
    }
}

void moveable_update_system(Scene_t* scene)
{
    CMoveable_t* p_moveable;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEABLE_T], ent_idx, p_moveable)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);

        if (p_moveable->gridmove)
        {
            float remaining_distance = p_moveable->target_pos.x - p_ctransform->position.x;
            if (fabs(remaining_distance) < 0.1)
            {
                p_ctransform->prev_position = p_moveable->prev_pos;
                p_ctransform->position = p_moveable->target_pos;
                p_moveable->gridmove = false;
                p_bbox->solid = true;
                p_ctransform->movement_mode = REGULAR_MOVEMENT;
            }
            else if (remaining_distance > 0.1)
            {
                p_ctransform->position.x +=  (remaining_distance > p_moveable->move_speed) ? p_moveable->move_speed : remaining_distance;
            }
            else
            {
                p_ctransform->position.x +=  (remaining_distance < -p_moveable->move_speed) ? -p_moveable->move_speed : remaining_distance;
                memset(&p_ctransform->velocity, 0, sizeof(p_ctransform->velocity));
            }
        }

        // Intentional. Want this check even after a gridmove to allow gridmove after that
        if (!p_moveable->gridmove)
        {
            if (p_ctransform->prev_velocity.y <= 0 && p_ctransform->prev_position.x == p_ctransform->position.x) continue;

            TileGrid_t tilemap = (CONTAINER_OF(scene, LevelScene_t, scene)->data).tilemap;
            Vector2 point_to_check = {
                .x = p_ctransform->position.x + p_bbox->half_size.x,
                .y = p_ctransform->position.y + p_bbox->size.y + 5 // 5 is arbitrary, just to make sure there's a little gap
            };
            int tile_x = point_to_check.x / TILE_SIZE;
            int tile_y = point_to_check.y / TILE_SIZE;
            if (tile_y >= tilemap.height) continue;

            int tile_idx = tile_y * tilemap.width + tile_x;
            unsigned int other_ent_idx;
            bool can_move = false;
            int tile_y2 = tile_y - 1;
            sc_map_foreach_key(&tilemap.tiles[tile_idx].entities_set, other_ent_idx)
            {
                if (other_ent_idx == ent_idx) continue;
                sc_map_get_64v(&scene->ent_manager.component_map[CMOVEABLE_T], other_ent_idx);
                if (!sc_map_found(&scene->ent_manager.component_map[CMOVEABLE_T])) continue;

                {
                    Entity_t* other_ent = get_entity(&scene->ent_manager, other_ent_idx);
                    CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                    CTransform_t* p_other_ct = get_component(other_ent, CTRANSFORM_COMP_T);
                    Rectangle box = {p_other_ct->position.x, p_other_ct->position.y, p_other_bbox->size.x, p_other_bbox->size.y};
                    if (!point_in_AABB(point_to_check, box)) continue;
                }

                tile_x = (p_ctransform->position.x) / TILE_SIZE - 1;

                if (tile_x >= 0 && tile_x < tilemap.width)
                {
                    unsigned int tile_idx1 = tile_y * tilemap.width + tile_x;
                    unsigned int tile_idx2 = tile_y2 * tilemap.width + tile_x;
                    if ( tilemap.tiles[tile_idx1].moveable && tilemap.tiles[tile_idx2].moveable )
                    {
                        bool any_solid = false;
                        unsigned int idx_to_check;
                        sc_map_foreach_key(&tilemap.tiles[tile_idx1].entities_set, idx_to_check)
                        {
                            Entity_t* other_ent = get_entity(&scene->ent_manager, idx_to_check);
                            CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                            if (p_other_bbox != NULL)
                            {
                                any_solid |= p_other_bbox->solid;
                            }
                        }
                        sc_map_foreach_key(&tilemap.tiles[tile_idx2].entities_set, idx_to_check)
                        {
                            Entity_t* other_ent = get_entity(&scene->ent_manager, idx_to_check);
                            CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                            if (p_other_bbox != NULL)
                            {
                                any_solid |= p_other_bbox->solid;
                            }
                        }
                        if (!any_solid)
                        {
                            can_move = true;
                            break;
                        }
                    }
                }

                tile_x += 2;
                if (tile_x >= 0 && tile_x < tilemap.width)
                {
                    unsigned int tile_idx1 = tile_y * tilemap.width + tile_x;
                    unsigned int tile_idx2 = tile_y2 * tilemap.width + tile_x;
                    if ( tilemap.tiles[tile_idx1].moveable && tilemap.tiles[tile_idx2].moveable )
                    {
                        bool any_solid = false;
                        unsigned int idx_to_check;
                        sc_map_foreach_key(&tilemap.tiles[tile_idx1].entities_set, idx_to_check)
                        {
                            Entity_t* other_ent = get_entity(&scene->ent_manager, idx_to_check);
                            CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                            if (p_other_bbox != NULL)
                            {
                                any_solid |= p_other_bbox->solid;
                            }
                        }
                        sc_map_foreach_key(&tilemap.tiles[tile_idx2].entities_set, idx_to_check)
                        {
                            Entity_t* other_ent = get_entity(&scene->ent_manager, idx_to_check);
                            CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                            if (p_other_bbox != NULL)
                            {
                                any_solid |= p_other_bbox->solid;
                            }
                        }
                        if (!any_solid)
                        {
                            can_move = true;
                            break;
                        }
                    }
                }
                break;
            }
            if (can_move)
            {
                p_moveable->gridmove = true;
                p_bbox->solid = false;
                p_moveable->prev_pos = p_ctransform->position;
                p_moveable->target_pos = Vector2Scale((Vector2){tile_x,tile_y2}, TILE_SIZE); 
                memset(&p_ctransform->velocity, 0, sizeof(p_ctransform->velocity));
                memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));
                p_ctransform->movement_mode = KINEMATIC_MOVEMENT;
            }

        }

    }

}

void player_pushing_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CMovementState_t* p_movement = get_component(p_player, CMOVEMENTSTATE_T);
        CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);
        if (!(p_movement->ground_state & 1) || p_pstate->player_dir.x == 0) continue;

        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);
        Vector2 point_to_check = p_ctransform->position;
        point_to_check.y += p_bbox->half_size.y;
        if (p_pstate->player_dir.x > 0)
        {
            point_to_check.x += p_bbox->size.x + 1;
        }
        else
        {
            point_to_check.x -= 1;
        }
        //CHitBoxes_t* p_hitbox = get_component(p_player, CHITBOXES_T);
        // Get the occupied tiles
        // For each tile, loop through the entities, check collision and move
        // exclude self
        unsigned int tile_x = (point_to_check.x) / TILE_SIZE;
        unsigned int tile_y = (point_to_check.y) / TILE_SIZE;
        unsigned int tile_idx = tile_y * tilemap.width + tile_x;
        if(tilemap.tiles[tile_idx].tile_type != EMPTY_TILE) continue;

        Entity_t* p_other_ent;
        sc_map_foreach_value(&tilemap.tiles[tile_idx].entities_set, p_other_ent)
        {
            if (p_other_ent->m_id == p_player->m_id) continue;

            if (!p_other_ent->m_alive) continue;

            CMoveable_t *p_other_moveable = get_component(p_other_ent, CMOVEABLE_T);
            if (p_other_moveable == NULL) continue;

            CTransform_t *p_other_ct = get_component(p_other_ent, CTRANSFORM_COMP_T);
            CBBox_t *p_other_bbox = get_component(p_other_ent, CBBOX_COMP_T);
            Rectangle box = {
                .x = p_other_ct->position.x,
                .y = p_other_ct->position.y,
                .width = p_other_bbox->size.x,
                .height = p_other_bbox->size.y
            };
            if (point_in_AABB(point_to_check, box))
            {
                Vector2 target_pos = p_other_ct->position;
                if (p_ctransform->position.x < p_other_ct->position.x)
                {
                    target_pos.x += TILE_SIZE;
                    tile_x++;
                }
                else
                {
                    target_pos.x -= TILE_SIZE;
                    tile_x--;
                }

                if (tile_x >= 0 && tile_x < tilemap.width)
                {
                    unsigned int target_tile_idx = tile_y * tilemap.width + tile_x;
                    if (
                        tilemap.tiles[target_tile_idx].moveable
                        && sc_map_size_64v(&tilemap.tiles[target_tile_idx].entities_set) == 0
                    )
                    {
                        p_other_moveable->gridmove = true;
                        p_other_moveable->prev_pos = p_other_ct->position;
                        p_other_moveable->target_pos = target_pos;
                        memset(&p_ctransform->velocity, 0, sizeof(p_ctransform->velocity));
                        memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));
                        p_other_ct->movement_mode = KINEMATIC_MOVEMENT;
                    }
                }
            }
        }
    }
}

void movement_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    // Update movement
    float delta_time = 0.017; // TODO: Will need to think about delta time handling
    CTransform_t * p_ctransform;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], ent_idx, p_ctransform)
    {
        if (!p_ctransform->active)
        {
            memset(&p_ctransform->velocity, 0, sizeof(Vector2));
            memset(&p_ctransform->accel, 0, sizeof(Vector2));
            continue;
        }
        p_ctransform->prev_velocity = p_ctransform->velocity;

        if (p_ctransform->movement_mode == REGULAR_MOVEMENT)
        {
            p_ctransform->velocity =
                Vector2Add(
                    p_ctransform->velocity,
                    Vector2Scale(p_ctransform->accel, delta_time)
                );
        }

        float mag = Vector2Length(p_ctransform->velocity);
        p_ctransform->velocity = Vector2Scale(
            Vector2Normalize(p_ctransform->velocity),
            (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag
        );

        // 3 dp precision
        if (fabs(p_ctransform->velocity.x) < 1e-3) p_ctransform->velocity.x = 0;
        if (fabs(p_ctransform->velocity.y) < 1e-3) p_ctransform->velocity.y = 0;

        // Store previous position before update
        p_ctransform->prev_position = p_ctransform->position;
        p_ctransform->position = Vector2Add(
            p_ctransform->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );
        memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));

        // Level boundary collision
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
        unsigned int level_width = tilemap.width * TILE_SIZE;
        unsigned int level_height = tilemap.height * TILE_SIZE;
        if (p_bbox != NULL)
        {

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
                p_ctransform->velocity.x = 0;
            }
            
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
                p_ctransform->velocity.y = 0;
            }
        }
        else
        {
            if (
                p_ctransform->position.x < 0 || p_ctransform->position.x > level_width
                || p_ctransform->position.y < 0 || p_ctransform->position.y > level_height
            )
            {
                remove_entity_from_tilemap(&scene->ent_manager, &tilemap, p_ent);
            }
        
        }
    }
}

void player_ground_air_transition_system(Scene_t* scene)
{
    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CJump_t* p_cjump = get_component(p_player, CJUMP_COMP_T);
        const CMovementState_t* p_mstate = get_component(p_player, CMOVEMENTSTATE_T);
        const CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);

        // Handle Ground<->Air Transition
        bool in_water = (p_mstate->water_state & 1);
        // Landing or in water
        if ((p_mstate->ground_state & 1 || in_water || p_pstate->ladder_state) && !p_pstate->jump_pressed )
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
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    CMovementState_t* p_mstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMOVEMENTSTATE_T], ent_idx, p_mstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
        if (p_ctransform == NULL || p_bbox == NULL) continue;

        bool on_ground = check_on_ground(
            p_ent, p_ctransform->position, p_ctransform->prev_position, p_bbox->size,
            &data->tilemap
        );

        if (on_ground)
        {
            p_ctransform->grav_timer = p_ctransform->grav_delay;
        }

        bool in_water = false;
        if (!(p_mstate->water_state & 1))
        {
            Vector2 player_center = (Vector2){
                .x = p_ctransform->position.x + p_bbox->half_size.x,
                .y = p_ctransform->position.y + p_bbox->half_size.y,
            };
            unsigned int tile_idx = get_tile_idx(
                player_center.x,
                player_center.y,
                data->tilemap.width
            );
            if (data->tilemap.tiles[tile_idx].water_level > 0)
            {
                uint32_t water_height = data->tilemap.tiles[tile_idx].water_level * WATER_BBOX_STEP;
                Rectangle box = {
                    .x = (tile_idx % data->tilemap.width) * data->tilemap.tile_size,
                    .y = (tile_idx / data->tilemap.width + 1) * data->tilemap.tile_size - water_height,
                    .width = data->tilemap.tile_size,
                    .height = water_height,
                };

                in_water = point_in_AABB(player_center, box);
            }
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

        if (p_mstate->ground_state == 0b10)
        {
            p_ctransform->active = true;
        }
    }
}

void update_tilemap_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    CTileCoord_t* p_tilecoord;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTILECOORD_COMP_T], ent_idx, p_tilecoord)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
        if (p_ctransform == NULL) continue;
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);

        // Update tilemap position
        for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
        {
            // Use previously store tile position
            // Clear from those positions
            unsigned int tile_idx = p_tilecoord->tiles[i];
            sc_map_del_64v(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id);
        }
        p_tilecoord->n_tiles = 0;

        // Compute new occupied tile positions and add
        // Extend the check by a little to avoid missing
        unsigned int tile_x1 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ctransform->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ctransform->position.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ctransform->position.y) / TILE_SIZE;
        if (p_bbox != NULL)
        {
            tile_x2 = (p_ctransform->position.x + p_bbox->size.x - 1) / TILE_SIZE;
            tile_y2 = (p_ctransform->position.y + p_bbox->size.y - 1) / TILE_SIZE;
        }

        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                p_tilecoord->tiles[p_tilecoord->n_tiles++] = tile_idx;
                sc_map_put_64v(&(tilemap.tiles[tile_idx].entities_set), p_ent->m_id, (void *)p_ent);
            }
        }
    }
}

void hitbox_update_system(Scene_t* scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};

    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    unsigned int ent_idx;
    CHitBoxes_t* p_hitbox;
    sc_map_foreach(&scene->ent_manager.component_map[CHITBOXES_T], ent_idx, p_hitbox)
    {
        bool hit = false;
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;
        CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);

        memset(checked_entities, 0, sizeof(checked_entities));
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
                    if (tile_idx >= tilemap.n_tiles) break;
                    unsigned int other_ent_idx;
                    Entity_t* p_other_ent;
                    Vector2 overlap;

                    if (tilemap.tiles[tile_idx].tile_type != EMPTY_TILE)
                    {
                        Vector2 tile_pos = {tile_x * TILE_SIZE, tile_y * TILE_SIZE};
                        tile_pos = Vector2Add(tile_pos, tilemap.tiles[tile_idx].offset);
                        if (
                            find_AABB_overlap(
                                hitbox_pos, (Vector2){p_hitbox->boxes[i].width, p_hitbox->boxes[i].height},
                                tile_pos, tilemap.tiles[tile_idx].size, &overlap
                            )
                        )
                        {
                            hit = true;
                            if (p_hitbox->atk > tilemap.tiles[tile_idx].def)
                            {
                                change_a_tile(&tilemap, tile_idx, EMPTY_TILE);
                                continue;
                            }
                        }
                    }

                    sc_map_foreach(&tilemap.tiles[tile_idx].entities_set, other_ent_idx, p_other_ent)
                    {
                        if (other_ent_idx == ent_idx) continue;
                        if (checked_entities[other_ent_idx]) continue;

                        Entity_t* p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
                        if (!p_other_ent->m_alive) continue; // To only allow one way collision check
                        //if (p_other_ent->m_tag < p_ent->m_tag) continue; // To only allow one way collision check

                        CHurtbox_t* p_other_hurtbox = get_component(p_other_ent, CHURTBOX_T);
                        if (p_other_hurtbox == NULL) continue;
                        CTransform_t* p_other_ct = get_component(p_other_ent, CTRANSFORM_COMP_T);
                        Vector2 hurtbox_pos = Vector2Add(p_other_ct->position, p_other_hurtbox->offset);

                        if (
                            find_AABB_overlap(
                                hitbox_pos, (Vector2){p_hitbox->boxes[i].width, p_hitbox->boxes[i].height},
                                hurtbox_pos, p_other_hurtbox->size, &overlap
                            )
                        )
                        {
                            hit = true;
                            if (p_hitbox->atk > p_other_hurtbox->def)
                            {
                                p_other_hurtbox->damage_src = ent_idx;
                                if (p_other_ent->m_tag == CRATES_ENT_TAG)
                                {

                                    CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
                                    CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
                                    if (
                                        p_pstate != NULL
                                        && p_ctransform->position.y + p_bbox->size.y <= p_other_ct->position.y
                                    )
                                    {
                                        p_ctransform->velocity.y = -400;
                                        if (p_pstate->jump_pressed)
                                        {
                                            p_ctransform->velocity.y = -600;
                                            CJump_t * p_cjump = get_component(p_ent, CJUMP_COMP_T);
                                            p_cjump->short_hop = false;
                                            p_cjump->jumped = true;
                                        }
                                    }
                                }

                                if (p_ent->m_tag != PLAYER_ENT_TAG)
                                {
                                    remove_component(p_other_ent, CHURTBOX_T);
                                    CLifeTimer_t* p_clifetimer = add_component(p_other_ent, CLIFETIMER_T);
                                    p_clifetimer->life_time = 8;
                                }
                                else
                                {
                                    // Need to remove immediately, otherwise will interfere with bomb spawning
                                    remove_entity_from_tilemap(&scene->ent_manager, &tilemap, p_other_ent);
                                }
                            }

                        }
                    }
                }
            }
        }
        if (p_hitbox->one_hit && hit)
        {
            remove_entity_from_tilemap(&scene->ent_manager, &tilemap, p_ent);
        }
    }
}

void boulder_destroy_wooden_tile_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    Entity_t* p_boulder;
    sc_map_foreach_value(&scene->ent_manager.entities_map[BOULDER_ENT_TAG], p_boulder)
    {
        const CTransform_t* p_ctransform = get_component(p_boulder, CTRANSFORM_COMP_T);
        const CBBox_t* p_bbox = get_component(p_boulder, CBBOX_COMP_T);

        if (p_ctransform->velocity.y <= 0) continue;

        unsigned int tile_idx = get_tile_idx(
                p_ctransform->position.x + p_bbox->half_size.x,
                p_ctransform->position.y + p_bbox->size.y,
                tilemap.width
        );
        unsigned int tile_x = (p_ctransform->position.x + p_bbox->half_size.x) / TILE_SIZE;

        if (tilemap.tiles[tile_idx].tile_type == ONEWAY_TILE)
        {
            tilemap.tiles[tile_idx].tile_type = EMPTY_TILE;
            tilemap.tiles[tile_idx].solid = NOT_SOLID;
            tilemap.tiles[tile_idx].moveable = true;
            if (tile_x > 0 && tilemap.tiles[tile_idx - 1].tile_type == ONEWAY_TILE)
            {
                tilemap.tiles[tile_idx - 1].tile_type = EMPTY_TILE;
                tilemap.tiles[tile_idx - 1].solid = NOT_SOLID;
                tilemap.tiles[tile_idx - 1].moveable = true;
            }
            if (tile_x < tilemap.width && tilemap.tiles[tile_idx + 1].tile_type == ONEWAY_TILE)
            {
                tilemap.tiles[tile_idx + 1].tile_type = EMPTY_TILE;
                tilemap.tiles[tile_idx + 1].solid = NOT_SOLID;
                tilemap.tiles[tile_idx + 1].moveable = true;
            }
        }
    }
}

void container_destroy_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CContainer_t* p_container;
    sc_map_foreach(&scene->ent_manager.component_map[CCONTAINER_T], ent_idx, p_container)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive)
        {

            Entity_t* dmg_src = NULL;
            CHurtbox_t* p_hurtbox = get_component(p_ent, CHURTBOX_T);
            if(p_hurtbox != NULL)
            {
                dmg_src = get_entity(&scene->ent_manager, p_hurtbox->damage_src);
            }

            Entity_t* new_ent;
            switch (p_container->item)
            {
                case CONTAINER_LEFT_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, &scene->engine->assets, 0);
                break;
                case CONTAINER_RIGHT_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, &scene->engine->assets, 1);
                break;
                case CONTAINER_UP_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, &scene->engine->assets, 2);
                break;
                case CONTAINER_DOWN_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, &scene->engine->assets, 3);
                break;
                case CONTAINER_BOMB:
                    if (dmg_src != NULL && dmg_src->m_tag == PLAYER_ENT_TAG)
                    {
                        const CTransform_t* p_ctransform = get_component(p_ent, CTRANSFORM_COMP_T);
                        const CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
                        const CTransform_t* dmg_src_ctransform = get_component(dmg_src, CTRANSFORM_COMP_T);
                        const CBBox_t* dmg_src_bbox = get_component(dmg_src, CBBOX_COMP_T);
                        Vector2 launch_dir = {0, -1};
                        if (dmg_src_ctransform->position.x + dmg_src_bbox->size.x <= p_ctransform->position.x)
                        {
                            launch_dir.x = 1;
                        }
                        else if (dmg_src_ctransform->position.x >= p_ctransform->position.x + p_bbox->size.x)
                        {
                            launch_dir.x = -1;
                        }
                        new_ent = create_bomb(&scene->ent_manager, &scene->engine->assets, launch_dir);
                    }
                    else
                    {
                        new_ent = create_explosion(&scene->ent_manager, &scene->engine->assets);
                    }
                break;
                case CONTAINER_EXPLOSION:
                    new_ent = create_explosion(&scene->ent_manager, &scene->engine->assets);
                break;
                default:
                    new_ent = NULL;
                break;
            }
            if (new_ent != NULL)
            {
                CTransform_t* new_p_ct = get_component(new_ent, CTRANSFORM_COMP_T);
                CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_COMP_T);
                new_p_ct->position = Vector2Add(new_p_ct->position, p_ct->position);
            }
        }
    }
}

void lifetimer_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    unsigned int ent_idx;
    CLifeTimer_t* p_lifetimer;
    sc_map_foreach(&scene->ent_manager.component_map[CLIFETIMER_T], ent_idx, p_lifetimer)
    {
        p_lifetimer->life_time--;
        if (p_lifetimer->life_time == 0)
        {
            remove_entity_from_tilemap(&scene->ent_manager, &tilemap, get_entity(&scene->ent_manager, ent_idx));
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
        unsigned int next_idx = p_cspr->current_idx;
        if (p_cspr->transition_func != NULL)
        {
            next_idx = p_cspr->transition_func(p_ent);
        }
        if (p_cspr->pause) return;

        bool reset = p_cspr->current_idx != next_idx;
        p_cspr->current_idx = next_idx;

        SpriteRenderInfo_t spr = p_cspr->sprites[p_cspr->current_idx];
        if (spr.sprite == NULL) continue;

        if (reset) spr.sprite->current_frame = 0;

        // Animate it (handle frame count)
        spr.sprite->elapsed++;
        if (spr.sprite->elapsed == spr.sprite->speed)
        {
            spr.sprite->current_frame++;
            spr.sprite->current_frame %= spr.sprite->frame_count;
            spr.sprite->elapsed = 0;
        }
    }
}

void camera_update_system(Scene_t* scene)
{
    LevelScene_t* lvl_scene = CONTAINER_OF(scene, LevelScene_t, scene);
    Entity_t* p_player;
    const int width = lvl_scene->data.game_rec.width;
    const int height = lvl_scene->data.game_rec.height;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        lvl_scene->data.cam.offset = (Vector2){ width/2.0f, height/2.0f };
        lvl_scene->data.cam.target = p_ctransform->position;
    }
    Vector2 max = GetWorldToScreen2D(
        (Vector2){
            lvl_scene->data.tilemap.width * TILE_SIZE,
            lvl_scene->data.tilemap.height * TILE_SIZE
        },
        lvl_scene->data.cam
    );
    Vector2 min = GetWorldToScreen2D((Vector2){0, 0}, lvl_scene->data.cam);

    if (max.x < width) lvl_scene->data.cam.offset.x = width - (max.x - width/2.0f);
    if (max.y < height) lvl_scene->data.cam.offset.y = height - (max.y - height/2.0f);
    if (min.x > 0) lvl_scene->data.cam.offset.x = width/2.0f - min.x;
    if (min.y > 0) lvl_scene->data.cam.offset.y = height/2.0f - min.y;
}

void init_level_scene_data(LevelSceneData_t* data)
{
    data->game_viewport = LoadRenderTexture(VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE);
    data->game_rec = (Rectangle){25, 25, VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE};
    data->cam = (Camera2D){0};
    data->cam.rotation = 0.0f;
    data->cam.zoom = 1.0f;
}

void term_level_scene_data(LevelSceneData_t* data)
{
    UnloadRenderTexture(data->game_viewport); // Unload render texture
}
