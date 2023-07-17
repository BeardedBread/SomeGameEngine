#include "collisions.h"
#include "AABB.h"

void remove_entity_from_tilemap(EntityManager_t *p_manager, TileGrid_t* tilemap, Entity_t* p_ent)
{
    CTileCoord_t* p_tilecoord = get_component(p_ent, CTILECOORD_COMP_T);
    for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
    {
        // Use previously store tile position
        // Clear from those positions
        unsigned int tile_idx = p_tilecoord->tiles[i];
        sc_map_del_64v(&(tilemap->tiles[tile_idx].entities_set), p_ent->m_id);
    }
    remove_entity(p_manager, p_ent->m_id);
}

uint8_t check_collision(const CollideEntity_t* ent, TileGrid_t* grid, bool check_oneway)
{
    unsigned int tile_x1 = (ent->area.tile_x1 < 0) ? 0 : ent->area.tile_x1;
    unsigned int tile_x2 = (ent->area.tile_x2 >= grid->width) ? grid->width - 1 : ent->area.tile_x2;
    unsigned int tile_y1 = (ent->area.tile_y1 < 0) ? 0 : ent->area.tile_y1;
    unsigned int tile_y2 = (ent->area.tile_y2 >= grid->height) ? grid->height - 1 : ent->area.tile_y2;

    for(unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
    {
        for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
        {
            if (tile_x >= grid->width) return 0;
            unsigned int tile_idx = tile_y*grid->width + tile_x;

            Vector2 overlap;
            if (grid->tiles[tile_idx].solid == SOLID)
            {
                if (find_AABB_overlap(
                    (Vector2){ent->bbox.x, ent->bbox.y},
                    (Vector2){ent->bbox.width, ent->bbox.height},
                    (Vector2){tile_x * grid->tile_size + grid->tiles[tile_idx].offset.x, tile_y * grid->tile_size + grid->tiles[tile_idx].offset.y},
                    grid->tiles[tile_idx].size,
                    &overlap
                ))
                {
                    return 1;
                }
            }

            if (check_oneway && grid->tiles[tile_idx].solid == ONE_WAY)
            {
                find_AABB_overlap(
                    (Vector2){ent->bbox.x, ent->bbox.y},
                    (Vector2){ent->bbox.width, ent->bbox.height},
                    (Vector2){tile_x * grid->tile_size + grid->tiles[tile_idx].offset.x, tile_y * grid->tile_size + grid->tiles[tile_idx].offset.y},
                    grid->tiles[tile_idx].size,
                    &overlap
                );

                //For one-way platform, check for vectical collision, only return true for up direction
                if (overlap.y != 0 && ent->prev_bbox.y + ent->prev_bbox.height - 1 < tile_y * grid->tile_size) return 1;
            }

            Entity_t* p_other_ent;
            sc_map_foreach_value(&grid->tiles[tile_idx].entities_set, p_other_ent)
            {
                if (ent->p_ent->m_id == p_other_ent->m_id) continue;
                if (!ent->p_ent->m_alive) continue;
                CTransform_t *p_ctransform = get_component(p_other_ent, CTRANSFORM_COMP_T);
                CBBox_t *p_bbox = get_component(p_other_ent, CBBOX_COMP_T);
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
                        return (p_bbox->fragile) ? 2 : 1;
                    }
                }
            }
        }
    }
    return 0;
}

uint8_t check_collision_line(const CollideEntity_t* ent, TileGrid_t* grid, bool check_oneway)
{
    unsigned int tile_x1 = (ent->area.tile_x1 < 0) ? 0 : ent->area.tile_x1;
    unsigned int tile_x2 = (ent->area.tile_x2 >= grid->width) ? grid->width - 1 : ent->area.tile_x2;
    unsigned int tile_y1 = (ent->area.tile_y1 < 0) ? 0 : ent->area.tile_y1;
    unsigned int tile_y2 = (ent->area.tile_y2 >= grid->height) ? grid->height - 1 : ent->area.tile_y2;

    Vector2 p1 = {ent->bbox.x, ent->bbox.y};
    Vector2 p2 = {ent->bbox.x + ent->bbox.width - 1, ent->bbox.y + ent->bbox.height - 1};
    for(unsigned int tile_y = tile_y1; tile_y <= tile_y2; tile_y++)
    {
        if (tile_y >= grid->height) return 0;
        for(unsigned int tile_x = tile_x1; tile_x <= tile_x2; tile_x++)
        {
            if (tile_x >= grid->width) return 0;
            unsigned int tile_idx = tile_y*grid->width + tile_x;
            if (grid->tiles[tile_idx].solid == SOLID)
            {
                Rectangle tile_rec = {
                    .x = tile_x * grid->tile_size + grid->tiles[tile_idx].offset.x,
                    .y = tile_y * grid->tile_size + grid->tiles[tile_idx].offset.y,
                    .width = grid->tiles[tile_idx].size.x,
                    .height = grid->tiles[tile_idx].size.y
                };
                if ( line_in_AABB(p1, p2, tile_rec) ) return 1;
            }

            if (check_oneway && grid->tiles[tile_idx].solid == ONE_WAY)
            {
                Rectangle tile_rec = {
                    .x = tile_x * grid->tile_size + grid->tiles[tile_idx].offset.x,
                    .y = tile_y * grid->tile_size + grid->tiles[tile_idx].offset.y,
                    .width = grid->tiles[tile_idx].size.x,
                    .height = grid->tiles[tile_idx].size.y
                };
                bool collide = line_in_AABB(p1, p2, tile_rec);

                //For one-way platform, check for vectical collision, only return true for up direction
                if (collide && ent->prev_bbox.y + ent->prev_bbox.height - 1 < tile_y * grid->tile_size) return 1;
            }

            Entity_t* p_other_ent;
            sc_map_foreach_value(&grid->tiles[tile_idx].entities_set, p_other_ent)
            {
                if (ent->p_ent->m_id == p_other_ent->m_id) continue;
                if (!ent->p_ent->m_alive) continue;
                CTransform_t *p_ctransform = get_component(p_other_ent, CTRANSFORM_COMP_T);
                CBBox_t *p_bbox = get_component(p_other_ent, CBBOX_COMP_T);
                if (p_bbox == NULL || p_ctransform == NULL) continue;
                //if (p_bbox->solid && !p_bbox->fragile)
                if (p_bbox->solid)
                {
                    Rectangle box = {
                        .x = p_ctransform->position.x,
                        .y = p_ctransform->position.y,
                        .width = p_bbox->size.x,
                        .height = p_bbox->size.y,
                    };
                    if ( line_in_AABB(p1, p2, box) )
                    {
                        return (p_bbox->fragile) ? 2 : 1;
                    }
                }
            }
        }
    }
    return 0;
}

