#include "ent_impl.h"
#include "constants.h"

Entity_t* create_crate(EntityManager_t* ent_manager, Assets_t* assets, bool metal, ContainerItem_t item)
{
    Entity_t* p_crate = add_entity(ent_manager, CRATES_ENT_TAG);
    CBBox_t* p_bbox = add_component(p_crate, CBBOX_COMP_T);

    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = !metal;

    CTransform_t* p_ctransform = add_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 5;
    add_component(p_crate, CMOVEMENTSTATE_T);
    add_component(p_crate, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(p_crate, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = metal ? 2 : 1;

    if (item != CONTAINER_EMPTY)
    {
        CContainer_t* p_container = add_component(p_crate, CCONTAINER_T);
        p_container->item = item;
    }

    return p_crate;
}

Entity_t* create_boulder(EntityManager_t* ent_manager, Assets_t* assets)
{
    Entity_t* p_boulder = add_entity(ent_manager, BOULDER_ENT_TAG);
    CBBox_t* p_bbox = add_component(p_boulder, CBBOX_COMP_T);

    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = false;

    CTransform_t* p_ctransform = add_component(p_boulder, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 5;
    p_ctransform->active = true;
    add_component(p_boulder, CMOVEMENTSTATE_T);
    add_component(p_boulder, CTILECOORD_COMP_T);
    CMoveable_t* p_cmove = add_component(p_boulder, CMOVEABLE_T);
    p_cmove->move_speed = 8;
    CHurtbox_t* p_hurtbox = add_component(p_boulder, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = 2;
    return p_boulder;
}

Entity_t* create_arrow(EntityManager_t* ent_manager, Assets_t* assets, uint8_t dir)
{
    Entity_t* p_arrow = add_entity(ent_manager, ARROW_ENT_TAG);
    add_component(p_arrow, CTILECOORD_COMP_T);
    CHitBoxes_t* p_hitbox = add_component(p_arrow, CHITBOXES_T);
    p_hitbox->n_boxes = 1;
    p_hitbox->boxes[0] = (Rectangle){TILE_SIZE - 5, TILE_SIZE / 2 - 5, 5, 5};
    p_hitbox->atk = 3;
    p_hitbox->one_hit = true;

    CTransform_t* p_ctransform = add_component(p_arrow, CTRANSFORM_COMP_T);
    p_ctransform->movement_mode = KINEMATIC_MOVEMENT;
    p_ctransform->velocity.x = 500;
    p_ctransform->active = true;

    return p_arrow;
}
