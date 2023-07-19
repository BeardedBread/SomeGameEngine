#include "water_flow.h"
#include "sc/queue/sc_queue.h"
#include <stdio.h>
Entity_t* create_water_runner(EntityManager_t* ent_manager, int32_t width, int32_t height, int32_t start_tile)
{
    Entity_t* p_filler = add_entity(ent_manager, DYNMEM_ENT_TAG);
    if (p_filler == NULL) return NULL;
    CWaterRunner_t* p_crunner = add_component(p_filler, CWATERRUNNER_T);
    if (p_crunner == NULL)
    {
        remove_entity(ent_manager, p_filler->m_id);
        return NULL;
    }
    int32_t total = width * height;
    p_crunner->bfs_tilemap.tilemap = calloc(total, sizeof(BFSTile_t));
    if (p_crunner->bfs_tilemap.tilemap == NULL)
    {
        remove_entity(ent_manager, p_filler->m_id);
        return NULL;
    }
    p_crunner->bfs_tilemap.width = width;
    p_crunner->bfs_tilemap.height = height;
    p_crunner->bfs_tilemap.len = total;

    sc_queue_init(&p_crunner->bfs_queue);
    p_crunner->visited = calloc(total, sizeof(bool));

    p_crunner->current_tile = start_tile;
    p_crunner->target_tile = total;

    CTransform_t* p_ct = add_component(p_filler, CTRANSFORM_COMP_T);
    p_ct->movement_mode = KINEMATIC_MOVEMENT;
    add_component(p_filler, CTILECOORD_COMP_T);
    return p_filler;
}

void free_water_runner(Entity_t* ent, EntityManager_t* ent_manager)
{
    CWaterRunner_t* p_crunner = get_component(ent, CWATERRUNNER_T);
    free(p_crunner->bfs_tilemap.tilemap);
    free(p_crunner->visited);
    sc_queue_term(&p_crunner->bfs_queue);
    remove_entity(ent_manager, ent->m_id);
}

void update_water_runner_system(Scene_t* scene)
{
    // The core of the water runner is to:
    //  - Reach the lowest possible point in the tilemap
    //  - Scanline fill
    // A runner is given an amount of movement cost
    // Within the movement cost, do the following logic
    // Perform a modified BFS to find the lowest point:
    //  - Solid tiles are not reachable
    //  - If bottom tile is non-solid, that is the only reachable tile,
    //  - If bottom tile is filled with water fully, down+left+right are reachable
    //  - If bottom tile is solid, left+right are reachable
    //  - If bottom tile is OOB, terminate
    // Use a FIFO to deal with this.
    // On DFS completion, find the path to the lowest point. Keep track of this 
    // The DFS should have figured out all reachable tiles, start scanline filling at the lowest point.
    // On completion, move up update tile reachability by DFS on the current level. (repeat first step)
    //  - No need to recheck already reachable tiles
    //  - If current tile is solid, scan left and right for reachable tile and start from there
    // Repeat scanline fill
    LevelSceneData_t* data = &(CONTAINER_OF(scene, LevelScene_t, scene)->data);
    TileGrid_t tilemap = data->tilemap;

    CWaterRunner_t* p_crunner;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CWATERRUNNER_T], ent_idx, p_crunner)
    {
        //Entity_t* ent = get_entity(&scene->ent_manager, ent_idx);
        sc_queue_add_last(&p_crunner->bfs_queue, p_crunner->current_tile);

        switch (p_crunner->state)
        {
            case LOWEST_POINT_SEARCH:
            {
                for (size_t i = 0; i < p_crunner->bfs_tilemap.len; ++i)
                {
                    p_crunner->bfs_tilemap.tilemap[i].from = -1;
                }
                memset(p_crunner->visited, 0, p_crunner->bfs_tilemap.len * sizeof(bool));
                int32_t lowest_tile = p_crunner->current_tile;
                while (!sc_queue_empty(&p_crunner->bfs_queue))
                {
                    unsigned int curr_idx = sc_queue_peek_first(&p_crunner->bfs_queue);
                    sc_queue_del_first(&p_crunner->bfs_queue);

                    unsigned int curr_height = curr_idx / p_crunner->bfs_tilemap.width;
                    unsigned int curr_low = lowest_tile / p_crunner->bfs_tilemap.width;
                    if (curr_height > curr_low)
                    {
                        lowest_tile = curr_idx;
                    }

                    p_crunner->visited[curr_idx] = true;
                    p_crunner->bfs_tilemap.tilemap[curr_idx].reachable = true;
                    // Possible optimisation to avoid repeated BFS, dunno how possible

                    unsigned int next = curr_idx + p_crunner->bfs_tilemap.width;
                    if (next >= p_crunner->bfs_tilemap.len) continue;

                    Tile_t* next_tile = tilemap.tiles + next;
                    if (
                        next_tile->solid != SOLID
                        && next_tile->water_level < next_tile->max_water_level
                        && !p_crunner->visited[next]
                    )
                    {
                        sc_queue_add_last(&p_crunner->bfs_queue, next);
                        p_crunner->bfs_tilemap.tilemap[next].from = curr_idx;
                    }
                    else
                    {
                        next = curr_idx - 1;
                        if (next % p_crunner->bfs_tilemap.width != 0)
                        {
                            next_tile = tilemap.tiles + next;
                            if (
                                next_tile->solid != SOLID
                                && next_tile->water_level < next_tile->max_water_level
                                && !p_crunner->visited[next]
                            )
                            {
                                sc_queue_add_last(&p_crunner->bfs_queue, next);
                                p_crunner->bfs_tilemap.tilemap[next].from = curr_idx;
                            }
                        }
                        next = curr_idx + 1;
                        if (next % p_crunner->bfs_tilemap.width != 0)
                        {
                            next_tile = tilemap.tiles + next;
                            if (
                                next_tile->solid != SOLID
                                && next_tile->water_level < next_tile->max_water_level
                                && !p_crunner->visited[next]
                            )
                            {
                                sc_queue_add_last(&p_crunner->bfs_queue, next);
                                p_crunner->bfs_tilemap.tilemap[next].from = curr_idx;
                            }
                        }
                    }
                }
                p_crunner->target_tile = lowest_tile;

                // Trace path from lowest_tile
                unsigned int prev_idx = lowest_tile;
                unsigned int curr_idx = p_crunner->bfs_tilemap.tilemap[prev_idx].from;
                while (p_crunner->bfs_tilemap.tilemap[prev_idx].from >= 0)
                {
                    p_crunner->bfs_tilemap.tilemap[curr_idx].to = prev_idx;
                    prev_idx = curr_idx;
                    curr_idx = p_crunner->bfs_tilemap.tilemap[prev_idx].from;
                }
                p_crunner->state = LOWEST_POINT_MOVEMENT;
            }
            break;
            default:
            break;
        }
    }
}

void init_water_runner_system(void)
{
}