// TODO: This should be a point collision check, not an  AABB check
uint8_t check_collision_offset(Entity_t* p_ent, Vector2 pos, Vector2 bbox_sz, TileGrid_t* grid, Vector2 offset)
{
    Vector2 new_pos = Vector2Add(pos, offset);
    CollideEntity_t ent = {
        .p_ent = p_ent,
        .bbox = (Rectangle){new_pos.x, new_pos.y, bbox_sz.x, bbox_sz.y},
        .prev_bbox = (Rectangle){pos.x, pos.y, bbox_sz.x, bbox_sz.y},
        .area = (TileArea_t){
            .tile_x1 = (new_pos.x) / grid->tile_size,
            .tile_y1 = (new_pos.y) / grid->tile_size,
            .tile_x2 = (new_pos.x + bbox_sz.x - 1) / grid->tile_size,
            .tile_y2 = (new_pos.y + bbox_sz.y - 1) / grid->tile_size
        }
    };
    
    return check_collision(&ent, grid, false);
}

bool check_on_ground(Entity_t* p_ent, Vector2 pos, Vector2 prev_pos, Vector2 bbox_sz, TileGrid_t* grid)
{
    //return check_collision_at(ent_idx, pos, bbox_sz, grid, (Vector2){0, 1}, p_manager);
    Vector2 new_pos = Vector2Add(pos, (Vector2){0, 1});
    CollideEntity_t ent = {
        .p_ent = p_ent,
        .bbox = (Rectangle){new_pos.x, new_pos.y + bbox_sz.y - 1, bbox_sz.x, 1},
        .prev_bbox = (Rectangle){prev_pos.x, prev_pos.y, bbox_sz.x, bbox_sz.y},
        .area = (TileArea_t){
            .tile_x1 = (new_pos.x) / grid->tile_size,
            .tile_y1 = (new_pos.y + bbox_sz.y - 1) / grid->tile_size,
            .tile_x2 = (new_pos.x + bbox_sz.x - 1) / grid->tile_size,
            .tile_y2 = (new_pos.y + bbox_sz.y - 1) / grid->tile_size
        }
    };
    
    return check_collision(&ent, grid, true);
}

uint8_t check_bbox_edges(
    TileGrid_t* tilemap,
    Entity_t* p_ent, Vector2 pos, Vector2 prev_pos, Vector2 bbox,
    bool ignore_fragile
)
{
    uint8_t detected = 0;

    // Too lazy to adjust the tile area to check, so just make a big one
    CollideEntity_t ent = 
    {
        .p_ent = p_ent,
        .bbox = (Rectangle){pos.x - 1, pos.y, 1, bbox.y},
        .prev_bbox = (Rectangle){pos.x, pos.y, bbox.x, bbox.y},
        .area = (TileArea_t){
            .tile_x1 = (pos.x - 1) / tilemap->tile_size,
            .tile_y1 = (pos.y - 1) / tilemap->tile_size,
            .tile_x2 = (pos.x + bbox.x) / tilemap->tile_size,
            .tile_y2 = (pos.y + bbox.y) / tilemap->tile_size,
        }
    };


    // Left
    uint8_t collide_type = check_collision_line(&ent, tilemap, false);
    if (collide_type == 1 || (collide_type == 2 && !ignore_fragile))
    {
        detected |= 1 << 3;
    }

    //Right
    ent.bbox.x = pos.x + bbox.x + 1; // 2 to account for the previous subtraction
    collide_type = check_collision_line(&ent, tilemap, false);
    if (collide_type == 1 || (collide_type == 2 && !ignore_fragile))
    {
        detected |= 1 << 2;
    }

    // Up
    ent.bbox.x = pos.x;
    ent.bbox.y = pos.y - 1;
    ent.bbox.width = bbox.x;
    ent.bbox.height = 1;
    collide_type = check_collision_line(&ent, tilemap, false);
    if (collide_type == 1 || (collide_type == 2 && !ignore_fragile))
    {
        detected |= 1 << 1;
    }

    // Down
    ent.bbox.y = pos.y + bbox.y + 1;
    collide_type = check_collision_line(&ent, tilemap, true);
    if (collide_type == 1 || (collide_type == 2 && !ignore_fragile))
    {
        detected |= 1;
    }
    return detected;
}
