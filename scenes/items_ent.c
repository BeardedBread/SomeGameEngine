#include "ent_impl.h"
#include "constants.h"

Entity_t* create_crate(EntityManager_t* ent_manager, Assets_t* assets, bool metal)
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
