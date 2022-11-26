#include "mempool.h"
#include "sc/queue/sc_queue.h"
#include "sc/map/sc_map.h"

// Use hashmap as a Set
// Use list will be used to check if an object exist
// The alternative method to check the free list if idx is not there
// requires bound checking
// It's just easier on the mind overall
// If need to optimise memory, replace free_list with set and remove use_list
static struct EntityMemPool
{
    Entity_t entity_buffer[MAX_COMP_POOL_SIZE];
    struct sc_map_64 use_list;
    struct sc_queue_uint free_list;
}entity_mem_pool;

static struct BBoxMemPool
{
    CBBox_t bbox_buffer[MAX_COMP_POOL_SIZE];
    struct sc_map_64 use_list;
    struct sc_queue_uint free_list;
}bbox_mem_pool;

static bool pool_inited = false;
void init_memory_pools(void)
{
    if (!pool_inited)
    {
        memset(bbox_mem_pool.bbox_buffer, 0, sizeof(bbox_mem_pool.bbox_buffer));
        memset(entity_mem_pool.entity_buffer, 0, sizeof(entity_mem_pool.entity_buffer));
        sc_queue_init(&bbox_mem_pool.free_list);
        for (int i=0;i<MAX_COMP_POOL_SIZE;++i)
        {
            sc_queue_add_last(&(bbox_mem_pool.free_list), i);
        }
        sc_queue_init(&entity_mem_pool.free_list);
        for (int i=0;i<MAX_COMP_POOL_SIZE;++i)
        {
            entity_mem_pool.entity_buffer[i].m_id = i;
            sc_map_init_64(&entity_mem_pool.entity_buffer[i].components, 16 ,0);
            sc_queue_add_last(&(entity_mem_pool.free_list), i);
        }
        sc_map_init_64(&bbox_mem_pool.use_list, MAX_COMP_POOL_SIZE ,0);
        sc_map_init_64(&entity_mem_pool.use_list, MAX_COMP_POOL_SIZE ,0);
        pool_inited = true;
    }
}

void free_memory_pools(void)
{
    if (pool_inited)
    {
        sc_map_term_64(&bbox_mem_pool.use_list);
        sc_map_term_64(&entity_mem_pool.use_list);
        sc_queue_term(&bbox_mem_pool.free_list);
        sc_queue_term(&entity_mem_pool.free_list);
        for (int i=0;i<MAX_COMP_POOL_SIZE;++i)
        {
            sc_map_term_64(&entity_mem_pool.entity_buffer[i].components);
        }
        pool_inited = false;
    }
}

Entity_t* new_entity_from_mempool(unsigned long *p_e_idx)
{
    if(sc_queue_empty(&entity_mem_pool.free_list)) return NULL;
    unsigned long e_idx = sc_queue_del_first(&entity_mem_pool.free_list);
    *p_e_idx = e_idx;
    sc_map_put_64(&entity_mem_pool.use_list, e_idx, e_idx);
    Entity_t * ent = entity_mem_pool.entity_buffer + e_idx;
    sc_map_clear_64(&ent->components);
    ent->m_alive = true;
    ent->m_tag = NO_ENT_TAG;
    return ent;
}

Entity_t * get_entity_wtih_id(unsigned long idx)
{
    sc_map_get_64(&entity_mem_pool.use_list, idx);
    if (!sc_map_found(&entity_mem_pool.use_list)) return NULL;
    return entity_mem_pool.entity_buffer + idx;
}

void free_entity_to_mempool(unsigned long idx)
{
	sc_map_del_64(&entity_mem_pool.use_list, idx);
	if (sc_map_found(&entity_mem_pool.use_list))
    {
        sc_queue_add_first(&entity_mem_pool.free_list, idx);
    }
}

void* new_component_from_mempool(ComponentEnum_t comp_type, unsigned long *idx)
{
    void * comp = NULL;
    switch(comp_type)
    {
        case CBBOX_COMP_T:
            if(sc_queue_empty(&bbox_mem_pool.free_list)) break;
            *idx = sc_queue_del_first(&bbox_mem_pool.free_list);
            sc_map_put_64(&bbox_mem_pool.use_list, *idx, *idx);
            comp = bbox_mem_pool.bbox_buffer + *idx;
            memset(comp, 0, sizeof(CBBox_t));
        break;
        default:
        break;
    }
    return comp;
}

void* get_component_wtih_id(ComponentEnum_t comp_type, unsigned long idx)
{
    void * comp = NULL;
    switch(comp_type)
    {
        case CBBOX_COMP_T:
            sc_map_get_64(&bbox_mem_pool.use_list, idx);
            if (!sc_map_found(&bbox_mem_pool.use_list)) break;
            comp = bbox_mem_pool.bbox_buffer + idx;
        break;
        default:
        break;
    }
    return comp;
}

void free_component_to_mempool(ComponentEnum_t comp_type, unsigned long idx)
{
    // This just free the component from the memory pool
    switch(comp_type)
    {
        case CBBOX_COMP_T:
            sc_map_del_64(&bbox_mem_pool.use_list, idx);
            if (sc_map_found(&bbox_mem_pool.use_list))
            {
                sc_queue_add_first(&bbox_mem_pool.free_list, idx);
            }
        break;
        default:
        break;
    }
}
