#include "game_systems.h"

#include "scene_impl.h"
#include "ent_impl.h"

#include "particle_sys.h"
#include "AABB.h"
#include "EC.h"

#include "constants.h"
#include <stdio.h>

void simple_particle_system_update(Particle_t* part, void* user_data, float delta_time);
void floating_particle_system_update(Particle_t* part, void* user_data, float delta_time);
bool check_in_water(const ParticleEmitter_t* emitter, float delta_time);

static const Vector2 GRAVITY = {0, GRAV_ACCEL};
static const Vector2 UPTHRUST = {0, -GRAV_ACCEL * 1.15};

static inline unsigned int get_tile_idx(int x, int y, TileGrid_t gridmap)
{
    unsigned int tile_x = x / gridmap.tile_size;
    unsigned int tile_y = y / gridmap.tile_size;
    return tile_y * gridmap.width + tile_x;
}

static inline void destroy_tile(LevelSceneData_t* lvl_data, unsigned int tile_idx)
{
    Scene_t* scene = &CONTAINER_OF(lvl_data, LevelScene_t, data)->scene;
    TileGrid_t tilemap = lvl_data->tilemap;

    Sprite_t* spr = NULL;
    switch (tilemap.tiles[tile_idx].tile_type)
    {
        case LADDER:
            spr = get_sprite(&scene->engine->assets, "p_ladder");
        break;
        case ONEWAY_TILE:
            spr = get_sprite(&scene->engine->assets, "p_wood");
        break;
        case SPIKES:
            spr = get_sprite(&scene->engine->assets, "p_spike");
        break;
        default:
        break;
    }


    if (spr != NULL)
    {
        ParticleEmitter_t emitter = {
            .spr = spr,
            .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
            .position = {
                .x = tile_idx % tilemap.width * tilemap.tile_size + (tilemap.tile_size >> 1),
                .y = tile_idx / tilemap.width * tilemap.tile_size + (tilemap.tile_size >> 1),
            },
            .n_particles = 5,
            .user_data = CONTAINER_OF(lvl_data, LevelScene_t, data),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter);
    }

    change_a_tile(&tilemap, tile_idx, EMPTY_TILE);
}

// ------------------------- Collision functions ------------------------------------
// Do not subtract one for the size for any collision check, just pass normally. The extra one is important for AABB test

static uint8_t check_collision_and_move(
    TileGrid_t* tilemap,
    Entity_t* ent, Vector2* other_pos, Vector2 other_bbox,
    SolidType_t other_solid
)
{
    // TODO: Need some NULL checks to be more robust
    CTransform_t* p_ct = get_component(ent, CTRANSFORM_COMP_T);
    CBBox_t* p_bbox = get_component(ent, CBBOX_COMP_T);
    Vector2 overlap = {0,0};
    Vector2 prev_overlap = {0,0};
    uint8_t overlap_mode = find_AABB_overlap(ent->position, p_bbox->size, *other_pos, other_bbox, &overlap);
    uint8_t collided_side = 0;
    if (overlap_mode == 1)
    {
        // If there is collision, use previous overlap to determine direction
        find_AABB_overlap(p_ct->prev_position, p_bbox->size, *other_pos, other_bbox, &prev_overlap);

        // Resolve collision via moving player by the overlap amount only if other is solid
        // also check for empty to prevent creating new collision. Not fool-proof, but good enough
        Vector2 offset = {0, 0};
        if (other_solid == SOLID)
        {
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
        }

        if (offset.y < 0)
        {
            collided_side |= 1;
        }
        else if (offset.y > 0)
        {
            collided_side |= (1<<1);
        }
        else if (offset.x < 0)
        {
            collided_side |= (1<<2);
        }
        else if (offset.x > 0)
        {
            collided_side |= (1<<3);
        }

        ent->position = Vector2Add(ent->position, offset);
    }
    else if (overlap_mode == 2)
    {
        if ( other_solid != SOLID ) goto collision_end;
        // On complete overlap, find a free space in this order: top, left, right, bottom
        Vector2 point_to_test = {0};
        point_to_test.x = ent->position.x;
        point_to_test.y = other_pos->y - p_bbox->size.y + 1;
        if (!check_collision_at(ent, point_to_test, p_bbox->size, tilemap))
        {
            ent->position = point_to_test;
            collided_side |= 1;
            goto collision_end;
        }

        point_to_test.x = other_pos->x - p_bbox->size.x + 1;
        point_to_test.y = ent->position.y;
        if (!check_collision_at(ent, point_to_test, p_bbox->size, tilemap))
        {
            ent->position = point_to_test;
            collided_side |= (1<<2);
            goto collision_end;
        }

        point_to_test.x = other_pos->x + other_bbox.x - 1;
        point_to_test.y = ent->position.y;
        if (!check_collision_at(ent, point_to_test, p_bbox->size, tilemap))
        {
            ent->position = point_to_test;
            collided_side |= (1<<3);
            goto collision_end;
        }

        point_to_test.x = ent->position.x;
        point_to_test.y = other_pos->y + other_bbox.y - 1;
        if (!check_collision_at(ent, point_to_test, p_bbox->size, tilemap))
        {
            ent->position = point_to_test;
            collided_side |= (1<<1);
            goto collision_end;
        }
        // If no free space, Move up no matter what
        //p_ct->position.x = p_ct->position.x;
        ent->position.y = other_pos->y - p_bbox->size.y + 1;
    }
collision_end:
    return collided_side;
}

static void destroy_entity(Scene_t* scene, TileGrid_t* tilemap, Entity_t* p_ent)
{
    Vector2 half_size = {0,0};
    CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
    if (p_bbox != NULL)
    {
        half_size = p_bbox->half_size;
    }

    if (p_ent->m_tag == BOULDER_ENT_TAG)
    {
        ParticleEmitter_t emitter = {
            .spr = get_sprite(&scene->engine->assets, "p_rock"),
            .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
            .position = Vector2Add(p_ent->position, half_size),
            .n_particles = 5,
            .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter);
    }
    else if (p_ent->m_tag == CRATES_ENT_TAG)
    {
        const CContainer_t* p_container = get_component(p_ent, CCONTAINER_T);
        ParticleEmitter_t emitter = {
            .spr = get_sprite(&scene->engine->assets, (p_container->material == WOODEN_CONTAINER) ? "p_wood" : "p_metal"),
            .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
            .position = Vector2Add(p_ent->position, half_size),
            .n_particles = 5,
            .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter);
    }
    else if (p_ent->m_tag == CHEST_ENT_TAG)
    {
        ParticleEmitter_t emitter = {
            .spr = get_sprite(&scene->engine->assets, "p_wood"),
            .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
            .position = Vector2Add(p_ent->position, half_size),
            .n_particles = 5,
            .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter);

        ParticleEmitter_t emitter2 = {
            .spr = get_sprite(&scene->engine->assets, "p_coin"),
            .config = get_emitter_conf(&scene->engine->assets, "pe_single"),
            .position = Vector2Add(p_ent->position, half_size),
            .n_particles = 1,
            .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter2);

        play_sfx(scene->engine, COIN_SFX);
    }
    else if (p_ent->m_tag == ARROW_ENT_TAG)
    {
        ParticleEmitter_t emitter = {
            .spr = get_sprite(&scene->engine->assets, "p_arrow"),
            .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
            .position = Vector2Add(p_ent->position, half_size),
            .n_particles = 2,
            .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
            .update_func = &simple_particle_system_update,
            .emitter_update_func = NULL,
        };
        play_particle_emitter(&scene->part_sys, &emitter);
        play_sfx(scene->engine, ARROW_DESTROY_SFX);
    }

    clear_an_entity(scene, tilemap, p_ent);
}

