#include "ent_impl.h"
#include "constants.h"
#include "raymath.h"

static SpriteRenderInfo_t item_sprite_map[20] = {0};

bool init_item_creation(Assets_t* assets)
{
    item_sprite_map[0].sprite = get_sprite(assets, "w_crate");
    item_sprite_map[1].sprite = get_sprite(assets, "m_crate");
    item_sprite_map[2].sprite = get_sprite(assets, "r_arrow");
    item_sprite_map[2].offset = (Vector2){-8, 4};
    item_sprite_map[3].sprite = get_sprite(assets, "u_arrow");
    item_sprite_map[4].sprite = get_sprite(assets, "l_arrow");
    item_sprite_map[5].sprite = get_sprite(assets, "d_arrow");
    item_sprite_map[6].sprite = get_sprite(assets, "bomb");
    item_sprite_map[6].offset = (Vector2){0, -4};
    item_sprite_map[7].sprite = get_sprite(assets, "w_ra_crate");
    item_sprite_map[8].sprite = get_sprite(assets, "m_ra_crate");
    item_sprite_map[9].sprite = get_sprite(assets, "w_ua_crate");
    item_sprite_map[10].sprite = get_sprite(assets, "m_ua_crate");
    item_sprite_map[11].sprite = get_sprite(assets, "w_la_crate");
    item_sprite_map[12].sprite = get_sprite(assets, "m_la_crate");
    item_sprite_map[13].sprite = get_sprite(assets, "w_da_crate");
    item_sprite_map[14].sprite = get_sprite(assets, "m_da_crate");
    item_sprite_map[15].sprite = get_sprite(assets, "w_b_crate");
    item_sprite_map[16].sprite = get_sprite(assets, "m_b_crate");
    item_sprite_map[17].sprite = get_sprite(assets, "explode");
    item_sprite_map[17].offset = (Vector2){-12, -12};
    item_sprite_map[18].sprite = get_sprite(assets, "chest");
    item_sprite_map[19].sprite = get_sprite(assets, "boulder");
    return true;
}


