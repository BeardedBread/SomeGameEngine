#include "ent_impl.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>
#include "raymath.h"

#define N_PLAYER_SPRITES 11
enum PlayerSpriteEnum
{
    SPR_PLAYER_STAND = 0,
    SPR_PLAYER_RUN,
    SPR_PLAYER_JUMP,
    SPR_PLAYER_FALL,
    SPR_PLAYER_LADDER,
    SPR_PLAYER_CROUCH,
    SPR_PLAYER_CRMOVE,
    SPR_PLAYER_SWIM,
    SPR_PLAYER_USWIM,
    SPR_PLAYER_DSWIM,
    SPR_PLAYER_DEAD,
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
        if (Vector2LengthSqr(p_ctrans->velocity) > 1000.0f)
        {
            return (p_plr->is_crouch) ? SPR_PLAYER_CRMOVE : SPR_PLAYER_RUN;
        }
        else
        {
            return (p_plr->is_crouch) ? SPR_PLAYER_CROUCH : SPR_PLAYER_STAND;
        }
    }
    else if (p_plr->ladder_state)
    {
        p_spr->pause = Vector2LengthSqr(p_ctrans->velocity) < 10.0f;
        return SPR_PLAYER_LADDER;
    }
    else if (p_move->water_state & 1)
    {
        if (p_ctrans->velocity.y > 50) return SPR_PLAYER_DSWIM;

        if (p_ctrans->velocity.y < -50) return SPR_PLAYER_USWIM;

        return SPR_PLAYER_SWIM;
    }
    return (p_ctrans->velocity.y < 0) ? SPR_PLAYER_JUMP : SPR_PLAYER_FALL;
}

Entity_t* create_player(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, PLAYER_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CBBox_t* p_bbox = add_component(p_ent, CBBOX_COMP_T);

    set_bbox(p_bbox, PLAYER_WIDTH, PLAYER_HEIGHT);
    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_COMP_T);
    p_ct->active = true;
    p_ct->shape_factor = (Vector2){1, 1};

    CJump_t* p_cjump = add_component(p_ent, CJUMP_COMP_T);
    p_cjump->jump_speed = 680;
    p_cjump->jumps = 1;
    p_cjump->max_jumps = 1;
    p_cjump->jump_ready = true;
    add_component(p_ent, CPLAYERSTATE_T);
    add_component(p_ent, CTILECOORD_COMP_T);

    CMovementState_t* p_move = add_component(p_ent, CMOVEMENTSTATE_T);
    p_move->ground_state |= 3;
    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
    p_hitbox->n_boxes = 2;
    p_hitbox->boxes[0] = (Rectangle) {
        .x = 0,
        .y = -2,
        .width = p_bbox->size.x,
        .height = p_bbox->size.y + 4,
    };
    p_hitbox->boxes[1] = (Rectangle) {
        .x =  -2,
        .y = 0,
        .width = p_bbox->size.x + 4,
        .height = p_bbox->size.y,
    };
    p_hitbox->atk = 2;
    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = p_bbox->size;

    CAirTimer_t* p_air = add_component(p_ent, CAIRTIMER_T);
    p_air->max_count = 10;
    p_air->curr_count = 10;
    p_air->max_ftimer = 1.0f;
    p_air->decay_rate = 1.0f;

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = player_sprite_map;
    p_cspr->transition_func = &player_sprite_transition_func;

    CEmitter_t* p_emitter = add_component(p_ent, CEMITTER_T);
    p_emitter->offset = (Vector2){7,0};

    return p_ent;
}

Entity_t* create_dead_player(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, NO_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_COMP_T);
    p_ct->active = true;
    p_ct->shape_factor = (Vector2){1, 1};
    p_ct->velocity.y = -450;

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = player_sprite_map;
    p_cspr->current_idx = SPR_PLAYER_DEAD;

    add_component(p_ent, CMOVEMENTSTATE_T);

    return p_ent;
}

static AnchorPoint_t parse_anchor_symbol(const char symbol[2])
{
    if (symbol[0] == 't')
    {
        if (symbol[1] == 'l') return AP_TOP_LEFT;
        if (symbol[1] == 'c') return AP_TOP_CENTER;
        if (symbol[1] == 'r') return AP_TOP_RIGHT;
    }
    else if (symbol[0] == 'm')
    {
        if (symbol[1] == 'l') return AP_MID_LEFT;
        if (symbol[1] == 'c') return AP_MID_CENTER;
        if (symbol[1] == 'r') return AP_MID_RIGHT;
    }
    else if (symbol[0] == 'b')
    {
        if (symbol[1] == 'l') return AP_BOT_LEFT;
        if (symbol[1] == 'c') return AP_BOT_CENTER;
        if (symbol[1] == 'r') return AP_BOT_RIGHT;
    }

    return AP_TOP_LEFT;
}

static bool init_player_file(FILE* in_file, Assets_t* assets)
{
    static bool already_init = false;

    if (already_init) return false;

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
        char src_ap_symbol[3];
        char dest_ap_symbol[3];
        int data_count = sscanf(
            info_str, "%f,%f,%2s,%2s",
            &offset.x, &offset.y, src_ap_symbol, dest_ap_symbol
        );
        if (data_count != 4)
        {
            printf("Unable to parse info for player at line %lu\n", line_num);
            return false;
        }
        Sprite_t* spr = get_sprite(assets, name);
        spr->anchor = Vector2Scale(spr->frame_size, 0.5f);
        player_sprite_map[i].sprite = spr;
        player_sprite_map[i].offset = offset;
        player_sprite_map[i].src_anchor = parse_anchor_symbol(src_ap_symbol);
        player_sprite_map[i].dest_anchor = parse_anchor_symbol(dest_ap_symbol);
        i++;
    }
    already_init = true;
    return true;
}

bool init_player_creation(const char* info_file, Assets_t* assets)
{
    FILE* in_file = fopen(info_file, "r");
    if (in_file == NULL)
    {
        printf("Unable to open file %s\n", info_file);
        return false;
    }
    bool okay = init_player_file(in_file, assets);
    fclose(in_file);

    return okay;
}

bool init_player_creation_rres(const char* rres_fname, const char* file, Assets_t* assets)
{
    RresFileInfo_t rres_file;
    rres_file.dir = rresLoadCentralDirectory(rres_fname);
    rres_file.fname = rres_fname;

    if (rres_file.dir.count == 0)
    {
        puts("Empty central directory");
        return false;
    }

    unsigned int res_id = rresGetResourceId(rres_file.dir, file);
    rresResourceChunk chunk = rresLoadResourceChunk(rres_file.fname, res_id);
    
    bool okay = false;
    if (chunk.info.id == res_id)
    {
        FILE* in_file = fmemopen(chunk.data.raw, chunk.info.baseSize, "rb");
        okay = init_player_file(in_file, assets);
        fclose(in_file);
    }

    rresUnloadResourceChunk(chunk);
    rresUnloadCentralDirectory(rres_file.dir);
    return okay;
}
