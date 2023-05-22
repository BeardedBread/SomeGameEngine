#include "mempool.h"
//#include "sc/queue/sc_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

typedef struct ULongCircBuffer {
    unsigned long* buffer;     // data buffer
    unsigned long* buffer_end; // end of data buffer
    uint32_t capacity;  // maximum number of items in the buffer
    uint32_t count;     // number of items in the buffer
    unsigned long* head;       // pointer to head
    unsigned long* tail;       // pointer to tail
}ULongCircBuffer_t;

static void cb_init(ULongCircBuffer_t* cb, size_t capacity)
{
    cb->buffer = (unsigned long*)malloc(capacity * sizeof(unsigned long));
    assert(cb->buffer != NULL);

    cb->buffer_end = cb->buffer + capacity;
    cb->capacity = capacity;
    cb->count = 0;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
}

static void cb_free(ULongCircBuffer_t* cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

static bool cb_pop_front(ULongCircBuffer_t* cb, unsigned long* item)
{
    if (cb->count == 0) return false;

    *item = *cb->tail;
    cb->tail++;
    if(cb->tail == cb->buffer_end) cb->tail = cb->buffer;
    cb->count--;
    return true;
}

static bool cb_push_back(ULongCircBuffer_t* cb, unsigned long item)
{
    if(cb->count == cb->capacity) return false;
    *(cb->head) = item;
    cb->head++;
    if(cb->head == cb->buffer_end) cb->head = cb->buffer;
    cb->count++;
    return true;
}

typedef struct MemPool {
    void * const buffer;
    const unsigned long max_size;
    const unsigned long elem_size;
    bool *use_list;
    ULongCircBuffer_t free_list;
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
            cb_init(&comp_mempools[i].free_list, comp_mempools[i].max_size);
            for (size_t j = 0; j < comp_mempools[i].max_size; ++j)
            {
                comp_mempools[i].free_list.buffer[j] = j;
            }
            comp_mempools[i].free_list.count = comp_mempools[i].max_size;
        }

        memset(ent_mempool.buffer, 0, ent_mempool.elem_size*ent_mempool.max_size);
        cb_init(&ent_mempool.free_list, ent_mempool.max_size);
        ent_mempool.use_list = (bool *)calloc(ent_mempool.max_size, sizeof(bool));
        for (size_t i = 0; i < ent_mempool.max_size; ++i)
        {
            entity_buffer[i].m_id = i;
            ent_mempool.free_list.buffer[i] = i;
        }
        ent_mempool.free_list.count = ent_mempool.max_size;
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
            cb_free(&comp_mempools[i].free_list);
        }
        free(ent_mempool.use_list);
        cb_free(&ent_mempool.free_list);

        pool_inited = false;
    }
}

Entity_t* new_entity_from_mempool(unsigned long* e_idx_ptr)
{
    unsigned long e_idx;
    if (!cb_pop_front(&ent_mempool.free_list, &e_idx)) return NULL;

    *e_idx_ptr = e_idx;
    ent_mempool.use_list[e_idx] = true;
    Entity_t* ent = entity_buffer + e_idx;
    for (size_t j = 0; j< N_COMPONENTS; j++)
    {
        ent->components[j] = MAX_COMP_POOL_SIZE;
    }
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
        cb_push_back(&ent_mempool.free_list, idx);
    }
}

void* new_component_from_mempool(ComponentEnum_t comp_type, unsigned long* idx)
{
    void* comp = NULL;
    assert(comp_type < N_COMPONENTS);
    if (cb_pop_front(&comp_mempools[comp_type].free_list, idx))
    {
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
        cb_push_back(&comp_mempools[comp_type].free_list, idx);
    }
}

void print_mempool_stats(char* buffer)
{
    for (size_t i = 0; i < N_COMPONENTS; ++i)
    {
        buffer += sprintf(
            buffer, "%lu: %u/%u\n",
            i, comp_mempools[i].free_list.count, comp_mempools[i].free_list.capacity
        );
    }
}
