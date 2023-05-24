#include "ent_impl.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>
#include "raymath.h"

#define N_PLAYER_SPRITES 7
enum PlayerSpriteEnum
{
    SPR_PLAYER_STAND = 0,
    SPR_PLAYER_RUN,
    SPR_PLAYER_JUMP,
    SPR_PLAYER_FALL,
    SPR_PLAYER_LADDER,
    SPR_PLAYER_CROUCH,
    SPR_PLAYER_SWIM,
};

static SpriteRenderInfo_t player_sprite_map[N_PLAYER_SPRITES] = {0};

static unsigned int player_sprite_transition_func(Entity_t* ent)
{
    CTransform_t* p_ctrans = get_component(ent, CTRANSFORM_COMP_T);
    CMovementState_t* p_move = get_component(ent, CMOVEMENTSTATE_T);
    CSprite_t* p_spr = get_component(ent, CSPRITE_T);
    CPlayerState_t* p_plr = get_component(ent, CPLAYERSTATE_T);
    if (p_ctrans->velocity.x > 0) p_spr->flip_x = true;
    else if (p_ctrans->velocity.x < 0) p_spr->flip_x = false;

    p_spr->pause = false;

    if (p_move->ground_state & 1)
    {
        if (p_plr->is_crouch)
        {
            return SPR_PLAYER_CROUCH;
        }
        else
        {
            if (Vector2LengthSqr(p_ctrans->velocity) > 1000.0f)
            {
                return SPR_PLAYER_RUN;
            }
            return SPR_PLAYER_STAND;
        }
    }
    else if (p_plr->ladder_state)
    {
        p_spr->pause = Vector2LengthSqr(p_ctrans->velocity) < 10.0f;
        return SPR_PLAYER_LADDER;
    }
    else if (p_move->water_state & 1)
    {
        return SPR_PLAYER_SWIM;
    }
    return (p_ctrans->velocity.y < 0) ? SPR_PLAYER_JUMP : SPR_PLAYER_FALL;
}

Entity_t* create_player(EntityManager_t* ent_manager, Assets_t* assets)
{
    Entity_t* p_ent = add_entity(ent_manager, PLAYER_ENT_TAG);
    CBBox_t* p_bbox = add_component(p_ent, CBBOX_COMP_T);

    set_bbox(p_bbox, PLAYER_WIDTH, PLAYER_HEIGHT);
    add_component(p_ent, CTRANSFORM_COMP_T);

    CJump_t* p_cjump = add_component(p_ent, CJUMP_COMP_T);
    p_cjump->jump_speed = 680;
    p_cjump->jumps = 1;
    p_cjump->max_jumps = 1;
    p_cjump->jump_ready = true;
    add_component(p_ent, CPLAYERSTATE_T);
    add_component(p_ent, CTILECOORD_COMP_T);
    add_component(p_ent, CMOVEMENTSTATE_T);
    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
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
    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = player_sprite_map;
    p_cspr->transition_func = &player_sprite_transition_func;

    return p_ent;
}

bool init_player_creation(const char* info_file, Assets_t* assets)
{
    static bool already_init = false;

    if (already_init) return false;

    FILE* in_file = fopen(info_file, "r");
    if (in_file == NULL)
    {
        printf("Unable to open file %s\n", info_file);
        return false;
    }

    char buffer[256];
    char* tmp;
    size_t line_num = 0;
    uint8_t i = 0;
    while (true)
    {
        tmp = fgets(buffer, 256, in_file);
        if (tmp == NULL) break;
        tmp[strcspn(tmp, "\r\n")] = '\0';

        if (i == N_PLAYER_SPRITES)  break;

        char* name = strtok(buffer, ":");
        char* info_str = strtok(NULL, ":");
        if (name == NULL || info_str == NULL) return false;

        while(*name == ' ' || *name == '\t') name++;
        while(*info_str == ' ' || *info_str == '\t') info_str++;

        Vector2 offset;
        int data_count = sscanf(
            info_str, "%f,%f",
            &offset.x, &offset.y
        );
        if (data_count !=2)
        {
            printf("Unable to parse info for player at line %lu\n", line_num);
            return false;
        }
        Sprite_t* spr = get_sprite(assets, name);
        player_sprite_map[i].sprite = spr;
        player_sprite_map[i].offset = offset;
        i++;
    }
    already_init = true;
    return true;
}
