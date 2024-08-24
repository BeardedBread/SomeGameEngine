#ifndef __MEMPOOL_H
#define __MEMPOOL_H
#include "EC.h"
#include "sc/queue/sc_queue.h"
void init_memory_pools(void);
void free_memory_pools(void);

typedef struct MemPool {
    void * const buffer;
    const unsigned long max_size;
    const unsigned long elem_size;
    bool *use_list;
    struct sc_queue_32 free_list;
} MemPool_t;

// Game needs to implement this somewhere
extern MemPool_t comp_mempools[N_COMPONENTS];

Entity_t* new_entity_from_mempool(unsigned long* e_idx_ptr);
Entity_t* get_entity_wtih_id(unsigned long idx);
void free_entity_to_mempool(unsigned long idx);

void* new_component_from_mempool(unsigned int comp_type, unsigned long* idx);
void* get_component_wtih_id(unsigned int comp_type, unsigned long idx);
void free_component_to_mempool(unsigned int comp_type, unsigned long idx);

void print_mempool_stats(char* buffer);
uint32_t get_num_of_free_entities(void);

#define DEFINE_COMP_MEMPOOL_BUF(type, n) \
    static type type##_buf[n]; \
    const unsigned long type##_CNT = n; \

#define ADD_COMP_MEMPOOL(type) \
    {type##_buf, type##_CNT, sizeof(type), NULL, {0}}, \

#define BEGIN_DEFINE_COMP_MEMPOOL \
    DEFINE_COMP_MEMPOOL_BUF(CBBox_t, MAX_COMP_POOL_SIZE); \
    DEFINE_COMP_MEMPOOL_BUF(CTransform_t, MAX_COMP_POOL_SIZE); \
    DEFINE_COMP_MEMPOOL_BUF(CTileCoord_t, MAX_COMP_POOL_SIZE); \
    MemPool_t comp_mempools[N_COMPONENTS] = { \
        ADD_COMP_MEMPOOL(CBBox_t) \
        ADD_COMP_MEMPOOL(CTransform_t) \
        ADD_COMP_MEMPOOL(CTileCoord_t) \

#define END_DEFINE_COMP_MEMPOOL };

#endif //__MEMPOOL_H
