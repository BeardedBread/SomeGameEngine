#ifndef __MEMPOOL_H
#define __MEMPOOL_H
#include "entity.h"
#include "components.h"
#define MAX_COMP_POOL_SIZE 1024
void init_memory_pools(void);
void free_memory_pools(void);

Entity_t* new_entity_from_mempool(unsigned long* e_idx_ptr);
Entity_t* get_entity_wtih_id(unsigned long idx);
void free_entity_to_mempool(unsigned long idx);

void* new_component_from_mempool(ComponentEnum_t comp_type, unsigned long* idx);
void* get_component_wtih_id(ComponentEnum_t comp_type, unsigned long idx);
void free_component_to_mempool(ComponentEnum_t comp_type, unsigned long idx);

void print_mempool_stats(char* buffer);
#endif //__MEMPOOL_H