Entity_t* create_crate(EntityManager_t* ent_manager, bool metal, ContainerItem_t item)
{
    Entity_t* p_crate = add_entity(ent_manager, CRATES_ENT_TAG);
    if (p_crate == NULL) return NULL;

    CBBox_t* p_bbox = add_component(p_crate, CBBOX_COMP_T);
    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = false;

    CTransform_t* p_ctransform = add_component(p_crate, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 7;
    p_ctransform->shape_factor = metal ? (Vector2){0.7,0.7} : (Vector2){0.8,0.8} ;
    add_component(p_crate, CMOVEMENTSTATE_T);

    add_component(p_crate, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(p_crate, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = metal ? 2 : 1;
    p_hurtbox->damage_src = -1;

    CSprite_t* p_cspr = add_component(p_crate, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    {
        CContainer_t* p_container = add_component(p_crate, CCONTAINER_T);
        p_container->material = metal? METAL_CONTAINER : WOODEN_CONTAINER;
        p_container->item = item;
    }
    switch(item)
    {
        case CONTAINER_RIGHT_ARROW: p_cspr->current_idx = 7; break;
        case CONTAINER_UP_ARROW: p_cspr->current_idx = 9; break;
        case CONTAINER_LEFT_ARROW: p_cspr->current_idx = 11; break;
        case CONTAINER_DOWN_ARROW: p_cspr->current_idx = 13; break;
        case CONTAINER_BOMB: p_cspr->current_idx = 15; break;
        default: p_cspr->current_idx = 0; break;
    }

    if (metal) p_cspr->current_idx++;

    return p_crate;
}

Entity_t* create_boulder(EntityManager_t* ent_manager)
{
    Entity_t* p_boulder = add_entity(ent_manager, BOULDER_ENT_TAG);
    if (p_boulder == NULL) return NULL;

    CBBox_t* p_bbox = add_component(p_boulder, CBBOX_COMP_T);
    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = false;

    CTransform_t* p_ctransform = add_component(p_boulder, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 5;
    p_ctransform->active = true;
    p_ctransform->shape_factor = (Vector2){0.6, 0.6};
    CMovementState_t* p_move = add_component(p_boulder, CMOVEMENTSTATE_T);
    p_move->ground_state |= 3;

    add_component(p_boulder, CTILECOORD_COMP_T);
    CMoveable_t* p_cmove = add_component(p_boulder, CMOVEABLE_T);
    p_cmove->move_speed = 8;
    CHurtbox_t* p_hurtbox = add_component(p_boulder, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = 2;
    p_hurtbox->damage_src = -1;

    CSprite_t* p_cspr = add_component(p_boulder, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    p_cspr->current_idx = 19;

    return p_boulder;
}

Entity_t* create_arrow(EntityManager_t* ent_manager, uint8_t dir)
{
    Entity_t* p_arrow = add_entity(ent_manager, ARROW_ENT_TAG);
    if (p_arrow == NULL) return NULL;

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
            p_ctransform->velocity.x = -ARROW_SPEED;
            p_cspr->current_idx += 2;
        break;
        case 2:
            p_hitbox->boxes[0] = (Rectangle){TILE_SIZE / 2 - 5, 10, 5, 10};
            p_ctransform->velocity.y = -ARROW_SPEED;
            p_cspr->current_idx += 1;
        break;
        case 3:
            p_hitbox->boxes[0] = (Rectangle){TILE_SIZE / 2 - 5, 10, 5, 10};
            p_ctransform->velocity.y = ARROW_SPEED;
            p_cspr->current_idx += 3;
        break;
        default:
            p_hitbox->boxes[0] = (Rectangle){10, TILE_SIZE / 2 - 5, 10, 5};
            p_ctransform->velocity.x = ARROW_SPEED;
        break;
    }

    return p_arrow;
}

Entity_t* create_bomb(EntityManager_t* ent_manager, Vector2 launch_dir)
{
    Entity_t* p_bomb = add_entity(ent_manager, DESTRUCTABLE_ENT_TAG);
    if (p_bomb == NULL) return NULL;

    add_component(p_bomb, CTILECOORD_COMP_T);
    add_component(p_bomb, CMOVEMENTSTATE_T);
    CHitBoxes_t* p_hitbox = add_component(p_bomb, CHITBOXES_T);
    p_hitbox->n_boxes = 1;
    p_hitbox->boxes[0] = (Rectangle){0, 0, 25, 25};

    p_hitbox->atk = 0;
    p_hitbox->one_hit = true;

    CContainer_t* p_container = add_component(p_bomb, CCONTAINER_T);
    p_container->item = CONTAINER_EXPLOSION;

    CSprite_t* p_cspr = add_component(p_bomb, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    p_cspr->current_idx = 6;

    CTransform_t* p_ctransform = add_component(p_bomb, CTRANSFORM_COMP_T);
    p_ctransform->active = true; 
    p_ctransform->shape_factor = (Vector2){0.1, 0.1};
    p_ctransform->movement_mode = REGULAR_MOVEMENT;
    p_ctransform->position.x += (TILE_SIZE - 25) / 2;
    p_ctransform->position.y += (TILE_SIZE - 25) / 2;

    p_ctransform->velocity = Vector2Scale(Vector2Normalize(launch_dir), 500);

    if (launch_dir.x > 0)
    {
        p_ctransform->position.x += TILE_SIZE/ 2;
    }
    else if (launch_dir.x < 0)
    {
        p_ctransform->position.x -= TILE_SIZE / 2;
    }
    return p_bomb;
}

Entity_t* create_explosion(EntityManager_t* ent_manager)
{
    Entity_t* p_explosion = add_entity(ent_manager, DESTRUCTABLE_ENT_TAG);
    if (p_explosion == NULL) return NULL;

    add_component(p_explosion, CTILECOORD_COMP_T);
    CHitBoxes_t* p_hitbox = add_component(p_explosion, CHITBOXES_T);
    p_hitbox->n_boxes = 1;

    p_hitbox->atk = 3;

    CTransform_t* p_ctransform = add_component(p_explosion, CTRANSFORM_COMP_T);
    p_ctransform->movement_mode = KINEMATIC_MOVEMENT;
    p_ctransform->active = true;
    p_ctransform->position.x -= 16;
    p_ctransform->position.y -= 16;
    p_hitbox->boxes[0] = (Rectangle){0, 0, TILE_SIZE + 32, TILE_SIZE + 32};

    CSprite_t* p_cspr = add_component(p_explosion, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    p_cspr->current_idx = 17;

    CLifeTimer_t* p_clifetimer = add_component(p_explosion, CLIFETIMER_T);
    p_clifetimer->life_time = 3;
    return p_explosion;
}

Entity_t* create_chest(EntityManager_t* ent_manager)
{
    Entity_t* p_chest = add_entity(ent_manager, CHEST_ENT_TAG);
    if (p_chest == NULL) return NULL;

    CBBox_t* p_bbox = add_component(p_chest, CBBOX_COMP_T);
    set_bbox(p_bbox, TILE_SIZE, TILE_SIZE);
    p_bbox->solid = true;
    p_bbox->fragile = true;

    CTransform_t* p_ctransform = add_component(p_chest, CTRANSFORM_COMP_T);
    p_ctransform->grav_delay = 7;
    p_ctransform->shape_factor = (Vector2){0.7,0.7};
    add_component(p_chest, CMOVEMENTSTATE_T);
    add_component(p_chest, CTILECOORD_COMP_T);
    CHurtbox_t* p_hurtbox = add_component(p_chest, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;
    p_hurtbox->def = 4;
    p_hurtbox->damage_src = -1;

    CSprite_t* p_cspr = add_component(p_chest, CSPRITE_T);
    p_cspr->sprites = item_sprite_map;
    p_cspr->current_idx = 18;


    return p_chest;
}

Entity_t* create_level_end(EntityManager_t* ent_manager)
{
    Entity_t* p_flag = add_entity(ent_manager, LEVEL_END_TAG);
    if (p_flag == NULL) return NULL;

    add_component(p_flag, CTRANSFORM_COMP_T);
    return p_flag;
}
