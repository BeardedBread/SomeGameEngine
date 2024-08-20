#ifndef __ASSETS_H
#define __ASSETS_H
#include "sc/map/sc_map.h"
#include "EC.h"
#include "raylib.h"
#include "rres.h"

#define N_ASSETS_TYPE 6
typedef enum AssetType
{
    AST_TEXTURE = 0,
    AST_SPRITE,
    AST_SOUND,
    AST_FONT,
    AST_LEVELPACK,
    AST_EMITTER_CONF,
}AssetType_t;

typedef enum PartEmitterType
{
    EMITTER_UNKNOWN = 0,
    EMITTER_BURST,
    EMITTER_STREAM,
} PartEmitterType_t;

typedef struct EmitterConfig
{
    float launch_range[2];
    float speed_range[2];
    float angle_range[2];
    float rotation_range[2];
    float particle_lifetime[2];
    float initial_spawn_delay;
    PartEmitterType_t type;
    bool one_shot;
}EmitterConfig_t;

typedef struct Assets
{
    struct sc_map_s64 m_textures;
    struct sc_map_s64 m_sounds;
    struct sc_map_s64 m_fonts;
    struct sc_map_s64 m_sprites;
    struct sc_map_s64 m_levelpacks;
    struct sc_map_s64 m_emitter_confs;
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
    uint16_t n_chests;
    LevelTileInfo_t* tiles;
}LevelMap_t;

typedef struct LevelPack
{
   uint32_t n_levels;
   LevelMap_t* levels;
}LevelPack_t;

// Credits to bedroomcoders.co.uk for this
typedef struct Sprite {
    Texture2D* texture;
    Vector2 frame_size;
    Vector2 origin; // TL of the frame
    Vector2 anchor; // Where transformation anchors on
    uint8_t frame_per_row;
    int frame_count;
    int speed;
    char* name;
} Sprite_t;

typedef struct RresFileInfo
{
    rresCentralDir dir;
    const char* fname;
}RresFileInfo_t;

void init_assets(Assets_t* assets);
void free_all_assets(Assets_t* assets);
void term_assets(Assets_t* assets);

Texture2D* add_texture(Assets_t* assets, const char* name, const char* path);
Texture2D* add_texture_from_img(Assets_t* assets, const char* name, Image img);
Sound* add_sound(Assets_t * assets, const char* name, const char* path);
Font* add_font(Assets_t* assets, const char* name, const char* path);
LevelPack_t* add_level_pack(Assets_t* assets, const char* name, const char* path);
LevelPack_t* uncompress_level_pack(Assets_t* assets, const char* name, const char* path);

// Rres version
Texture2D* add_texture_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file);
LevelPack_t* add_level_pack_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file);
Sound* add_sound_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file);

Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture);
EmitterConfig_t* add_emitter_conf(Assets_t* assets, const char* name);

Texture2D* get_texture(Assets_t* assets, const char* name);
Sprite_t* get_sprite(Assets_t* assets, const char* name);
EmitterConfig_t* get_emitter_conf(Assets_t* assets, const char* name);
Sound* get_sound(Assets_t* assets, const char* name);
Font* get_font(Assets_t* assets, const char* name);
LevelPack_t* get_level_pack(Assets_t* assets, const char* name);

void draw_sprite(Sprite_t* spr, int frame_num, Vector2 pos, float rotation, bool flip_x);
void draw_sprite_pro(Sprite_t* spr, int frame_num, Vector2 pos, float rotation, uint8_t flip, Vector2 scale, Color colour);
Vector2 get_anchor_offset(Vector2 bbox, AnchorPoint_t anchor, bool flip_x);
Vector2 shift_bbox(Vector2 bbox, Vector2 new_bbox, AnchorPoint_t anchor);

typedef struct SFX
{
    Sound* snd;
    uint8_t plays;
    uint8_t cooldown;
} SFX_t;

#endif // __ASSETS_H
