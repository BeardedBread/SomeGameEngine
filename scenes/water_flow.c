#include "water_flow.h"
#include "constants.h"
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
    p_crunner->movement_delay = 5;
    p_crunner->movement_speed = 6;

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
    p_crunner->bfs_tilemap.tilemap = NULL;
    free(p_crunner->visited);
    p_crunner->visited = NULL;
    sc_queue_term(&p_crunner->bfs_queue);
    remove_entity(ent_manager, ent->m_id);
}

static void runner_BFS(const TileGrid_t* tilemap, CWaterRunner_t* p_crunner, int32_t* lowest_tile, int32_t start_tile)
{
    memset(p_crunner->visited, 0, p_crunner->bfs_tilemap.len * sizeof(bool));
    p_crunner->visited[start_tile] = true;
    p_crunner->bfs_tilemap.tilemap[start_tile].reachable = true;
    sc_queue_add_last(&p_crunner->bfs_queue, start_tile);
    int init_height = start_tile / p_crunner->bfs_tilemap.width;
    bool first_reached = false;
    while (!sc_queue_empty(&p_crunner->bfs_queue))
    {
        const int curr_idx = sc_queue_peek_first(&p_crunner->bfs_queue);
        sc_queue_del_first(&p_crunner->bfs_queue);

        int curr_height = curr_idx / p_crunner->bfs_tilemap.width;
        int curr_low = *lowest_tile / p_crunner->bfs_tilemap.width;

        // Possible optimisation to avoid repeated BFS, dunno how possible

        bool to_go[4] = {false, false, false, false};
        Tile_t* curr_tile = tilemap->tiles + curr_idx;

        int next = curr_idx + p_crunner->bfs_tilemap.width;
        Tile_t* next_tile = tilemap->tiles + next;

        if (
            ((curr_height <= init_height && !first_reached) || curr_height > curr_low)
            && curr_tile->water_level < curr_tile->max_water_level
        )
        {
            first_reached = true;
            *lowest_tile = curr_idx;
        }


        if (next < p_crunner->bfs_tilemap.len)
        {
            to_go[0] = (next_tile->solid != SOLID && next_tile->max_water_level == MAX_WATER_LEVEL);

            if (
                next_tile->solid == SOLID
                || next_tile->water_level >= next_tile->max_water_level
                || curr_tile->water_level >= curr_tile->max_water_level
            )
            {
                if (curr_idx % p_crunner->bfs_tilemap.width != 0)
                {
                    next = curr_idx - 1;
                    next_tile = tilemap->tiles + next;
                    to_go[1] = (next_tile->solid != SOLID && next_tile->max_water_level == MAX_WATER_LEVEL);
                }
                next = curr_idx + 1;
                if (next % p_crunner->bfs_tilemap.width != 0)
                {
                    next_tile = tilemap->tiles + next;
                    to_go[2] = (next_tile->solid != SOLID && next_tile->max_water_level == MAX_WATER_LEVEL);
                }
            }
        }
        
        if (curr_tile->water_level >= curr_tile->max_water_level)
        {

            next = curr_idx - p_crunner->bfs_tilemap.width;
            if (next >= 0)
            {
                next_tile = tilemap->tiles + next;
                to_go[3] = (next_tile->solid != SOLID && next_tile->max_water_level == MAX_WATER_LEVEL);
            }
        }

        const int8_t offsets[4] = {p_crunner->bfs_tilemap.width, -1, 1, -p_crunner->bfs_tilemap.width};
        for (uint8_t i = 0; i < 4; ++i)
        {
            next = curr_idx + offsets[i];
            if (to_go[i] && !p_crunner->visited[next])
            {
                sc_queue_add_last(&p_crunner->bfs_queue, next);
                p_crunner->bfs_tilemap.tilemap[next].from = curr_idx;
                p_crunner->visited[next] = true;
                p_crunner->bfs_tilemap.tilemap[next].reachable = true;
            }
        }
    }
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
        Entity_t* ent = get_entity(&scene->ent_manager, ent_idx);
        if (tilemap.tiles[p_crunner->current_tile].solid == SOLID)
        {
            CTileCoord_t* p_tilecoord = get_component(
                ent, CTILECOORD_COMP_T
            );
            for (size_t i = 0;i < p_tilecoord->n_tiles; ++i)
            {
                // Use previously store tile position
                // Clear from those positions
                int tile_idx = p_tilecoord->tiles[i];
                sc_map_del_64v(&(tilemap.tiles[tile_idx].entities_set), ent_idx);
            }
            free_water_runner(ent, &scene->ent_manager);
            continue;
        }

        switch (p_crunner->state)
        {
            case BFS_RESET:
                for (int32_t i = 0; i < p_crunner->bfs_tilemap.len; ++i)
                {
                    //p_crunner->bfs_tilemap.tilemap[i].to = -1;
                    p_crunner->bfs_tilemap.tilemap[i].from = -1;
                    p_crunner->bfs_tilemap.tilemap[i].reachable = false;
                }
                p_crunner->state = LOWEST_POINT_SEARCH;
                // Want the fallthough
            case LOWEST_POINT_SEARCH:
            {
                int32_t lowest_tile = p_crunner->current_tile - p_crunner->bfs_tilemap.width;
                if (lowest_tile <= 0) lowest_tile = p_crunner->current_tile;

                runner_BFS(&tilemap, p_crunner, &lowest_tile, p_crunner->current_tile);
                p_crunner->target_tile = lowest_tile;
                // Trace path from lowest_tile
                int prev_idx = lowest_tile;
                int curr_idx = p_crunner->bfs_tilemap.tilemap[prev_idx].from;
                while (p_crunner->bfs_tilemap.tilemap[prev_idx].from >= 0)
                {
                    p_crunner->bfs_tilemap.tilemap[curr_idx].to = prev_idx;
                    prev_idx = curr_idx;
                    curr_idx = p_crunner->bfs_tilemap.tilemap[prev_idx].from;
                }
                p_crunner->bfs_tilemap.tilemap[lowest_tile].to = -1;
                if (prev_idx == lowest_tile)
                {
                    p_crunner->state = REACHABILITY_SEARCH;
                    break;
                }
                p_crunner->counter = p_crunner->movement_delay;
                p_crunner->state = LOWEST_POINT_MOVEMENT;
            }
            break;
            case LOWEST_POINT_MOVEMENT:
                p_crunner->counter--;
                if (p_crunner->counter == 0)
                {

                    int8_t move_left = p_crunner->movement_speed;
                    while (move_left)
                    {
                        p_crunner->current_tile = p_crunner->bfs_tilemap.tilemap[p_crunner->current_tile].to;
                        CTransform_t* p_ct = get_component(ent, CTRANSFORM_COMP_T);
                        p_ct->position.x = (p_crunner->current_tile % tilemap.width) * tilemap.tile_size; 
                        p_ct->position.y = (p_crunner->current_tile / tilemap.width) * tilemap.tile_size; 

                        Tile_t* tile = tilemap.tiles + p_crunner->current_tile;
                        tile->wet = true;
                        move_left--;
                        if (tile->water_level != tile->max_water_level)
                        {
                            move_left--;
                        }
                        if (p_crunner->current_tile == p_crunner->target_tile)
                        {
                            p_crunner->state = REACHABILITY_SEARCH;
                            break;
                        }
                        p_crunner->counter = p_crunner->movement_delay;
                    }
                }
            break;
            case REACHABILITY_SEARCH:
            {
                Tile_t* curr_tile = tilemap.tiles + p_crunner->current_tile;
                if (curr_tile->water_level >= curr_tile->max_water_level)
                {
                    p_crunner->state = BFS_RESET;
                    break;
                }
                int start_tile =
                    (p_crunner->current_tile / p_crunner->bfs_tilemap.width) * p_crunner->bfs_tilemap.width;

                for (int32_t i = 0; i < p_crunner->bfs_tilemap.width; ++i)
                {
                    p_crunner->bfs_tilemap.tilemap[start_tile + i].reachable = false;
                }

                int32_t lowest_tile = p_crunner->current_tile;
                runner_BFS(&tilemap, p_crunner, &lowest_tile, p_crunner->current_tile);

                for (int i = 0; i < p_crunner->bfs_tilemap.width; ++i)
                {
                    if (
                        p_crunner->bfs_tilemap.tilemap[start_tile + i].reachable
                    )
                    {
                        p_crunner->fill_range[0] = i;
                        break;
                    }
                }
                for (int i = p_crunner->bfs_tilemap.width - 1; i >= 0; --i)
                {
                    if (
                        p_crunner->bfs_tilemap.tilemap[start_tile + i].reachable
                    )
                    {
                        p_crunner->fill_range[1] = i;
                        break;
                    }
                }

                p_crunner->fill_idx = p_crunner->fill_range[0];
                p_crunner->counter = 0;
                p_crunner->state = SCANLINE_FILL;
            }
            break;
            case SCANLINE_FILL:
            {
                // Unsigned usage here is okay
                unsigned int start_tile =
                    (p_crunner->current_tile / p_crunner->bfs_tilemap.width) * p_crunner->bfs_tilemap.width;
                //for (size_t i = 0; i < 10; ++i)
                {
                    unsigned int curr_idx = start_tile + p_crunner->fill_idx;
                    Tile_t* curr_tile = tilemap.tiles + curr_idx;
                    if (
                        p_crunner->bfs_tilemap.tilemap[curr_idx].reachable
                    )
                    {
                        if (curr_tile->water_level < curr_tile->max_water_level)
                        {
                            curr_tile->water_level++;
                        }
                        if (curr_tile->water_level < curr_tile->max_water_level)
                        {
                            p_crunner->counter++;
                        }
                        else
                        {
                            curr_tile->wet = false;
                        }
                    }


                    p_crunner->fill_idx++;
                    if (p_crunner->fill_idx > p_crunner->fill_range[1])
                    {
                        if (p_crunner->counter == 0)
                        {
                            p_crunner->state = BFS_RESET;
                            break;
                        }
                        p_crunner->counter = 0;
                        p_crunner->fill_idx = p_crunner->fill_range[0];
                    }
                }
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
