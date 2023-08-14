#include "assets.h"
#include "assert.h"

#include <stdio.h>

#define MAX_TEXTURES 16
#define MAX_SPRITES 16
#define MAX_SOUNDS 16
#define MAX_FONTS 4
#define MAX_N_TILES 4096
#define MAX_NAME_LEN 32
#define MAX_LEVEL_PACK 1
uint8_t n_loaded[5] = {0};

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
typedef struct LevelPackData
{
    LevelPack_t pack;
    char name[MAX_NAME_LEN];
}LevelPackData_t;

static TextureData_t textures[MAX_TEXTURES];
static FontData_t fonts[MAX_FONTS];
static SoundData_t sfx[MAX_SOUNDS];
static SpriteData_t sprites[MAX_SPRITES];
static LevelPackData_t levelpacks[MAX_LEVEL_PACK];

static void unload_level_pack(LevelPack_t pack)
{
    for (uint8_t i = 0; i < pack.n_levels; ++i)
    {
        free(pack.levels[i].tiles);
    }
    free(pack.levels);
}

// Maybe need a circular buffer??
Texture2D* add_texture(Assets_t* assets, const char* name, const char* path)
{
    uint8_t tex_idx = n_loaded[0];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTexture(path);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    n_loaded[0]++;
    return &textures[tex_idx].texture;
}

Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture)
{
    uint8_t spr_idx = n_loaded[1];
    assert(spr_idx < MAX_SPRITES);
    memset(sprites + spr_idx, 0, sizeof(Sprite_t));
    sprites[spr_idx].sprite.texture = texture;
    strncpy(sprites[spr_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sprites, sprites[spr_idx].name, spr_idx);
    n_loaded[1]++;
    return &sprites[spr_idx].sprite;
}

Sound* add_sound(Assets_t* assets, const char* name, const char* path)
{
    uint8_t snd_idx = n_loaded[2];
    assert(snd_idx < MAX_SOUNDS);
    sfx[snd_idx].sound = LoadSound(path);
    strncpy(sfx[snd_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sounds, sfx[snd_idx].name, snd_idx);
    n_loaded[2]++;
    return &sfx[snd_idx].sound;
}

Font* add_font(Assets_t* assets, const char* name, const char* path)
{
    uint8_t fnt_idx = n_loaded[3];
    assert(fnt_idx < MAX_FONTS);
    fonts[fnt_idx].font = LoadFont(path);
    strncpy(fonts[fnt_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_fonts, fonts[fnt_idx].name, fnt_idx);
    n_loaded[3]++;
    return &fonts[fnt_idx].font;
}

LevelPack_t* add_level_pack(Assets_t* assets, const char* name, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    LevelPackData_t* pack_info = levelpacks + n_loaded[4];
    fread(&pack_info->pack.n_levels, sizeof(uint32_t), 1, file);
    pack_info->pack.levels = calloc(pack_info->pack.n_levels, sizeof(LevelMap_t));

    for (uint8_t i = 0; i < pack_info->pack.n_levels; ++i)
    {
        fread(pack_info->pack.levels[i].level_name, sizeof(char), 32, file);
        fread(&pack_info->pack.levels[i].width, sizeof(uint16_t), 1, file);
        fread(&pack_info->pack.levels[i].height, sizeof(uint16_t), 1, file);
        uint32_t n_tiles = pack_info->pack.levels[i].width * pack_info->pack.levels[i].height;

        pack_info->pack.levels[i].tiles = calloc(n_tiles, sizeof(LevelTileInfo_t));
        uint16_t dummy;
        for (uint32_t j = 0; j < n_tiles; j++)
        {
            fread(&pack_info->pack.levels[i].tiles[j].tile_type, 1, 1, file);
            fread(&pack_info->pack.levels[i].tiles[j].entity_to_spawn, 1, 1, file);
            fread(&dummy, 2, 1, file);
        }
    }
    
    fclose(file);
    uint8_t pack_idx = n_loaded[4];
    strncpy(pack_info->name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_levelpacks, levelpacks[pack_idx].name, pack_idx);
    n_loaded[4]++;

    return &levelpacks[pack_idx].pack;
}

void init_assets(Assets_t* assets)
{
    sc_map_init_s64(&assets->m_fonts, MAX_FONTS, 0);
    sc_map_init_s64(&assets->m_sprites, MAX_SPRITES, 0);
    sc_map_init_s64(&assets->m_textures, MAX_TEXTURES, 0);
    sc_map_init_s64(&assets->m_sounds, MAX_SOUNDS, 0);
    sc_map_init_s64(&assets->m_levelpacks, MAX_LEVEL_PACK, 0);
}

void free_all_assets(Assets_t* assets)
{
    for (uint8_t i = 0; i < n_loaded[0]; ++i)
    {
        UnloadTexture(textures[i].texture);
    }
    for (uint8_t i = 0; i < n_loaded[2]; ++i)
    {
        UnloadSound(sfx[i].sound);
    }
    for (uint8_t i = 0; i < n_loaded[3]; ++i)
    {
        UnloadFont(fonts[i].font);
    }
    for (uint8_t i = 0; i < n_loaded[4]; ++i)
    {
        unload_level_pack(levelpacks[i].pack);
    }

    sc_map_clear_s64(&assets->m_textures);
    sc_map_clear_s64(&assets->m_fonts);
    sc_map_clear_s64(&assets->m_sounds);
    sc_map_clear_s64(&assets->m_sprites);
    sc_map_clear_s64(&assets->m_levelpacks);
    memset(n_loaded, 0, sizeof(n_loaded));
}

void term_assets(Assets_t* assets)
{
    free_all_assets(assets);
    sc_map_term_s64(&assets->m_textures);
    sc_map_term_s64(&assets->m_fonts);
    sc_map_term_s64(&assets->m_sounds);
    sc_map_term_s64(&assets->m_sprites);
    sc_map_term_s64(&assets->m_levelpacks);
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

LevelPack_t* get_level_pack(Assets_t* assets, const char* name)
{
    uint8_t pack_idx = sc_map_get_s64(&assets->m_levelpacks, name);
    if (sc_map_found(&assets->m_levelpacks))
    {
        return &levelpacks[pack_idx].pack;
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
