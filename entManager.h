#ifndef __ENTITY_MANAGER_H
#define __ENTITY_MANAGER_H
#include "sc/queue/sc_queue.h"
#include "sc/map/sc_map.h"
#include "mempool.h" // includes entity and components

typedef struct EntityManager
{
    // All fields are Read-Only
    struct sc_map_64v entities;
    struct sc_map_64v component_map[N_COMPONENTS];
    struct sc_queue_uint to_add;
    struct sc_queue_uint to_remove;
}EntityManager_t;

void init_entity_manager(EntityManager_t *p_manager);
void update_entity_manager(EntityManager_t *p_manager);
void clear_entity_manager(EntityManager_t *p_manager);
void free_entity_manager(EntityManager_t *p_manager);

Entity_t *add_entity(EntityManager_t *p_manager, const char *tag);
void remove_entity(EntityManager_t *p_manager, unsigned long id);

void *add_component(EntityManager_t *p_manager, Entity_t *entity, ComponentEnum_t comp_type);
void *get_component(EntityManager_t *p_manager, Entity_t *entity, ComponentEnum_t comp_type);
void remove_component(EntityManager_t *p_manager, Entity_t *entity, ComponentEnum_t comp_type);

#endif // __ENTITY_MANAGER_H
