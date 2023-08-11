#include "ent_impl.h"
#include "constants.h"
#include "raymath.h"

static SpriteRenderInfo_t item_sprite_map[3] = {0};

bool init_item_creation(Assets_t* assets)
{
    item_sprite_map[0].sprite = get_sprite(assets, "w_crate");
    item_sprite_map[1].sprite = get_sprite(assets, "m_crate");
    item_sprite_map[2].sprite = get_sprite(assets, "arrow");
    item_sprite_map[2].offset = (Vector2){-8, 4};
    return true;
}


Entity_t* create_crate(EntityManager_t* ent_manager, Assets_t* assets, bool metal, ContainerItem_t item)
{
    Entity_t* p_crate = add_entity(ent_manager, CRATES_ENT_TAG);
    CBBox_t* p_bbox = add_component(p_crate, CBBOX_COMP_T);

    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = !metal;

    if (item == CONTAINER_EMPTY)
    {
        CSprite_t* p_cspr = add_component(p_crate, CSPRITE_T);
        p_cspr->sprites = item_sprite_map;
        p_cspr->current_idx = metal? 1 : 0;
    }

    CTransform_t* p_ctransform = add_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 7;
    p_ctransform->shape_factor = metal ? (Vector2){0.7,0.7} : (Vector2){0.8,0.8} ;
    add_component(p_crate, CMOVEMENTSTATE_T);
    add_component(p_crate, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(p_crate, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = metal ? 2 : 1;
    p_hurtbox->damage_src = -1;

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
    p_ctransform->shape_factor = (Vector2){0.6, 0.6};
    add_component(p_boulder, CMOVEMENTSTATE_T);
    add_component(p_boulder, CTILECOORD_COMP_T);
    CMoveable_t* p_cmove = add_component(p_boulder, CMOVEABLE_T);
    p_cmove->move_speed = 8;
    CHurtbox_t* p_hurtbox = add_component(p_boulder, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = 2;
    p_hurtbox->damage_src = -1;
    return p_boulder;
}

Entity_t* create_arrow(EntityManager_t* ent_manager, Assets_t* assets, uint8_t dir)
{
    Entity_t* p_arrow = add_entity(ent_manager, DESTRUCTABLE_ENT_TAG);
    add_component(p_arrow, CTILECOORD_COMP_T);
    CHitBoxes_t* p_hitbox = add_component(p_arrow, CHITBOXES_T);
    p_hitbox->n_boxes = 1;

    p_hitbox->atk = 3;
    p_hitbox->one_hit = true;

    CTransform_t* p_ctransform = add_component(p_arrow, CTRANSFORM_COMP_T);
    p_ctransform->movement_mode = KINEMATIC_MOVEMENT;
    p_ctransform->active = true;

    CSprite_t* p_cspr = add_component(p_arrow, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    p_cspr->current_idx = 2;
    //p_hitbox->boxes[0] = (Rectangle){TILE_SIZE - 5, TILE_SIZE / 2 - 5, 5, 5};
    switch(dir)
    {
        case 0:
            p_hitbox->boxes[0] = (Rectangle){10, TILE_SIZE / 2 - 5, 10, 5};
            p_ctransform->velocity.x = -250;
        break;
        case 2:
            p_hitbox->boxes[0] = (Rectangle){TILE_SIZE / 2 - 5, 10, 5, 10};
            p_ctransform->velocity.y = -250;
        break;
        case 3:
            p_hitbox->boxes[0] = (Rectangle){TILE_SIZE / 2 - 5, 10, 5, 10};
            p_ctransform->velocity.y = 250;
        break;
        default:
            p_hitbox->boxes[0] = (Rectangle){10, TILE_SIZE / 2 - 5, 10, 5};
            p_ctransform->velocity.x = 250;
        break;
    }

    return p_arrow;
}

Entity_t* create_bomb(EntityManager_t* ent_manager, Assets_t* assets, Vector2 launch_dir)
{
    Entity_t* p_bomb = add_entity(ent_manager, DESTRUCTABLE_ENT_TAG);
    add_component(p_bomb, CTILECOORD_COMP_T);
    add_component(p_bomb, CMOVEMENTSTATE_T);
    CHitBoxes_t* p_hitbox = add_component(p_bomb, CHITBOXES_T);
    p_hitbox->n_boxes = 1;
    p_hitbox->boxes[0] = (Rectangle){0, 0, 25, 25};

    p_hitbox->atk = 0;
    p_hitbox->one_hit = true;

    CContainer_t* p_container = add_component(p_bomb, CCONTAINER_T);
    p_container->item = CONTAINER_EXPLOSION;

    CTransform_t* p_ctransform = add_component(p_bomb, CTRANSFORM_COMP_T);
    p_ctransform->active = true; 
    p_ctransform->shape_factor = (Vector2){0.1, 0.1};
    p_ctransform->movement_mode = REGULAR_MOVEMENT;
    p_ctransform->position.x += (TILE_SIZE - 25) / 2;
    p_ctransform->position.y += (TILE_SIZE - 25) / 2;

    p_ctransform->velocity = Vector2Scale(Vector2Normalize(launch_dir), 500);
    return p_bomb;
}

Entity_t* create_explosion(EntityManager_t* ent_manager, Assets_t* assets)
{
    Entity_t* p_explosion = add_entity(ent_manager, DESTRUCTABLE_ENT_TAG);
    add_component(p_explosion, CTILECOORD_COMP_T);
    CHitBoxes_t* p_hitbox = add_component(p_explosion, CHITBOXES_T);
    p_hitbox->n_boxes = 1;

    p_hitbox->atk = 3;

    CTransform_t* p_ctransform = add_component(p_explosion, CTRANSFORM_COMP_T);
    p_ctransform->movement_mode = KINEMATIC_MOVEMENT;
    p_ctransform->active = true;
    p_ctransform->position.x -= 24;
    p_ctransform->position.y -= 24;
    p_hitbox->boxes[0] = (Rectangle){0, 0, TILE_SIZE + 48, TILE_SIZE + 48};

    CLifeTimer_t* p_clifetimer = add_component(p_explosion, CLIFETIMER_T);
    p_clifetimer->life_time = 3;
    return p_explosion;
}