void check_player_dead_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    Entity_t* p_player;
    // Cannot create player while looping though the players
    // So have to create outside of the loop
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        if (!p_player->m_alive)
        {
            Entity_t* ent = create_dead_player(&scene->ent_manager);
            if (ent != NULL)
            {
                ent->position = p_player->position;
            }
            destroy_entity(scene, &data->tilemap, p_player);
            change_level_state(data, LEVEL_STATE_DEAD);
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
            // Transit into ladder state if possible
            if (!p_pstate->locked)
            {
                if (p_pstate->player_dir.y < 0)
                {
                    unsigned int tile_idx = get_tile_idx(
                        p_player->position.x + p_bbox->half_size.x,
                        p_player->position.y + p_bbox->half_size.y,
                        data->tilemap
                    );
                    if (tilemap.tiles[tile_idx].tile_type == LADDER && p_ctransform->velocity.y >= 0)
                    {
                        p_pstate->ladder_state = true;
                        p_player->position.y--;
                    }
                }
                else if (p_pstate->player_dir.y > 0)
                {
                    unsigned int tile_idx;

                    if (p_mstate->ground_state & 1)
                    {
                        tile_idx = get_tile_idx(
                            p_player->position.x + p_bbox->half_size.x,
                            p_player->position.y + p_bbox->size.y,
                            data->tilemap
                        );
                    }
                    else
                    {
                        tile_idx = get_tile_idx(
                            p_player->position.x + p_bbox->half_size.x,
                            p_player->position.y + p_bbox->half_size.y,
                            data->tilemap
                        );
                    }
                    if (tile_idx < tilemap.n_tiles && tilemap.tiles[tile_idx].tile_type == LADDER)
                    {
                        p_pstate->ladder_state = true;
                        p_player->position.y++;
                    }
                }
            }
        }
        else
        {
            unsigned int tile_x =  (p_player->position.x + p_bbox->half_size.x) / TILE_SIZE;
            unsigned int tile_y1 = (p_player->position.y + p_bbox->half_size.y) / TILE_SIZE;
            unsigned int tile_y2 = (p_player->position.y + p_bbox->size.y) / TILE_SIZE;
            
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
                p_ctransform->velocity.y = p_pstate->player_dir.y * 150 * (p_pstate->locked ? 0 : 1);
                p_ctransform->velocity.x = p_pstate->player_dir.x * 40 * (p_pstate->locked ? 0 : 1);
                if (p_pstate->player_dir.y != 0)
                {
                    p_player->position.x = tile_x * tilemap.tile_size;
                    p_player->position.x += (tilemap.tile_size >> 1);
                    p_player->position.x -= p_bbox->half_size.x;
                }
            }
        }

        bool in_water = (p_mstate->water_state & 1);
        if (p_pstate->locked)
        {
            p_pstate->is_crouch |= (p_pstate->is_crouch & 1)? 0b10 : 0;
        }
        else
        {
            p_pstate->is_crouch |= (p_pstate->player_dir.y > 0)? 0b10 : 0;
            Vector2 point_to_check = Vector2Add(
                p_player->position,
                (Vector2){0, p_bbox->size.y - PLAYER_HEIGHT}
            );
            uint8_t collide_type = check_collision_at(
                    p_player, point_to_check, p_bbox->size, &tilemap
            );
            if (collide_type == 1)
            {
                p_pstate->is_crouch |= 0b10;
            }
            if (!(p_mstate->ground_state & 1)) p_pstate->is_crouch &= 0b01;
        }

        if (!in_water)
        {
            p_pstate->player_dir.y = 0;
        }

        if (!p_pstate->locked)
        {
                // Although this can be achieved via higher friction, i'll explain away as the player is not
                // good with swimming, resulting in lower movement acceleration
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL / (in_water ? 1.2f : 1.0f));

            if (p_pstate->is_crouch & 1)
            {
                p_ctransform->accel = Vector2Scale(p_ctransform->accel, 0.5);
            }
        }

        // Short Hop
        // Jumped check is needed to make sure it is applied on jumps, not generally
        // One issue caused is lower velocity in water
        if (p_cjump->jumped)
        {
            if (!p_pstate->jump_pressed || p_pstate->locked)
            {
                p_cjump->jump_released = true;
                if (!p_cjump->short_hop && p_ctransform->velocity.y < 0)
                {
                    p_cjump->short_hop = true;
                    p_ctransform->velocity.y /= 2;
                }
            }
        }


        // Jumps is possible as long as you have a jump

        // Check if possible to jump when jump is pressed
        if (!p_pstate->locked)
        {
            if (p_cjump->jump_released && p_pstate->jump_pressed && p_cjump->jumps > 0 && p_cjump->jump_ready)
            {
                play_sfx(scene->engine, PLAYER_JMP_SFX);
                p_cjump->jumps--;
                if (!in_water)
                {
                    if (p_mstate->ground_state & 1 || p_cjump->coyote_timer > 0)
                    {
                        p_ctransform->velocity.y = -p_cjump->jump_speed;
                    }
                    else if (p_pstate->ladder_state)
                    {
                        p_ctransform->velocity.y = -p_cjump->jump_speed / 1.4;
                    }
                }
                else
                {
                    p_ctransform->velocity.y = -p_cjump->jump_speed / 1.75;
                }

                p_pstate->ladder_state = false;
                p_cjump->coyote_timer = 0;
                p_cjump->jumped = true;
                p_cjump->jump_ready = false;
                p_cjump->jump_released = false;
            }
            else if (p_mstate->ground_state == 0b01)
            {
                // the else if check is to prevent playing the landing sfx
                // on first frame jumps
                // This is also why this is done here instead of in the 
                // state transition function
                play_sfx_pitched(scene->engine, PLAYER_LAND_SFX, 0.8f + rand() * 0.4f / (float)RAND_MAX);
            }
        }

        // Post movement state update
        p_pstate->is_crouch >>= 1;
    }
}

