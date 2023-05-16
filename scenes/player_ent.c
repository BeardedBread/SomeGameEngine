#include "ent_impl.h"
#include "constants.h"

#define N_PLAYER_SPRITES 2
enum PlayerSpriteEnum
{
    SPR_PLAYER_STAND = 0,
    SPR_PLAYER_RUN
};

static const char* const player_sprite_map[N_PLAYER_SPRITES] = {
    "plr_stand",
    "plr_run",
};

static unsigned int player_sprite_transition_func(Entity_t* ent)
{
    return SPR_PLAYER_RUN;
}

Entity_t* create_player(EntityManager_t* ent_manager, Assets_t* assets)
{
    Entity_t* p_ent = add_entity(ent_manager, PLAYER_ENT_TAG);
    CBBox_t* p_bbox = add_component(ent_manager, p_ent, CBBOX_COMP_T);

    set_bbox(p_bbox, PLAYER_WIDTH, PLAYER_HEIGHT);
    add_component(ent_manager, p_ent, CTRANSFORM_COMP_T);

    CJump_t* p_cjump = add_component(ent_manager, p_ent, CJUMP_COMP_T);
    p_cjump->jump_speed = 680;
    p_cjump->jumps = 1;
    p_cjump->max_jumps = 1;
    p_cjump->jump_ready = true;
    add_component(ent_manager, p_ent, CPLAYERSTATE_T);
    add_component(ent_manager, p_ent, CTILECOORD_COMP_T);
    add_component(ent_manager, p_ent, CMOVEMENTSTATE_T);
    CHitBoxes_t* p_hitbox = add_component(ent_manager, p_ent, CHITBOXES_T);
    p_hitbox->n_boxes = 2;
    p_hitbox->boxes[0] = (Rectangle) {
        .x = 0,
        .y = -1,
        .width = p_bbox->size.x - 1,
        .height = p_bbox->size.y + 2,
    };
    p_hitbox->boxes[1] = (Rectangle) {
        .x =  -1,
        .y = 0,
        .width = p_bbox->size.x + 2,
        .height = p_bbox->size.y - 1,
    };
    CSprite_t* p_cspr = add_component(ent_manager, p_ent, CSPRITE_T);
    p_cspr->sprite = get_sprite(assets, "plr_stand");
    p_cspr->offset = (Vector2){0, -20};
    p_cspr->sprites_map = player_sprite_map;
    p_cspr->transition_func = &player_sprite_transition_func;

    return p_ent;
}
