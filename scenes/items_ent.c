#include "ent_impl.h"
#include "constants.h"

Entity_t* create_crate(EntityManager_t* ent_manager, Assets_t* assets, bool metal)
{
    Entity_t* p_crate = add_entity(ent_manager, CRATES_ENT_TAG);
    CBBox_t* p_bbox = add_component(ent_manager, p_crate, CBBOX_COMP_T);

    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = !metal;

    add_component(ent_manager, p_crate, CTRANSFORM_COMP_T);
    add_component(ent_manager, p_crate, CMOVEMENTSTATE_T);
    add_component(ent_manager, p_crate, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(ent_manager, p_crate, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->fragile = !metal;
    return p_crate;
}
