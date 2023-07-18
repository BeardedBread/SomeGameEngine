#include "water_flow.h"


Entity_t* create_water_runner(EntityManager_t* ent_manager, int32_t width, int32_t height, int32_t start_tile)
{
    Entity_t* p_filler = add_entity(ent_manager, NO_ENT_TAG);
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

    p_crunner->current_tile = start_tile;
    return p_filler;
}

void free_water_runner(Entity_t** ent, EntityManager_t* ent_manager)
{
    CWaterRunner_t* p_crunner = get_component(*ent, CWATERRUNNER_T);
    free(p_crunner->bfs_tilemap.tilemap);
    remove_entity(ent_manager, (*ent)->m_id);
    *ent = NULL;
}


void update_water_runner_system(Scene_t* scene)
{
    // The core of the water runner is to:
    //  - Reach the lowest possible point in the tilemap
    //  - Scanline fill
    // A runner is given an amount of movement cost
    // Within the movement cost, do the following logic
    // Perform a modified DFS to find the lowest point:
    //  - Solid tiles are not reachable
    //  - If bottom tile is non-solid, that is the only reachable tile,
    //  - If bottom tile is filled with water fully, down+left+right are reachable
    //  - If bottom tile is solid, left+right are reachable
    //  - If bottom tile is OOB, terminate
    // Use a LIFO to deal with this.
    // On DFS completion, find the path to the lowest point. Keep track of this 
    // The DFS should have figured out all reachable tiles, start scanline filling at the lowest point.
    // On completion, move up update tile reachability by DFS on the current level. (repeat first step)
    //  - No need to recheck already reachable tiles
    //  - If current tile is solid, scan left and right for reachable tile and start from there
    // Repeat scanline fill
}
