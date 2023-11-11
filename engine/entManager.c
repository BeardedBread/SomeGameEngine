#include "mempool.h"

void init_entity_manager(EntityManager_t* p_manager)
{
    sc_map_init_64v(&p_manager->entities, MAX_COMP_POOL_SIZE, 0);
    for (size_t i = 0; i < N_COMPONENTS; ++i)
    {
        sc_map_init_64v(p_manager->component_map + i, MAX_COMP_POOL_SIZE, 0);
    }
    memset(p_manager->tag_map_inited, 0, sizeof(p_manager->tag_map_inited));
    sc_queue_init(&p_manager->to_add);
    sc_queue_init(&p_manager->to_remove);
    sc_queue_init(&p_manager->to_update);
}

void init_entity_tag_map(EntityManager_t* p_manager, unsigned int tag_number, unsigned int initial_size)
{
    if (tag_number >= N_TAGS) return;

    sc_map_init_64v(p_manager->entities_map + tag_number, initial_size, 0);
    p_manager->tag_map_inited[tag_number] = true;
}

void update_entity_manager(EntityManager_t* p_manager)
{
    // This will only update the entity map of the manager
    // It does not make new entities, but will free entity
    // New entities are assigned during add_entity
	unsigned long e_idx;
    
    sc_queue_foreach (&p_manager->to_add, e_idx)
    {
        Entity_t *p_entity = get_entity_wtih_id(e_idx);
        sc_map_put_64v(&p_manager->entities, e_idx, (void *)p_entity);
        if (p_manager->tag_map_inited[p_entity->m_tag])
        {
            sc_map_put_64v(&p_manager->entities_map[p_entity->m_tag], e_idx, (void *)p_entity);
        }
    }
    sc_queue_clear(&p_manager->to_add);

    sc_queue_foreach (&p_manager->to_remove, e_idx)
    {
        Entity_t *p_entity = (Entity_t *)sc_map_get_64v(&p_manager->entities, e_idx);
        if (!p_entity) continue;
        for (size_t i = 0; i < N_COMPONENTS; ++i)
        {
            if (p_entity->components[i] == MAX_COMP_POOL_SIZE) continue;
            free_component_to_mempool(i, p_entity->components[i]);
            sc_map_del_64v(&p_manager->component_map[i], e_idx);
            p_entity->components[i] = MAX_COMP_POOL_SIZE;
        }
        if (p_manager->tag_map_inited[p_entity->m_tag])
        {
            sc_map_del_64v(&p_manager->entities_map[p_entity->m_tag], e_idx);
        }
        free_entity_to_mempool(e_idx);
        sc_map_del_64v(&p_manager->entities, e_idx);
    }
    sc_queue_clear(&p_manager->to_remove);

    struct EntityUpdateEventInfo evt;
    sc_queue_foreach (&p_manager->to_update, evt)
    {
        sc_map_get_64v(&p_manager->entities, evt.e_id);
        if(!sc_map_found(&p_manager->entities)) continue;
        switch(evt.evt_type)
        {
            case COMP_ADDTION:
                sc_map_put_64v(&p_manager->component_map[evt.comp_type], evt.e_id, get_component_wtih_id(evt.comp_type, evt.c_id));
            break;
            case COMP_DELETION:
                sc_map_del_64v(&p_manager->component_map[evt.comp_type], evt.e_id);
                free_component_to_mempool(evt.comp_type, evt.c_id);
            break;
        }
    }
    sc_queue_clear(&p_manager->to_update);
}

void clear_entity_manager(EntityManager_t* p_manager)
{
    unsigned long e_id;
    Entity_t* p_ent;
    sc_map_foreach (&p_manager->entities, e_id, p_ent) 
    {
        remove_entity(p_manager, e_id);
    }
    update_entity_manager(p_manager);
}

void free_entity_manager(EntityManager_t* p_manager)
{
    clear_entity_manager(p_manager);
    sc_map_term_64v(&p_manager->entities);
    for (size_t i = 0; i < N_COMPONENTS; ++i)
    {
        sc_map_term_64v(p_manager->component_map + i);
    }
    for (size_t i = 0; i < N_TAGS; ++i)
    {
        if (p_manager->tag_map_inited[i])
        {
            sc_map_term_64v(p_manager->entities_map + i);
        }
    }
    sc_queue_term(&p_manager->to_add);
    sc_queue_term(&p_manager->to_remove);
    sc_queue_term(&p_manager->to_update);
}

Entity_t *add_entity(EntityManager_t* p_manager, unsigned int tag)
{
    unsigned long e_idx = 0;
    Entity_t* p_ent = new_entity_from_mempool(&e_idx);
    if (p_ent == NULL) return NULL;

    p_ent->spawn_pos = (Vector2){0, 0};
    p_ent->m_tag = tag;
    sc_queue_add_last(&p_manager->to_add, e_idx);
    p_ent->manager = p_manager;
    return p_ent;
}

void remove_entity(EntityManager_t* p_manager, unsigned long id)
{
    Entity_t* p_entity = sc_map_get_64v(&p_manager->entities, id);
    if(sc_map_found(&p_manager->entities))
    {
        // This only marks the entity for deletion
        // Does not free entity. This is done during the update
        if (p_entity->m_alive)
        {
            p_entity->m_alive = false;
        }
    }
    // Queue anyways because added entity may be in update queue
    sc_queue_add_last(&p_manager->to_remove, id);
}

Entity_t* get_entity(EntityManager_t* p_manager, unsigned long id)
{
    Entity_t* p_entity = sc_map_get_64v(&p_manager->entities, id);
    if(!sc_map_found(&p_manager->entities)) return NULL;
    return p_entity;
}

void* add_component(Entity_t* p_entity, ComponentEnum_t comp_type)
{
    if (p_entity->components[comp_type] == MAX_COMP_POOL_SIZE)
    {

        unsigned long comp_idx = 0;
        void* p_comp = new_component_from_mempool(comp_type, &comp_idx);
        if (p_comp)
        {
            p_entity->components[comp_type] = comp_idx;
            struct EntityUpdateEventInfo evt = (struct EntityUpdateEventInfo){p_entity->m_id, comp_type, comp_idx, COMP_ADDTION};
            sc_queue_add_last(&p_entity->manager->to_update, evt);
        }
        return p_comp;
    }

    return get_component(p_entity, comp_type);
}

void* get_component(Entity_t *p_entity, ComponentEnum_t comp_type)
{
    unsigned long comp_type_idx = (unsigned long)comp_type;
    unsigned long c_idx = p_entity->components[comp_type_idx];
    if (c_idx == MAX_COMP_POOL_SIZE) return NULL;
    return get_component_wtih_id(comp_type, c_idx);
}

void remove_component(Entity_t *p_entity, ComponentEnum_t comp_type)
{
    if (p_entity->components[comp_type] == MAX_COMP_POOL_SIZE) return;
    struct EntityUpdateEventInfo evt = (struct EntityUpdateEventInfo){p_entity->m_id, comp_type, p_entity->components[comp_type] , COMP_DELETION};
    p_entity->components[comp_type] = MAX_COMP_POOL_SIZE;
    sc_queue_add_last(&p_entity->manager->to_update, evt);
}