void player_bbox_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
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

        Vector2 point_to_check = Vector2Add(p_player->position, offset);
        if (
            check_collision_at(
                p_player, point_to_check, new_bbox, &tilemap
            ) != 1
        )
        {
            set_bbox(p_bbox, new_bbox.x, new_bbox.y);
            p_player->position = Vector2Add(p_player->position, offset);
        }

        CHitBoxes_t* p_hitbox = get_component(p_player, CHITBOXES_T);
        p_hitbox->boxes[0].height = p_bbox->size.y + 4;
        p_hitbox->boxes[1].height = p_bbox->size.y;
        CHurtbox_t* p_hurtbox = get_component(p_player, CHURTBOX_T);

        if ((p_mstate->water_state & 1) && !(p_mstate->ground_state & 1))
        {
            p_hurtbox->size = p_bbox->size;
            p_hurtbox->size.x *= 1.7;
            p_hitbox->boxes[0].width = p_bbox->size.x * 2.0f;
            p_hitbox->boxes[1].width = p_bbox->size.x * 2.0f + 4;
            if (p_mstate->x_dir > 0)
            {
                p_hitbox->boxes[0].x = -p_bbox->size.x;
                p_hitbox->boxes[1].x = -p_bbox->size.x - 2;
                p_hurtbox->offset.x = -p_bbox->size.x * 0.7;
            }
            else
            {
                p_hitbox->boxes[0].x = 0;
                p_hitbox->boxes[1].x = -2;
                p_hurtbox->offset.x = 0;
            }
        }
        else
        {
            p_hitbox->boxes[0].x = 0;
            p_hitbox->boxes[0].width = p_bbox->size.x;
            p_hitbox->boxes[1].x = -2;
            p_hitbox->boxes[1].width = p_bbox->size.x + 4;
            p_hurtbox->offset = (Vector2){0,0};
            p_hurtbox->size = p_bbox->size;
        }
    }
}

