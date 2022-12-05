#include "mempool.h"
#include "sc/queue/sc_queue.h"
#include "sc/map/sc_map.h"

// Static allocate buffers
static Entity_t entity_buffer[MAX_COMP_POOL_SIZE];
static CBBox_t bbox_buffer[MAX_COMP_POOL_SIZE];
static CTransform_t ctransform_buffer[MAX_COMP_POOL_SIZE];

// Use hashmap as a Set
// Use list will be used to check if an object exist
// The alternative method to check the free list if idx is not there
// requires bound checking
// It's just easier on the mind overall
// If need to optimise memory, replace free_list with set and remove use_list
typedef struct MemPool
{
    void * const buffer;
    const unsigned long max_size;
    const unsigned long elem_size;
    struct sc_map_64 use_list;
    struct sc_queue_uint free_list;
}MemPool_t;

// Static allocate mempools
static MemPool_t comp_mempools[N_COMPONENTS] =
{
    {bbox_buffer, MAX_COMP_POOL_SIZE, sizeof(CBBox_t), {0}, {0}},
    {ctransform_buffer, MAX_COMP_POOL_SIZE, sizeof(CTransform_t), {0}, {0}},
};
static MemPool_t ent_mempool = {entity_buffer, MAX_COMP_POOL_SIZE, sizeof(Entity_t), {0}, {0}};

static bool pool_inited = false;
void init_memory_pools(void)
{
    if (!pool_inited)
    {
        for (size_t i=0; i<N_COMPONENTS; ++i)
        {
            memset(comp_mempools[i].buffer, 0, comp_mempools[i].elem_size*comp_mempools[i].max_size);
            sc_map_init_64(&comp_mempools[i].use_list, MAX_COMP_POOL_SIZE ,0);
            sc_queue_init(&comp_mempools[i].free_list);
            for (size_t j=0;j<MAX_COMP_POOL_SIZE;++j)
            {
                sc_queue_add_last(&comp_mempools[i].free_list, j);
            }
        }

        memset(ent_mempool.buffer, 0, ent_mempool.elem_size*ent_mempool.max_size);
        sc_queue_init(&ent_mempool.free_list);
        sc_map_init_64(&ent_mempool.use_list, MAX_COMP_POOL_SIZE ,0);
        for (size_t i=0;i<MAX_COMP_POOL_SIZE;++i)
        {
            entity_buffer[i].m_id = i;
            sc_map_init_64(&entity_buffer[i].components, 16 ,0);
            sc_queue_add_last(&(ent_mempool.free_list), i);
        }
        pool_inited = true;
    }
}

void free_memory_pools(void)
{
    if (pool_inited)
    {
        for (size_t i=0; i<N_COMPONENTS; ++i)
        {
            sc_map_term_64(&comp_mempools[i].use_list);
            sc_queue_term(&comp_mempools[i].free_list);
        }
        sc_map_term_64(&ent_mempool.use_list);
        sc_queue_term(&ent_mempool.free_list);

        for (int i=0;i<MAX_COMP_POOL_SIZE;++i)
        {
            sc_map_term_64(&entity_buffer[i].components);
        }
        pool_inited = false;
    }
}

Entity_t* new_entity_from_mempool(unsigned long *p_e_idx)
{
    if(sc_queue_empty(&ent_mempool.free_list)) return NULL;
    unsigned long e_idx = sc_queue_del_first(&ent_mempool.free_list);
    *p_e_idx = e_idx;
    sc_map_put_64(&ent_mempool.use_list, e_idx, e_idx);
    Entity_t * ent = entity_buffer + e_idx;
    sc_map_clear_64(&ent->components);
    ent->m_alive = true;
    ent->m_tag = NO_ENT_TAG;
    return ent;
}

Entity_t * get_entity_wtih_id(unsigned long idx)
{
    sc_map_get_64(&ent_mempool.use_list, idx);
    if (!sc_map_found(&ent_mempool.use_list)) return NULL;
    return entity_buffer + idx;
}

void free_entity_to_mempool(unsigned long idx)
{
	sc_map_del_64(&ent_mempool.use_list, idx);
	if (sc_map_found(&ent_mempool.use_list))
    {
        sc_queue_add_first(&ent_mempool.free_list, idx);
    }
}

void* new_component_from_mempool(ComponentEnum_t comp_type, unsigned long *idx)
{
    void * comp = NULL;
    assert(comp_type < N_COMPONENTS);
    if(!sc_queue_empty(&comp_mempools[comp_type].free_list))
    {
        *idx = sc_queue_del_first(&comp_mempools[comp_type].free_list);
        sc_map_put_64(&comp_mempools[comp_type].use_list, *idx, *idx);
        comp = comp_mempools[comp_type].buffer + (*idx * comp_mempools[comp_type].elem_size);
        memset(comp, 0, comp_mempools[comp_type].elem_size);
    }
    return comp;
}

void* get_component_wtih_id(ComponentEnum_t comp_type, unsigned long idx)
{
    void * comp = NULL;
    assert(comp_type < N_COMPONENTS);
    sc_map_get_64(&comp_mempools[comp_type].use_list, idx);
    if (sc_map_found(&comp_mempools[comp_type].use_list))
    {
        comp = comp_mempools[comp_type].buffer + (idx * comp_mempools[comp_type].elem_size);
    }
    return comp;
}

void free_component_to_mempool(ComponentEnum_t comp_type, unsigned long idx)
{
    assert(comp_type < N_COMPONENTS);
    // This just free the component from the memory pool
    sc_map_del_64(&comp_mempools[comp_type].use_list, idx);
    if (sc_map_found(&comp_mempools[comp_type].use_list))
    {
        sc_queue_add_first(&comp_mempools[comp_type].free_list, idx);
    }
}
