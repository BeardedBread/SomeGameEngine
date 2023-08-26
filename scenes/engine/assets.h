#ifndef __ASSETS_H
#define __ASSETS_H
#include "sc/map/sc_map.h"
#include "EC.h"
#include "raylib.h"

typedef struct Assets
{
    struct sc_map_s64 m_textures;
    struct sc_map_s64 m_sounds;
    struct sc_map_s64 m_fonts;
    struct sc_map_s64 m_sprites;
    struct sc_map_s64 m_levelpacks;
}Assets_t;

typedef struct LevelTileInfo
{
    uint8_t tile_type;
    uint8_t entity_to_spawn;
    uint8_t water;
    uint8_t dummy[1];
}LevelTileInfo_t;

typedef struct LevelMap
{
    char level_name[32];
    uint16_t width;
    uint16_t height;
    LevelTileInfo_t* tiles;
}LevelMap_t;

typedef struct LevelPack
{
   uint32_t n_levels;
   LevelMap_t* levels;
}LevelPack_t;

void init_assets(Assets_t* assets);
void free_all_assets(Assets_t* assets);
void term_assets(Assets_t* assets);

Texture2D* add_texture(Assets_t* assets, const char* name, const char* path);
Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture);
Sound* add_sound(Assets_t * assets, const char* name, const char* path);
Font* add_font(Assets_t* assets, const char* name, const char* path);
LevelPack_t* add_level_pack(Assets_t* assets, const char* name, const char* path);
LevelPack_t* uncompress_level_pack(Assets_t* assets, const char* name, const char* path);

Texture2D* get_texture(Assets_t* assets, const char* name);
Sprite_t* get_sprite(Assets_t* assets, const char* name);
Sound* get_sound(Assets_t* assets, const char* name);
Font* get_font(Assets_t* assets, const char* name);
LevelPack_t* get_level_pack(Assets_t* assets, const char* name);

void draw_sprite(Sprite_t* spr, Vector2 pos, bool flip_x);
#endif // __ASSETS_H
