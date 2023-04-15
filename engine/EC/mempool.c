#include "mempool.h"
#include "sc/queue/sc_queue.h"
#include <stdlib.h>
#include <stdio.h>

// Static allocate buffers
static Entity_t entity_buffer[MAX_COMP_POOL_SIZE];
static CBBox_t bbox_buffer[MAX_COMP_POOL_SIZE];
static CTransform_t ctransform_buffer[MAX_COMP_POOL_SIZE];
static CTileCoord_t ctilecoord_buffer[MAX_COMP_POOL_SIZE];
static CMovementState_t cmstate_buffer[MAX_COMP_POOL_SIZE];
static CJump_t cjump_buffer[1]; // Only player is expected to have this
static CPlayerState_t cplayerstate_buffer[1]; // Only player is expected to have this
static CContainer_t ccontainer_buffer[MAX_COMP_POOL_SIZE];
static CHitBoxes_t chitboxes_buffer[MAX_COMP_POOL_SIZE];
static CHurtbox_t churtbox_buffer[MAX_COMP_POOL_SIZE];
static CSprite_t csprite_buffer[MAX_COMP_POOL_SIZE];

typedef struct MemPool {
    void * const buffer;
    const unsigned long max_size;
    const unsigned long elem_size;
    bool *use_list;
    struct sc_queue_uint free_list;
} MemPool_t;

// Static allocate mempools
static MemPool_t comp_mempools[N_COMPONENTS] = {
    {bbox_buffer, MAX_COMP_POOL_SIZE, sizeof(CBBox_t), NULL, {0}},
    {ctransform_buffer, MAX_COMP_POOL_SIZE, sizeof(CTransform_t), NULL, {0}},
    {ctilecoord_buffer, MAX_COMP_POOL_SIZE, sizeof(CTileCoord_t), NULL, {0}},
    {cmstate_buffer, MAX_COMP_POOL_SIZE, sizeof(CMovementState_t), NULL, {0}},
    {cjump_buffer, 1, sizeof(CJump_t), NULL, {0}},
    {cplayerstate_buffer, 1, sizeof(CPlayerState_t), NULL, {0}},
    {ccontainer_buffer, MAX_COMP_POOL_SIZE, sizeof(CContainer_t), NULL, {0}},
    {chitboxes_buffer, MAX_COMP_POOL_SIZE, sizeof(CHitBoxes_t), NULL, {0}},
    {churtbox_buffer, MAX_COMP_POOL_SIZE, sizeof(CHurtbox_t), NULL, {0}},
    {csprite_buffer, MAX_COMP_POOL_SIZE, sizeof(CSprite_t), NULL, {0}},
};
static MemPool_t ent_mempool = {
    .buffer = entity_buffer,
    .max_size = MAX_COMP_POOL_SIZE,
    .elem_size = sizeof(Entity_t),
    .use_list = NULL,
    .free_list = {0}
};

static bool pool_inited = false;
void init_memory_pools(void)
{
    if (!pool_inited)
    {
        for (size_t i = 0; i < N_COMPONENTS; ++i)
        {
            memset(comp_mempools[i].buffer, 0, comp_mempools[i].elem_size * comp_mempools[i].max_size);
            comp_mempools[i].use_list = (bool*)calloc(comp_mempools[i].max_size, sizeof(bool));
            assert(comp_mempools[i].use_list != NULL);
            sc_queue_init(&comp_mempools[i].free_list);
            for (size_t j = 0; j < comp_mempools[i].max_size; ++j)
            {
                sc_queue_add_last(&comp_mempools[i].free_list, j);
            }
        }

        memset(ent_mempool.buffer, 0, ent_mempool.elem_size*ent_mempool.max_size);
        sc_queue_init(&ent_mempool.free_list);
        ent_mempool.use_list = (bool *)calloc(ent_mempool.max_size, sizeof(bool));
        for (size_t i = 0; i < ent_mempool.max_size; ++i)
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
        for (size_t i = 0; i < N_COMPONENTS; ++i)
        {
            free(comp_mempools[i].use_list);
            sc_queue_term(&comp_mempools[i].free_list);
        }
        free(ent_mempool.use_list);
        sc_queue_term(&ent_mempool.free_list);

        for (int i = 0; i < MAX_COMP_POOL_SIZE; ++i)
        {
            sc_map_term_64(&entity_buffer[i].components);
        }
        pool_inited = false;
    }
}

Entity_t* new_entity_from_mempool(unsigned long* e_idx_ptr)
{
    if(sc_queue_empty(&ent_mempool.free_list)) return NULL;
    unsigned long e_idx = sc_queue_del_first(&ent_mempool.free_list);
    *e_idx_ptr = e_idx;
    ent_mempool.use_list[e_idx] = true;
    Entity_t* ent = entity_buffer + e_idx;
    sc_map_clear_64(&ent->components);
    ent->m_alive = true;
    ent->m_tag = NO_ENT_TAG;
    return ent;
}

Entity_t* get_entity_wtih_id(unsigned long idx)
{
    if (!ent_mempool.use_list[idx]) return NULL;
    return entity_buffer + idx;
}

void free_entity_to_mempool(unsigned long idx)
{
    if (ent_mempool.use_list[idx])
    {
        ent_mempool.use_list[idx] = false;
        sc_queue_add_first(&ent_mempool.free_list, idx);
    }
}

void* new_component_from_mempool(ComponentEnum_t comp_type, unsigned long* idx)
{
    void* comp = NULL;
    assert(comp_type < N_COMPONENTS);
    if(!sc_queue_empty(&comp_mempools[comp_type].free_list))
    {
        *idx = sc_queue_del_first(&comp_mempools[comp_type].free_list);
        comp_mempools[comp_type].use_list[*idx] = true;
        comp = comp_mempools[comp_type].buffer + (*idx * comp_mempools[comp_type].elem_size);
        memset(comp, 0, comp_mempools[comp_type].elem_size);
    }
    return comp;
}

void* get_component_wtih_id(ComponentEnum_t comp_type, unsigned long idx)
{
    void * comp = NULL;
    assert(comp_type < N_COMPONENTS);
    if (comp_mempools[comp_type].use_list[idx])
    {
        comp = comp_mempools[comp_type].buffer + (idx * comp_mempools[comp_type].elem_size);
    }
    return comp;
}

void free_component_to_mempool(ComponentEnum_t comp_type, unsigned long idx)
{
    assert(comp_type < N_COMPONENTS);
    // This just free the component from the memory pool
    if (comp_mempools[comp_type].use_list[idx])
    {
        comp_mempools[comp_type].use_list[idx] = false;
        sc_queue_add_last(&comp_mempools[comp_type].free_list, idx);
    }
}

void print_mempool_stats(char* buffer)
{
    for (size_t i = 0; i < N_COMPONENTS; ++i)
    {
        buffer += sprintf(
            buffer, "%lu: %lu/%lu\n",
            i, sc_queue_size(&comp_mempools[i].free_list), comp_mempools[i].max_size
        );
    }
}
