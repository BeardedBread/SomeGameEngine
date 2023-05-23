#include "assets.h"
#include "assert.h"

#define MAX_TEXTURES 16
#define MAX_SPRITES 16
#define MAX_SOUNDS 16
#define MAX_FONTS 4
#define MAX_NAME_LEN 32
uint8_t free_idx[4] = {0};

// Hard limit number of 
typedef struct TextureData
{
    Texture2D texture;
    char name[MAX_NAME_LEN];
}TextureData_t;
typedef struct SpriteData
{
    Sprite_t sprite;
    char name[MAX_NAME_LEN];
}SpriteData_t;
typedef struct FontData
{
    Font font;
    char name[MAX_NAME_LEN];
}FontData_t;
typedef struct SoundData
{
    Sound sound;
    char name[MAX_NAME_LEN];
}SoundData_t;


static TextureData_t textures[MAX_TEXTURES];
static FontData_t fonts[MAX_FONTS];
static SoundData_t sfx[MAX_SOUNDS];
static SpriteData_t sprites[MAX_SPRITES];

// Maybe need a circular buffer??
Texture2D* add_texture(Assets_t* assets, const char* name, const char* path)
{
    uint8_t tex_idx = free_idx[0];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTexture(path);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    free_idx[0]++;
    return &textures[tex_idx].texture;
}

Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture)
{
    uint8_t spr_idx = free_idx[1];
    assert(spr_idx < MAX_SPRITES);
    memset(sprites + spr_idx, 0, sizeof(Sprite_t));
    sprites[spr_idx].sprite.texture = texture;
    strncpy(sprites[spr_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sprites, sprites[spr_idx].name, spr_idx);
    free_idx[1]++;
    return &sprites[spr_idx].sprite;
}

Sound* add_sound(Assets_t* assets, const char* name, const char* path)
{
    uint8_t snd_idx = free_idx[2];
    assert(snd_idx < MAX_SOUNDS);
    sfx[snd_idx].sound = LoadSound(path);
    strncpy(sfx[snd_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sounds, sfx[snd_idx].name, snd_idx);
    free_idx[2]++;
    return &sfx[snd_idx].sound;
}

Font* add_font(Assets_t* assets, const char* name, const char* path)
{
    uint8_t fnt_idx = free_idx[3];
    assert(fnt_idx < MAX_FONTS);
    fonts[fnt_idx].font = LoadFont(path);
    strncpy(fonts[fnt_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_fonts, fonts[fnt_idx].name, fnt_idx);
    free_idx[3]++;
    return &fonts[fnt_idx].font;
}

void init_assets(Assets_t* assets)
{
    sc_map_init_s64(&assets->m_fonts, MAX_FONTS, 0);
    sc_map_init_s64(&assets->m_sprites, MAX_SPRITES, 0);
    sc_map_init_s64(&assets->m_textures, MAX_TEXTURES, 0);
    sc_map_init_s64(&assets->m_sounds, MAX_SOUNDS, 0);
}

void free_all_assets(Assets_t* assets)
{
    sc_map_clear_s64(&assets->m_textures);
    sc_map_clear_s64(&assets->m_fonts);
    sc_map_clear_s64(&assets->m_sounds);
    sc_map_clear_s64(&assets->m_sprites);
    memset(free_idx, 0, sizeof(free_idx));
}

void term_assets(Assets_t* assets)
{
    free_all_assets(assets);
    sc_map_term_s64(&assets->m_textures);
    sc_map_term_s64(&assets->m_fonts);
    sc_map_term_s64(&assets->m_sounds);
    sc_map_term_s64(&assets->m_sprites);
}

Texture2D* get_texture(Assets_t* assets, const char* name)
{
    uint8_t tex_idx = sc_map_get_s64(&assets->m_textures, name);
    if (sc_map_found(&assets->m_textures))
    {
        return &textures[tex_idx].texture;
    }
    return NULL;
}

Sprite_t* get_sprite(Assets_t* assets, const char* name)
{
    uint8_t spr_idx = sc_map_get_s64(&assets->m_sprites, name);
    if (sc_map_found(&assets->m_sprites))
    {
        return &sprites[spr_idx].sprite;
    }
    return NULL;
}

Sound* get_sound(Assets_t* assets, const char* name)
{
    uint8_t snd_idx = sc_map_get_s64(&assets->m_sounds, name);
    if (sc_map_found(&assets->m_sounds))
    {
        return &sfx[snd_idx].sound;
    }
    return NULL;
}

Font* get_font(Assets_t* assets, const char* name)
{
    uint8_t fnt_idx = sc_map_get_s64(&assets->m_fonts, name);
    if (sc_map_found(&assets->m_fonts))
    {
        return &fonts[fnt_idx].font;
    }
    return NULL;
}

void draw_sprite(Sprite_t* spr, Vector2 pos, bool flip_x)
{
    Rectangle rec = {
        spr->origin.x + spr->frame_size.x * spr->current_frame,
        spr->origin.y,
        spr->frame_size.x * (flip_x ? -1:1),
        spr->frame_size.y
    };
    DrawTextureRec(*spr->texture, rec, pos, WHITE);
}