void player_crushing_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);

        uint8_t edges = check_bbox_edges(
            &data->tilemap, p_player,
            p_bbox->size, true
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
                .bbox = (Rectangle){p_player->position.x, p_player->position.y, p_bbox->size.x, 1},
                .prev_bbox = (Rectangle){p_player->position.x, p_player->position.y, p_bbox->size.x, p_bbox->size.y},
                .area = (TileArea_t){
                    .tile_x1 = (p_player->position.x) / TILE_SIZE,
                    .tile_y1 = (p_player->position.y) / TILE_SIZE,
                    .tile_x2 = (p_player->position.x + p_bbox->size.x - 1) / TILE_SIZE,
                    .tile_y2 = (p_player->position.y + p_bbox->size.y - 1) / TILE_SIZE,
                }
            };
            
            
            uint8_t collide_type = check_collision_line(&ent, &data->tilemap, false);
            if (collide_type == 1)
            {
                collide |= 1 << 1;
            }
            
            ent.bbox.y = p_player->position.y + p_bbox->size.y;
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
                .bbox = (Rectangle){p_player->position.x, p_player->position.y, 1, p_bbox->size.y},
                .prev_bbox = (Rectangle){p_player->position.x, p_player->position.y, p_bbox->size.x, p_bbox->size.y},
                .area = (TileArea_t){
                    .tile_x1 = (p_player->position.x) / TILE_SIZE,
                    .tile_y1 = (p_player->position.y) / TILE_SIZE,
                    .tile_x2 = (p_player->position.x + p_bbox->size.x - 1) / TILE_SIZE,
                    .tile_y2 = (p_player->position.y + p_bbox->size.y - 1) / TILE_SIZE,
                }
            };

            // Left
            uint8_t collide_type = check_collision_line(&ent, &data->tilemap, false);
            if (collide_type == 1)
            {
                collide |= 1 << 1;
            }

            //Right
            ent.bbox.x = p_player->position.x + p_bbox->size.x; // 2 to account for the previous subtraction
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
        unsigned int tile_x1 = (p_ent->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ent->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ent->position.x + p_bbox->size.x - 1) / TILE_SIZE;
        unsigned int tile_y2 = (p_ent->position.y + p_bbox->size.y - 1) / TILE_SIZE;

        tile_x1 = (tile_x1 < 0) ? 0 : tile_x1;
        tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
        tile_y1 = (tile_y1 < 0) ? 0 : tile_y1;
        tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.height - 1 : tile_y2;

        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                if(tilemap.tiles[tile_idx].tile_type == SPIKES)
                {
                    uint8_t collide = find_AABB_overlap(
                        p_ent->position, p_bbox->size, 
                        (Vector2){
                            tile_x * TILE_SIZE + tilemap.tiles[tile_idx].offset.x,
                            tile_y * TILE_SIZE + tilemap.tiles[tile_idx].offset.y
                        },
                    tilemap.tiles[tile_idx].size,
                    NULL);

                    if (collide)
                    {
                        if (p_ent->m_tag == PLAYER_ENT_TAG)
                        {
                            p_ent->m_alive = false;
                            return;
                        }
                        else
                        {
                            destroy_tile(data, tile_idx);
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
        unsigned int tile_x1 = (p_ent->position.x - 1) / TILE_SIZE;
        unsigned int tile_y1 = (p_ent->position.y - 1) / TILE_SIZE;
        unsigned int tile_x2 = (p_ent->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ent->position.y + p_bbox->size.y) / TILE_SIZE;

        tile_x1 = (tile_x1 < 0) ? 0 : tile_x1;
        tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
        tile_y1 = (tile_y1 < 0) ? 0 : tile_y1;
        tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.height - 1 : tile_y2;

        uint8_t collide_side = 0;
        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                if(tilemap.tiles[tile_idx].tile_type != EMPTY_TILE)
                {
                    Vector2 other;
                    other.x =
                        (tile_idx % tilemap.width) * tilemap.tile_size
                        + tilemap.tiles[tile_idx].offset.x;
                    other.y =
                        (tile_idx / tilemap.width) * tilemap.tile_size
                        + tilemap.tiles[tile_idx].offset.y; // Precision loss is intentional

                    
                    SolidType_t solid = tilemap.tiles[tile_idx].solid;
                    // One way collision is a bit special
                    if (solid == ONE_WAY)
                    {
                        solid = (
                            p_ctransform->prev_position.y + p_bbox->size.y <= other.y
                            && p_ent->position.y + p_bbox->size.y > other.y
                        ) ? SOLID : NOT_SOLID;
                    }
                    collide_side |= check_collision_and_move(
                        &tilemap, p_ent,
                        &other, 
                        (Vector2){tilemap.tile_size, tilemap.tile_size},
                        solid
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

                        SolidType_t solid = p_other_bbox->solid? SOLID : NOT_SOLID;
                        if (p_ent->m_tag == PLAYER_ENT_TAG && p_other_ent->m_tag == CHEST_ENT_TAG)
                        {
                            solid = NOT_SOLID;
                        }
                        collide_side |= check_collision_and_move(
                            &tilemap, p_ent,
                            &p_other_ent->position, p_other_bbox->size,
                            solid
                        );
                    }
                }


            }
        }

        if (collide_side & (1<<3))
        {
            if (p_ctransform->velocity.x < 0) p_ctransform->velocity.x = 0;
        }
        if (collide_side & (1<<2))
        {
            if (p_ctransform->velocity.x > 0) p_ctransform->velocity.x = 0;
        }
        if (collide_side & (1<<1))
        {
            if (p_ctransform->velocity.y < 0) p_ctransform->velocity.y = 0;
        }
        if (collide_side & (1))
        {
            if (p_ctransform->velocity.y > 0) p_ctransform->velocity.y = 0;
        }

        float decimal;
        float fractional = modff(p_ent->position.x, &decimal);
        if (fractional > 0.99)
        {
            p_ent->position.x = decimal;
            (p_ent->position.x > 0) ? p_ent->position.x++ : p_ent->position.x--;
        }
        else if (fractional < 0.01)
        {
            p_ent->position.x = decimal;
        }

        fractional = modff(p_ent->position.y, &decimal);
        if (fractional > 0.99)
        {
            p_ent->position.y = decimal;
            (p_ent->position.y > 0) ? p_ent->position.y++ : p_ent->position.y--;
        }
        else if (fractional < 0.01)
        {
            p_ent->position.y = decimal;
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
        if (p_mstate != NULL)
        {
            // Apply water friction
            // Consistent in all direction
            p_ct->fric_coeff = Vector2Add(
                Vector2Scale(
                    (Vector2){-WATER_FRICTION, -WATER_FRICTION},
                    p_mstate->water_overlap
                ),
                Vector2Scale(
                    (Vector2){-GROUND_X_FRICTION, -GROUND_Y_FRICTION},
                    (1 - p_mstate->water_overlap)
                )
            );
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
        if (p_pstate != NULL)
        {
            if (p_pstate->is_crouch & 1) p_ct->fric_coeff.x -= 4;
            if ((p_mstate->ground_state & 1) && p_pstate->player_dir.x != 0) p_ct->fric_coeff.x *= 0.9;
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

        if (p_ctransform->grav_timer > 0.0f)
        {
            p_ctransform->grav_timer -= scene->delta_time;
            continue;
        }

        if (!(p_mstate->ground_state & 1))
        {
            p_ctransform->accel = Vector2Multiply(
                Vector2Add(
                    p_ctransform->accel,
                    Vector2Scale(UPTHRUST, p_mstate->water_overlap)
                ),
                p_ctransform->shape_factor
            );

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
            p_bbox->size, false
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
            float remaining_distance = p_moveable->target_pos.x - p_ent->position.x;
            if (fabs(remaining_distance) < 0.1)
            {
                p_ctransform->prev_position = p_moveable->prev_pos;
                p_ent->position = p_moveable->target_pos;
                p_moveable->gridmove = false;
                p_bbox->solid = true;
                p_ctransform->movement_mode = REGULAR_MOVEMENT;
                play_sfx(scene->engine, BOULDER_LAND_SFX);
            }
            else if (remaining_distance > 0.1)
            {
                float x_to_move = p_moveable->move_speed * scene->delta_time;
                p_ent->position.x +=  (remaining_distance > x_to_move) ? x_to_move : remaining_distance;
            }
            else
            {
                float x_to_move = p_moveable->move_speed * scene->delta_time;
                p_ent->position.x +=  (remaining_distance < -x_to_move) ? -x_to_move : remaining_distance;
                memset(&p_ctransform->velocity, 0, sizeof(p_ctransform->velocity));
            }
        }

        // Intentional. Want this check even after a gridmove to allow gridmove after that
        if (!p_moveable->gridmove)
        {
            //if (p_ctransform->prev_velocity.y <= 0 && p_ctransform->prev_position.x == p_ctransform->position.x) continue;

            TileGrid_t tilemap = (CONTAINER_OF(scene, LevelScene_t, scene)->data).tilemap;

            // Point to check is the one row below
            Vector2 point_to_check = {
                .x = p_ent->position.x + p_bbox->half_size.x,
                .y = p_ent->position.y + p_bbox->size.y + 1
            };
            unsigned int tile_x = point_to_check.x / TILE_SIZE;
            unsigned int tile_y = point_to_check.y / TILE_SIZE;
            if (tile_y >= tilemap.height) continue;

            int tile_idx = tile_y * tilemap.width + tile_x;
            unsigned int other_ent_idx;
            bool can_move = false;
            sc_map_foreach_key(&tilemap.tiles[tile_idx].entities_set, other_ent_idx)
            {
                if (other_ent_idx == ent_idx) continue;
                sc_map_get_64v(&scene->ent_manager.component_map[CMOVEABLE_T], other_ent_idx);
                if (!sc_map_found(&scene->ent_manager.component_map[CMOVEABLE_T])) continue;

                {
                    Entity_t* other_ent = get_entity(&scene->ent_manager, other_ent_idx);
                    CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                    CTransform_t* p_other_ct = get_component(other_ent, CTRANSFORM_COMP_T);
                    Rectangle box = {other_ent->position.x, other_ent->position.y, p_other_bbox->size.x, p_other_bbox->size.y};
                    if (!point_in_AABB(point_to_check, box) || Vector2LengthSqr(p_other_ct->velocity) != 0) continue;
                }

                tile_x = (p_ent->position.x) / TILE_SIZE - 1;

                if (tile_x >= 0 && tile_x < tilemap.width)
                {
                    unsigned int tile_idx1 = tile_y * tilemap.width + tile_x;
                    unsigned int tile_idx2 = (tile_y - 1) * tilemap.width + tile_x;
                    if ( 
                        (tilemap.tiles[tile_idx1].moveable || tilemap.tiles[tile_idx1].tile_type == ONEWAY_TILE)
                        && tilemap.tiles[tile_idx2].moveable 
                    )
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
                                //any_solid |= true;
                                break;
                            }
                        }
                        sc_map_foreach_key(&tilemap.tiles[tile_idx2].entities_set, idx_to_check)
                        {
                            Entity_t* other_ent = get_entity(&scene->ent_manager, idx_to_check);
                            CBBox_t* p_other_bbox = get_component(other_ent, CBBOX_COMP_T);
                            if (p_other_bbox != NULL)
                            {
                                any_solid |= p_other_bbox->solid;
                                //any_solid |= true;
                                break;
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
                    unsigned int tile_idx2 = (tile_y - 1) * tilemap.width + tile_x;
                    if (
                        (tilemap.tiles[tile_idx1].moveable || tilemap.tiles[tile_idx1].tile_type == ONEWAY_TILE)
                        && tilemap.tiles[tile_idx2].moveable
                    )
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
                p_moveable->prev_pos = p_ent->position;
                p_moveable->target_pos = Vector2Scale((Vector2){tile_x,tile_y-1}, TILE_SIZE); 
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
        if (
            !(p_movement->ground_state & 1 || p_movement->water_state & 1)
            || p_pstate->player_dir.x == 0
        ) continue;

        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(p_player, CBBOX_COMP_T);
        Vector2 point_to_check = p_player->position;
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
                .x = p_other_ent->position.x,
                .y = p_other_ent->position.y,
                .width = p_other_bbox->size.x,
                .height = p_other_bbox->size.y
            };
            if (point_in_AABB(point_to_check, box))
            {
                Vector2 target_pos = p_other_ent->position;
                if (p_player->position.x < p_other_ent->position.x)
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
                        p_other_moveable->prev_pos = p_other_ent->position;
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
    float delta_time = scene->delta_time;
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

        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        // Store previous position before update
        p_ctransform->prev_position = p_ent->position;
        p_ent->position = Vector2Add(
            p_ent->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );
        memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));

        // Level boundary collision
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
        unsigned int level_width = tilemap.width * TILE_SIZE;
        unsigned int level_height = tilemap.height * TILE_SIZE;
        if (p_bbox != NULL)
        {

            if(p_ent->position.x < 0 || p_ent->position.x + p_bbox->size.x > level_width)
            {
                p_ent->position.x = (p_ent->position.x < 0) ? 0 : p_ent->position.x;
                if (p_ent->position.x + p_bbox->size.x > level_width)
                {
                    p_ent->position.x =  level_width - p_bbox->size.x;
                }
                else
                {
                    p_ent->position.x = p_ent->position.x;
                }
                p_ctransform->velocity.x = 0;
            }
            
            if(p_ent->position.y < 0 || p_ent->position.y + p_bbox->size.y > level_height)
            {
                p_ent->position.y = (p_ent->position.y < 0) ? 0 : p_ent->position.y;
                if (p_ent->position.y + p_bbox->size.y > level_height)
                {
                    p_ent->position.y = level_height - p_bbox->size.y;
                }
                else
                {
                    p_ent->position.y = p_ent->position.y;
                }
                p_ctransform->velocity.y = 0;
            }
        }
        else
        {
            if (
                p_ent->position.x < 0 || p_ent->position.x > level_width
                || p_ent->position.y < 0 || p_ent->position.y > level_height
            )
            {
                destroy_entity(scene, &tilemap, p_ent);
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
        bool jump_recover_cond = (p_mstate->ground_state & 1 || in_water || p_pstate->ladder_state);
        // Landing or in water
        if (p_mstate->water_state == 0b10 || p_mstate->ground_state == 0b10)
        {
            p_cjump->coyote_timer = 3;
        }
        else if (jump_recover_cond)
        {
            // Recover jumps
            p_cjump->jumps = p_cjump->max_jumps;
            p_cjump->jumped = false;
            if(!p_cjump->jump_released && !p_pstate->jump_pressed) p_cjump->jump_released = true;
            p_cjump->short_hop = false;
            p_cjump->jump_ready = true;
            p_cjump->coyote_timer = 0;
        }
        else
        {
            if (p_cjump->coyote_timer > 0)
            {
                p_cjump->coyote_timer--;
            }
            else
            {
                p_cjump->jumps -= (p_cjump->jumps > 0)? 1:0;
                if (p_mstate->ground_state & 1)
                {
                    p_cjump->jumps++;
                }
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

        if (p_ctransform->velocity.x > 0) p_mstate->x_dir = 1;
        else if (p_ctransform->velocity.x < 0) p_mstate->x_dir = 0;

        bool on_ground = check_on_ground(
            p_ent, p_ctransform->prev_position, p_bbox->size,
            &data->tilemap
        );

        if (on_ground)
        {
            p_ctransform->grav_timer = p_ctransform->grav_delay;
        }

        // Upthrust depends on water overlapping
        unsigned int tile_x1 = (p_ent->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ent->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ent->position.x + p_bbox->size.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ent->position.y + p_bbox->size.y) / TILE_SIZE;
        float water_area = 0;
        for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
        {
            for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
            {
                unsigned int tile_idx = tile_y * data->tilemap.width + tile_x;
                if (data->tilemap.tiles[tile_idx].water_level > 0)
                {
                    uint32_t water_height = data->tilemap.tiles[tile_idx].water_level * WATER_BBOX_STEP;
                    Vector2 water_tl = {
                        .x = (tile_idx % data->tilemap.width) * data->tilemap.tile_size,
                        .y = (tile_idx / data->tilemap.width + 1) * data->tilemap.tile_size - water_height};

                    Vector2 water_sz = {
                        .x = data->tilemap.tile_size,
                        .y = water_height,
                    };
                    
                    Vector2 overlap;
                    if (find_AABB_overlap(
                        p_ent->position, p_bbox->size,
                        water_tl, water_sz, &overlap
                    ))
                    {
                        water_area += fabs(overlap.x * overlap.y);
                    }
                }
            }
        }
        p_mstate->water_overlap = water_area/(p_bbox->size.x * p_bbox->size.y);


        bool in_water = p_mstate->water_overlap > 0;
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
        if (p_mstate->ground_state == 0b01)
        {
            if (p_ent->m_tag == BOULDER_ENT_TAG)
            {
                play_sfx(scene->engine, BOULDER_LAND_SFX);
            }
            else if (p_ent->m_tag == CRATES_ENT_TAG || p_ent->m_tag == CHEST_ENT_TAG)
            {
                play_sfx(scene->engine, WOOD_LAND_SFX);
            }
        }
        if (p_mstate->water_state == 0b01)
        {
            play_sfx(scene->engine, WATER_IN_SFX);
            ParticleEmitter_t emitter = {
                .spr = get_sprite(&scene->engine->assets, "p_water"),
                .config = get_emitter_conf(&scene->engine->assets, "pe_burst"),
                .position = Vector2Add(p_ent->position, p_bbox->half_size),
                .n_particles = 5,
                .user_data = (CONTAINER_OF(scene, LevelScene_t, scene)),
                .update_func = &simple_particle_system_update,
                .emitter_update_func = NULL,
            };
            play_particle_emitter(&scene->part_sys, &emitter);

        }

    }
}

void update_entity_emitter_system(Scene_t* scene)
{
    CEmitter_t* p_emitter;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CEMITTER_T], ent_idx, p_emitter)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        Vector2 new_pos = Vector2Add(p_ent->position,p_emitter->offset);
        if (is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
        {
            update_emitter_handle_position(&scene->part_sys, p_emitter->handle, new_pos);
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
        if (!p_ent->m_alive) continue;

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
        unsigned int tile_x1 = (p_ent->position.x) / TILE_SIZE;
        unsigned int tile_y1 = (p_ent->position.y) / TILE_SIZE;
        unsigned int tile_x2 = (p_ent->position.x) / TILE_SIZE;
        unsigned int tile_y2 = (p_ent->position.y) / TILE_SIZE;
        if (p_bbox != NULL)
        {
            tile_x2 = (p_ent->position.x + p_bbox->size.x - 1) / TILE_SIZE;
            tile_y2 = (p_ent->position.y + p_bbox->size.y - 1) / TILE_SIZE;
        }
        tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
        tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.width - 1 : tile_y2;

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
                .x = p_ent->position.x + p_hitbox->boxes[i].x,
                .y = p_ent->position.y + p_hitbox->boxes[i].y,
            };

            unsigned int tile_x1 = (hitbox_pos.x) / TILE_SIZE;
            unsigned int tile_y1 = (hitbox_pos.y) / TILE_SIZE;
            unsigned int tile_x2 = (hitbox_pos.x + p_hitbox->boxes[i].width) / TILE_SIZE;
            unsigned int tile_y2 = (hitbox_pos.y + p_hitbox->boxes[i].height) / TILE_SIZE;
            tile_x2 = (tile_x2 >= tilemap.width) ? tilemap.width - 1 : tile_x2;
            tile_y2 = (tile_y2 >= tilemap.height) ? tilemap.width - 1 : tile_y2;

            for (unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
            {
                for (unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
                {
                    unsigned int tile_idx = tile_y * tilemap.width + tile_x;
                    if (tile_idx >= tilemap.n_tiles) break;
                    unsigned int other_ent_idx;
                    Entity_t* p_other_ent;

                    if (tilemap.tiles[tile_idx].tile_type != EMPTY_TILE)
                    {
                        Vector2 tile_pos = {tile_x * TILE_SIZE, tile_y * TILE_SIZE};
                        tile_pos = Vector2Add(tile_pos, tilemap.tiles[tile_idx].offset);
                        if (
                            find_AABB_overlap(
                                hitbox_pos, (Vector2){p_hitbox->boxes[i].width, p_hitbox->boxes[i].height},
                                tile_pos, tilemap.tiles[tile_idx].size, NULL
                            )
                        )
                        {
                            hit = true;
                            if (p_hitbox->atk > tilemap.tiles[tile_idx].def)
                            {
                                destroy_tile(data, tile_idx); 
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
                        CBBox_t* p_other_bbox = get_component(p_other_ent, CBBOX_COMP_T);
                        Vector2 hurtbox_pos = Vector2Add(p_other_ent->position, p_other_hurtbox->offset);

                        if (
                            find_AABB_overlap(
                                hitbox_pos, (Vector2){p_hitbox->boxes[i].width, p_hitbox->boxes[i].height},
                                hurtbox_pos, p_other_hurtbox->size, NULL
                            )
                        )
                        {
                            hit = true;

                            uint8_t full_atk = p_hitbox->atk;
                            if (p_ent->m_tag == PLAYER_ENT_TAG && p_other_ent->m_tag == CHEST_ENT_TAG)
                            {
                                full_atk = p_other_hurtbox->def + 1;
                            }
                            if (full_atk > p_other_hurtbox->def)
                            {
                                p_other_hurtbox->damage_src = ent_idx;
                                if (p_other_ent->m_tag == CRATES_ENT_TAG)
                                {

                                    CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
                                    CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
                                    if (p_pstate != NULL)
                                    {
                                        if (p_ent->position.y + p_bbox->size.y <= p_other_ent->position.y)
                                        {
                                            p_ctransform->velocity.y = -400;
                                            if (p_pstate->jump_pressed)
                                            {
                                                p_ctransform->velocity.y = -600;
                                                CJump_t * p_cjump = get_component(p_ent, CJUMP_COMP_T);
                                                p_cjump->short_hop = false;
                                                p_cjump->jumped = true;
                                            }
                                            if (p_ent->m_tag == PLAYER_ENT_TAG)
                                            {
                                                data->camera.base_y = p_ent->position.y;
                                            }
                                        }
                                        else if (p_ent->position.y  >= p_other_ent->position.y + p_other_bbox->size.y)
                                        {
                                            p_ctransform->velocity.y = 0;
                                        }
                                        else
                                        {
                                            p_ctransform->velocity.x = 0;
                                        }
                                    }
                                }

                                if (p_ent->m_tag != PLAYER_ENT_TAG)
                                {

                                    if (p_other_ent->m_tag == PLAYER_ENT_TAG)
                                    {
                                        p_other_ent->m_alive = false;
                                    }
                                    else
                                    {
                                        remove_component(p_other_ent, CHURTBOX_T);
                                        CLifeTimer_t* p_clifetimer = add_component(p_other_ent, CLIFETIMER_T);
                                        if (p_other_ent->m_tag == CRATES_ENT_TAG)
                                        {
                                            CContainer_t* p_container = get_component(p_other_ent, CCONTAINER_T);
                                            if (p_container->item == CONTAINER_BOMB)
                                            {
                                                p_clifetimer->life_time = 0.1f;
                                            }
                                            else
                                            {
                                                p_clifetimer->life_time = 0.05f;
                                            }
                                        }
                                        else
                                        {
                                            p_clifetimer->life_time = 0.05f;
                                        }
                                    }
                                }
                                else
                                {
                                    // Need to remove immediately, otherwise will interfere with bomb spawning
                                    destroy_entity(scene, &tilemap, p_other_ent);
                                    if (p_other_ent->m_tag == CHEST_ENT_TAG)
                                    {
                                        data->coins.current++;
                                    }
                                }
                            }

                        }
                    }
                }
            }
        }
        if (p_hitbox->one_hit && hit)
        {
            destroy_entity(scene, &tilemap, p_ent);
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
        //const CTransform_t* p_ctransform = get_component(p_boulder, CTRANSFORM_COMP_T);
        const CBBox_t* p_bbox = get_component(p_boulder, CBBOX_COMP_T);

        //if (p_ctransform->velocity.y <= 0) continue;

        unsigned int tile_idx = get_tile_idx(
                p_boulder->position.x + p_bbox->half_size.x,
                p_boulder->position.y + p_bbox->size.y + 1,
                tilemap
        );
        unsigned int tile_x = (p_boulder->position.x + p_bbox->half_size.x) / tilemap.tile_size;

        if (tilemap.tiles[tile_idx].tile_type == ONEWAY_TILE)
        {
            play_sfx(scene->engine, WOOD_DESTROY_SFX);
            destroy_tile(data, tile_idx); 
            if (tile_x > 0 && tilemap.tiles[tile_idx - 1].tile_type == ONEWAY_TILE)
            {
                destroy_tile(data, tile_idx - 1); 
            }

            if (tile_x < tilemap.width && tilemap.tiles[tile_idx + 1].tile_type == ONEWAY_TILE)
            {
                destroy_tile(data, tile_idx + 1); 
            }
        }
    }
}

void container_destroy_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
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
            AnchorPoint_t spawn_anchor = AP_MID_CENTER;
            switch (p_container->item)
            {
                case CONTAINER_LEFT_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, 0);
                    play_sfx(scene->engine, ARROW_RELEASE_SFX);
                    spawn_anchor = AP_MID_LEFT;
                break;
                case CONTAINER_RIGHT_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, 1);
                    play_sfx(scene->engine, ARROW_RELEASE_SFX);
                    spawn_anchor = AP_MID_RIGHT;
                break;
                case CONTAINER_UP_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, 2);
                    play_sfx(scene->engine, ARROW_RELEASE_SFX);
                    spawn_anchor = AP_TOP_CENTER;
                break;
                case CONTAINER_DOWN_ARROW:
                    new_ent = create_arrow(&scene->ent_manager, 3);
                    play_sfx(scene->engine, ARROW_RELEASE_SFX);
                    spawn_anchor = AP_BOT_CENTER;
                break;
                case CONTAINER_BOMB:
                    if (dmg_src != NULL && dmg_src->m_tag == PLAYER_ENT_TAG)
                    {
                        const CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
                        const CBBox_t* dmg_src_bbox = get_component(dmg_src, CBBOX_COMP_T);
                        Vector2 launch_dir = {0, -1};
                        if (dmg_src->position.x + dmg_src_bbox->size.x <= p_ent->position.x)
                        {
                            launch_dir.x = 1;
                        }
                        else if (dmg_src->position.x >= p_ent->position.x + p_bbox->size.x)
                        {
                            launch_dir.x = -1;
                        }
                        new_ent = create_bomb(&scene->ent_manager, launch_dir);
                        play_sfx(scene->engine, BOMB_RELEASE_SFX);
                    }
                    else
                    {
                        new_ent = create_explosion(&scene->ent_manager); 
                        play_sfx(scene->engine, EXPLOSION_SFX);
                    }
                break;
                case CONTAINER_EXPLOSION:
                    new_ent = create_explosion(&scene->ent_manager);
                    play_sfx(scene->engine, EXPLOSION_SFX);
                break;
                default:
                    new_ent = NULL;
                break;
            }
            if (new_ent != NULL)
            {
                new_ent->position = Vector2Add(
                    p_ent->position,
                    get_anchor_offset(
                        (Vector2){data->tilemap.tile_size, data->tilemap.tile_size}
                        , spawn_anchor, false
                    )
                );
            }

            // In case there's more materials
            if (p_container->material == WOODEN_CONTAINER)
            {
                play_sfx(scene->engine, WOOD_DESTROY_SFX);
            }
            else
            {
                play_sfx(scene->engine, METAL_DESTROY_SFX);
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
        p_lifetimer->life_time -= scene->delta_time;

        if (p_lifetimer->life_time <= 0.0f)
        {
            destroy_entity(scene, &tilemap, get_entity(&scene->ent_manager, ent_idx));
        }
    }
}

void airtimer_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;
    unsigned int ent_idx;
    CAirTimer_t* p_air;
    sc_map_foreach(&scene->ent_manager.component_map[CAIRTIMER_T], ent_idx, p_air)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;
        CBBox_t* p_bbox = get_component(p_ent, CBBOX_COMP_T);
        CMovementState_t* p_mstate = get_component(p_ent, CBBOX_COMP_T);

        Vector2 point_to_check = p_ent->position;
        if (p_bbox != NULL)
        {
            if (p_mstate != NULL && (p_mstate->ground_state & 1))
            {
                point_to_check = Vector2Add(
                    point_to_check,
                    (Vector2){
                        p_bbox->half_size.x,
                        p_bbox->half_size.y / 2,
                    }
                );
            }
            else
            {
                point_to_check = Vector2Add(
                    point_to_check,
                    (Vector2){
                        p_bbox->half_size.x,
                        p_bbox->half_size.y / 3,
                    }
                );
            }
        }

        unsigned int tile_idx = get_tile_idx(
            point_to_check.x,
            point_to_check.y,
            tilemap
        );

        bool in_water = false;
        int tile_x = tile_idx % tilemap.width;
        int tile_y = tile_idx / tilemap.width;
        uint32_t water_height = data->tilemap.tiles[tile_idx].water_level * WATER_BBOX_STEP;
        Vector2 tl = {tile_x * data->tilemap.tile_size, (tile_y + 1) * data->tilemap.tile_size - water_height};
        in_water |= point_in_AABB(
            point_to_check,
            (Rectangle){tl.x, tl.y, tilemap.tile_size, water_height}
        );

        if (!in_water)
        {
            p_air->curr_count = p_air->max_count;
            p_air->curr_ftimer = p_air->max_ftimer * 2; // Lengthen the first
            CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);
            if (p_emitter != NULL)
            {
                if (is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
                {
                    unload_emitter_handle(&scene->part_sys, p_emitter->handle);
                }
            }
        }
        else
        {
            CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);
            if (p_emitter != NULL)
            {
                if (!is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
                {
                    //Vector2 new_pos = p_ctransform->position;
                    //new_pos.y += p_bbox->half_size.y;

                    ParticleEmitter_t emitter = {
                        .spr = get_sprite(&scene->engine->assets, "p_water"),
                        .config = get_emitter_conf(&scene->engine->assets, "pe_bubbling"),
                        //.position = new_pos,
                        .position = p_ent->position,
                        .n_particles = 5,
                        .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
                        .update_func = &floating_particle_system_update,
                        .emitter_update_func = &check_in_water,
                    };
                    p_emitter->handle = play_particle_emitter(&scene->part_sys, &emitter);
                }
                else
                {
                    play_emitter_handle(&scene->part_sys, p_emitter->handle);
                }
            }

            if (p_air->curr_ftimer > p_air->decay_rate * scene->delta_time)
            {
                p_air->curr_ftimer -= p_air->decay_rate * scene->delta_time;
            }
            else
            {
                if (p_air->curr_count > 0)
                {
                    p_air->curr_count--;
                    p_air->curr_ftimer += p_air->max_ftimer;
                    ParticleEmitter_t emitter = {
                        .spr = get_sprite(&scene->engine->assets, "p_bigbubble"),
                        .config = get_emitter_conf(&scene->engine->assets, "pe_slow"),
                        .position = p_ent->position,
                        .n_particles = 1,
                        .user_data = CONTAINER_OF(scene, LevelScene_t, scene),
                        .update_func = &floating_particle_system_update,
                        .emitter_update_func = NULL,
                    };
                    play_particle_emitter(&scene->part_sys, &emitter);
                    play_sfx(scene->engine, BUBBLE_SFX);
                }
                else
                {
                    if (p_ent->m_tag == PLAYER_ENT_TAG)
                    {
                        p_ent->m_alive = false;
                    }
                    else
                    {
                        destroy_entity(scene, &tilemap, get_entity(&scene->ent_manager, ent_idx));
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
        unsigned int next_idx = p_cspr->current_idx;
        if (p_cspr->transition_func != NULL)
        {
            next_idx = p_cspr->transition_func(p_ent);
        }

        bool reset = p_cspr->current_idx != next_idx;
        p_cspr->current_idx = next_idx;

        SpriteRenderInfo_t spr = p_cspr->sprites[p_cspr->current_idx];
        if (spr.sprite == NULL) continue;

        if (reset)
        {
            p_cspr->fractional = 0;
            p_cspr->elapsed = 0;
            p_cspr->current_frame = 0;
        }

        if (spr.sprite->speed == 0) continue;

        // Animate it (handle frame count)
        p_cspr->fractional += scene->delta_time;
        const float ANIM_FRAME_RATE = 1.0f/24;
        if (p_cspr->fractional > ANIM_FRAME_RATE)
        {
            p_cspr->fractional -= ANIM_FRAME_RATE;
            p_cspr->elapsed++;
            if (p_cspr->elapsed == spr.sprite->speed)
            {
                p_cspr->elapsed = 0;
                if (!p_cspr->pause)
                {
                    p_cspr->current_frame++;
                    p_cspr->current_frame %= spr.sprite->frame_count;
                }
            }
        }
    }
}

void level_end_detection_system(Scene_t* scene)
{
    LevelScene_t* lvl_scene = CONTAINER_OF(scene, LevelScene_t, scene);
    if (lvl_scene->data.coins.current < lvl_scene->data.coins.total) return;
    Entity_t* p_flag;

    TileGrid_t tilemap = lvl_scene->data.tilemap;
    sc_map_foreach_value(&scene->ent_manager.entities_map[LEVEL_END_TAG], p_flag)
    {
        unsigned int tile_idx = get_tile_idx(
                p_flag->position.x,
                p_flag->position.y,
                tilemap
        );

        unsigned int other_ent_idx;
        Entity_t* p_other_ent;
        sc_map_foreach(&tilemap.tiles[tile_idx].entities_set, other_ent_idx, p_other_ent)
        {
            if (p_other_ent->m_tag != PLAYER_ENT_TAG) continue;

            CBBox_t* p_other_bbox = get_component(p_other_ent, CBBOX_COMP_T);

            Vector2 pos = Vector2Subtract(p_flag->position,(Vector2){tilemap.tile_size >> 1, tilemap.tile_size >> 1});
            if (
                find_AABB_overlap(
                    pos, (Vector2){tilemap.tile_size, tilemap.tile_size},
                    p_other_ent->position, p_other_bbox->size, NULL
                )
            )
            {
                Entity_t* ent = create_player_finish(&scene->ent_manager);
                if (ent != NULL)
                {
                    ent->position = p_flag->position;
                    ent->position.y += tilemap.tile_size >> 1;
                    CSprite_t* p_cspr = get_component(p_other_ent, CSPRITE_T);
                    CSprite_t* new_cspr = get_component(ent, CSPRITE_T);
                    new_cspr->flip_x = p_cspr->flip_x;

                }
                destroy_entity(scene, &lvl_scene->data.tilemap, p_other_ent);
                change_level_state(&lvl_scene->data, LEVEL_STATE_COMPLETE);


                //do_action(scene, ACTION_NEXTLEVEL, true);
            }
        }

    }
}

void level_state_management_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);

    if (data->sm.state_functions[data->sm.state] != NULL)
    {
        data->sm.state_functions[data->sm.state](scene);
    }
}

static inline bool is_point_in_water(Vector2 pos, TileGrid_t tilemap)
{
    unsigned int tile_idx = get_tile_idx(pos.x, pos.y, tilemap);
    if (tile_idx >= tilemap.n_tiles) return false;

    int tile_x = tile_idx % tilemap.width;
    int tile_y = tile_idx / tilemap.width;
    uint32_t water_height = tilemap.tiles[tile_idx].water_level * WATER_BBOX_STEP;
    Vector2 tl = {tile_x * tilemap.tile_size, (tile_y + 1) * tilemap.tile_size - water_height};
    return point_in_AABB(
        pos,
        (Rectangle){tl.x, tl.y, tilemap.tile_size, water_height}
    );
}

bool check_in_water(const ParticleEmitter_t* emitter, float delta_time)
{
    (void)delta_time;

    LevelScene_t* scene = (LevelScene_t*)emitter->user_data;
    TileGrid_t tilemap = scene->data.tilemap;
    return is_point_in_water(emitter->position, tilemap);
}

void simple_particle_system_update(Particle_t* part, void* user_data, float delta_time)
{
    LevelScene_t* scene = (LevelScene_t*)user_data;
    TileGrid_t tilemap = scene->data.tilemap;

    part->velocity =
        Vector2Add(
            part->velocity,
            Vector2Scale(GRAVITY, delta_time)
        );

    float mag = Vector2Length(part->velocity);
    part->velocity = Vector2Scale(
        Vector2Normalize(part->velocity),
        (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag
    );
    // 3 dp precision
    if (fabs(part->velocity.x) < 1e-3) part->velocity.x = 0;
    if (fabs(part->velocity.y) < 1e-3) part->velocity.y = 0;

    part->position = Vector2Add(
        part->position,
        Vector2Scale(part->velocity, delta_time)
    );

    Vector2 center = Vector2AddValue(
        part->position,
        part->size / 2
    );
    // Level boundary collision
    unsigned int level_width = tilemap.width * TILE_SIZE;
    unsigned int level_height = tilemap.height * TILE_SIZE;

    if(
        center.x < 0 || center.x + part->size > level_width
        || center.y < 0 || center.y + part->size > level_height
    )
    {
        part->timer = 0;
    }
}

void simple_float_particle_system_update(Particle_t* part, void* user_data, float delta_time)
{
    LevelScene_t* scene = (LevelScene_t*)user_data;
    TileGrid_t tilemap = scene->data.tilemap;

    part->position = Vector2Add(
        part->position,
        Vector2Scale(part->velocity, delta_time)
    );

    Vector2 center = Vector2AddValue(
        part->position,
        part->size / 2
    );
    // Level boundary collision
    unsigned int level_width = tilemap.width * TILE_SIZE;
    unsigned int level_height = tilemap.height * TILE_SIZE;

    if(
        center.x < 0 || center.x + part->size > level_width
        || center.y < 0 || center.y + part->size > level_height
    )
    {
        part->timer = 0;
    }
}

void floating_particle_system_update(Particle_t* part, void* user_data, float delta_time)
{
    simple_float_particle_system_update(part, user_data, delta_time);

    LevelScene_t* scene = (LevelScene_t*)user_data;
    TileGrid_t tilemap = scene->data.tilemap;

    Vector2 center = Vector2AddValue(
        part->position,
        part->size / 2
    );
    center.y = part->position.y;

    if (!is_point_in_water(center, tilemap))
    {
        part->timer = 0;
    }
}
