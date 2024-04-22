#include "assets.h"
#include "assert.h"
#include "engine_conf.h"

#define RRES_RAYLIB_IMPLEMENTATION
#include "rres.h"

#include "zstd.h"
#include <stdio.h>

uint8_t n_loaded[N_ASSETS_TYPE] = {0};

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
typedef struct EmitterConfData
{
    EmitterConfig_t conf;
    char name[MAX_NAME_LEN];
}EmitterConfData_t;

static TextureData_t textures[MAX_TEXTURES];
static FontData_t fonts[MAX_FONTS];
static SoundData_t sfx[MAX_SOUNDS];
static SpriteData_t sprites[MAX_SPRITES];
static LevelPackData_t levelpacks[MAX_LEVEL_PACK];
static EmitterConfData_t emitter_confs[MAX_EMITTER_CONF];

#define DECOMPRESSOR_INBUF_LEN 4096
#define DECOMPRESSOR_OUTBUF_LEN 4096
static struct ZstdDecompressor
{
    ZSTD_DCtx* ctx;
    uint8_t in_buffer[DECOMPRESSOR_INBUF_LEN];
    uint8_t out_buffer[DECOMPRESSOR_OUTBUF_LEN];
}level_decompressor;

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
    uint8_t tex_idx = n_loaded[AST_TEXTURE];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTexture(path);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    n_loaded[AST_TEXTURE]++;
    return &textures[tex_idx].texture;
}

Texture2D* add_texture_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file)
{
    uint8_t tex_idx = n_loaded[AST_TEXTURE];
    assert(tex_idx < MAX_TEXTURES);

    int res_id = rresGetResourceId(rres_file->dir, filename);
    rresResourceChunk chunk = rresLoadResourceChunk(rres_file->fname, res_id);

    Texture2D* out_tex = NULL;
    if (chunk.info.baseSize > 0)
    {
        uint32_t sz = chunk.info.baseSize - sizeof(int) - (chunk.data.propCount*sizeof(int));
        //Expect RAW type of png extension
        Image image = LoadImageFromMemory(GetFileExtension(filename), chunk.data.raw, sz);
        Texture2D tex = LoadTextureFromImage(image);
        UnloadImage(image); 
        
        textures[tex_idx].texture = tex;
        strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
        sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
        n_loaded[AST_TEXTURE]++;
        out_tex = &textures[tex_idx].texture;
    }
    rresUnloadResourceChunk(chunk);
    return out_tex;
}

Sound* add_sound_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file)
{
    uint8_t snd_idx = n_loaded[AST_SOUND];
    assert(snd_idx < MAX_SOUNDS);

    int res_id = rresGetResourceId(rres_file->dir, filename);
    rresResourceChunk chunk = rresLoadResourceChunk(rres_file->fname, res_id);

    Sound* out_snd = NULL;
    if (chunk.info.baseSize > 0)
    {
        uint32_t sz = chunk.info.baseSize - sizeof(int) - (chunk.data.propCount*sizeof(int));
        Wave wave = LoadWaveFromMemory(GetFileExtension(filename), chunk.data.raw, sz);
        Sound snd = LoadSoundFromWave(wave);
        UnloadWave(wave); 
        
        sfx[snd_idx].sound = snd;
        strncpy(sfx[snd_idx].name, name, MAX_NAME_LEN);
        sc_map_put_s64(&assets->m_sounds, sfx[snd_idx].name, snd_idx);
        n_loaded[AST_SOUND]++;
        out_snd = &sfx[snd_idx].sound;
    }
    rresUnloadResourceChunk(chunk);
    return out_snd;
}

Texture2D* add_texture_from_img(Assets_t* assets, const char* name, Image img)
{
    uint8_t tex_idx = n_loaded[AST_TEXTURE];
    assert(tex_idx < MAX_TEXTURES);
    Texture2D tex = LoadTextureFromImage(img);
    if (tex.width == 0 || tex.height == 0) return NULL;

    textures[tex_idx].texture = tex;
    strncpy(textures[tex_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_textures, textures[tex_idx].name, tex_idx);
    n_loaded[AST_TEXTURE]++;
    return &textures[tex_idx].texture;
}

Sprite_t* add_sprite(Assets_t* assets, const char* name, Texture2D* texture)
{
    uint8_t spr_idx = n_loaded[AST_SPRITE];
    assert(spr_idx < MAX_SPRITES);
    memset(sprites + spr_idx, 0, sizeof(SpriteData_t));
    sprites[spr_idx].sprite.texture = texture;
    strncpy(sprites[spr_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sprites, sprites[spr_idx].name, spr_idx);
    n_loaded[AST_SPRITE]++;
    return &sprites[spr_idx].sprite;
}

Sound* add_sound(Assets_t* assets, const char* name, const char* path)
{
    uint8_t snd_idx = n_loaded[AST_SOUND];
    assert(snd_idx < MAX_SOUNDS);
    sfx[snd_idx].sound = LoadSound(path);
    strncpy(sfx[snd_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_sounds, sfx[snd_idx].name, snd_idx);
    n_loaded[AST_SOUND]++;
    return &sfx[snd_idx].sound;
}

Font* add_font(Assets_t* assets, const char* name, const char* path)
{
    uint8_t fnt_idx = n_loaded[AST_FONT];
    assert(fnt_idx < MAX_FONTS);
    fonts[fnt_idx].font = LoadFont(path);
    strncpy(fonts[fnt_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_fonts, fonts[fnt_idx].name, fnt_idx);
    n_loaded[AST_FONT]++;
    return &fonts[fnt_idx].font;
}

EmitterConfig_t* add_emitter_conf(Assets_t* assets, const char* name)
{
    uint8_t emitter_idx = n_loaded[AST_EMITTER_CONF];
    assert(emitter_idx < MAX_EMITTER_CONF);
    memset(emitter_confs + emitter_idx, 0, sizeof(EmitterConfData_t));
    strncpy(emitter_confs[emitter_idx].name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_emitter_confs, emitter_confs[emitter_idx].name, emitter_idx);
    n_loaded[AST_EMITTER_CONF]++;
    return &emitter_confs[emitter_idx].conf;
}

LevelPack_t* add_level_pack(Assets_t* assets, const char* name, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    LevelPackData_t* pack_info = levelpacks + n_loaded[AST_LEVELPACK];
    fread(&pack_info->pack.n_levels, sizeof(uint32_t), 1, file);
    pack_info->pack.levels = calloc(pack_info->pack.n_levels, sizeof(LevelMap_t));

    for (uint8_t i = 0; i < pack_info->pack.n_levels; ++i)
    {
        fread(pack_info->pack.levels[i].level_name, sizeof(char), 32, file);
        fread(&pack_info->pack.levels[i].width, sizeof(uint16_t), 1, file);
        fread(&pack_info->pack.levels[i].height, sizeof(uint16_t), 1, file);
        uint32_t n_tiles = pack_info->pack.levels[i].width * pack_info->pack.levels[i].height;

        pack_info->pack.levels[i].tiles = calloc(n_tiles, sizeof(LevelTileInfo_t));
        fread(pack_info->pack.levels[i].tiles, 4, n_tiles, file);
    }
    
    fclose(file);
    uint8_t pack_idx = n_loaded[AST_LEVELPACK];
    strncpy(pack_info->name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_levelpacks, levelpacks[pack_idx].name, pack_idx);
    n_loaded[AST_LEVELPACK]++;

    return &levelpacks[pack_idx].pack;
}


static LevelPack_t* add_level_pack_zst(Assets_t* assets, const char* name, const uint8_t* zst_buffer, uint32_t len)
{
    LevelPackData_t* pack_info = levelpacks + n_loaded[AST_LEVELPACK];
    size_t read = 0;

    ZSTD_inBuffer input = { level_decompressor.in_buffer, read, 0 };
    ZSTD_outBuffer output = { level_decompressor.out_buffer, 4, 0 };

    ZSTD_DCtx_reset(level_decompressor.ctx, ZSTD_reset_session_only);
    do
    {
        if (input.pos == input.size)
        {
            puts("Read more");

            read = (len < DECOMPRESSOR_INBUF_LEN) ? len : DECOMPRESSOR_INBUF_LEN;
            memcpy(level_decompressor.in_buffer, zst_buffer, read);
            zst_buffer += read;
            len -= read;

            if (read == 0) break;

            input.size = read;
            input.pos = 0;
        }
        size_t const ret = ZSTD_decompressStream(level_decompressor.ctx, &output , &input);
        if (ZSTD_isError(ret))
        {
            printf("Decompression Error: %s\n", ZSTD_getErrorName(ret));
            break;
        }
    }
    while (output.pos == 0);

    if (output.pos == 0)
    {
        perror("Could not read number of levels");
        return NULL;
    }

    // Read number of levels and alloc the memory for the levels
    uint32_t n_levels = 0;
    uint8_t lvls = 0;
    bool err = false;
    memcpy(&n_levels, level_decompressor.out_buffer, 4);
    pack_info->pack.levels = calloc(n_levels, sizeof(LevelMap_t));

    for (lvls = 0; lvls < n_levels; ++lvls)
    {
        printf("Parsing level %u\n", lvls);
        output.size = 36;
        output.pos = 0;

        do
        {
            if (input.pos == input.size)
            {
                read = (len < DECOMPRESSOR_INBUF_LEN) ? len : DECOMPRESSOR_INBUF_LEN;
                memcpy(level_decompressor.in_buffer, zst_buffer, read);
                zst_buffer += read;
                len -= read;

                if (read == 0) break;
                input.size = read;
                input.pos = 0;
            }
            size_t const ret = ZSTD_decompressStream(level_decompressor.ctx, &output , &input);
            if (ZSTD_isError(ret))
            {
                printf("Decompression Error: %s\n", ZSTD_getErrorName(ret));
                break;
            }
            printf("Compare %lu to %lu\n", output.pos, output.size);
        }
        while (output.pos != output.size);

        if (output.pos != output.size)
        {
            perror("Could not read level");
            err = true;
            goto load_end;
        }
        memcpy(pack_info->pack.levels[lvls].level_name, level_decompressor.out_buffer, 32);
        memcpy(&pack_info->pack.levels[lvls].width, level_decompressor.out_buffer + 32, 2);
        memcpy(&pack_info->pack.levels[lvls].height, level_decompressor.out_buffer + 34, 2);
        pack_info->pack.levels[lvls].level_name[31] = '\0';
        printf("Level name: %s\n", pack_info->pack.levels[lvls].level_name);
        printf("WxH: %u %u\n", pack_info->pack.levels[lvls].width, pack_info->pack.levels[lvls].height);

        uint32_t n_tiles = pack_info->pack.levels[lvls].width * pack_info->pack.levels[lvls].height;

        uint32_t remaining_len = n_tiles * 4;
        pack_info->pack.levels[lvls].tiles = calloc(n_tiles, sizeof(LevelTileInfo_t));
        output.size = DECOMPRESSOR_OUTBUF_LEN;
        output.pos = 0;
        uint8_t* data_ptr = (uint8_t*)pack_info->pack.levels[lvls].tiles;
        do
        {
            if (input.pos == input.size)
            {
                read = (len < DECOMPRESSOR_INBUF_LEN) ? len : DECOMPRESSOR_INBUF_LEN;
                memcpy(level_decompressor.in_buffer, zst_buffer, read);
                zst_buffer += read;
                len -= read;

                if (read == 0) break;
                input.size = read;
                input.pos = 0;
            }
            size_t to_read = (remaining_len > DECOMPRESSOR_OUTBUF_LEN) ? DECOMPRESSOR_OUTBUF_LEN : remaining_len;
            output.size = to_read;
            output.pos = 0;
            size_t const ret = ZSTD_decompressStream(level_decompressor.ctx, &output , &input);
            if (ZSTD_isError(ret))
            {
                printf("Decompression Error: %s\n", ZSTD_getErrorName(ret));
                break;
            }
            memcpy(data_ptr, level_decompressor.out_buffer, output.pos);
            data_ptr += output.pos;
            remaining_len -= output.pos;
        }
        while (remaining_len > 0);

        if (remaining_len > 0)
        {
            free(pack_info->pack.levels[lvls].tiles);
            perror("Could not read level tiles");
            err = true;
            goto load_end;
        }
    }
load_end:
    if (err)
    {
        unload_level_pack(pack_info->pack);
        return NULL;
    }

    pack_info->pack.n_levels = lvls;
    uint8_t pack_idx = n_loaded[AST_LEVELPACK];
    strncpy(pack_info->name, name, MAX_NAME_LEN);
    sc_map_put_s64(&assets->m_levelpacks, levelpacks[pack_idx].name, pack_idx);
    n_loaded[AST_LEVELPACK]++;

    return &levelpacks[pack_idx].pack;
}

LevelPack_t* uncompress_level_pack(Assets_t* assets, const char* name, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0L, SEEK_END);
    uint32_t sz = ftell(file);

    rewind(file);
    uint8_t* zst_buffer = malloc(sz);
    if (zst_buffer == NULL)
    {
        fclose(file);
        return NULL;
    }
    fread(zst_buffer, 1, sz, file);
    fclose(file);

    LevelPack_t* pack = add_level_pack_zst(assets, name, zst_buffer, sz);
    free(zst_buffer);

    return pack;

}

LevelPack_t* add_level_pack_rres(Assets_t* assets, const char* name, const char* filename, const RresFileInfo_t* rres_file)
{
    int res_id = rresGetResourceId(rres_file->dir, filename);
    rresResourceChunk chunk = rresLoadResourceChunk(rres_file->fname, res_id);

    LevelPack_t* pack = NULL;
    if (chunk.info.baseSize > 0 && strcmp(".lpk", (const char*)(chunk.data.props + 1)) == 0)
    {
        uint32_t sz = chunk.info.baseSize - sizeof(int) - (chunk.data.propCount*sizeof(int));
        pack = add_level_pack_zst(assets, name, chunk.data.raw, sz);
    }
    else
    {
        printf("Cannot load level pack for %s\n", name);
    }
    rresUnloadResourceChunk(chunk);
    return pack;
}

void init_assets(Assets_t* assets)
{
    sc_map_init_s64(&assets->m_fonts, MAX_FONTS, 0);
    sc_map_init_s64(&assets->m_sprites, MAX_SPRITES, 0);
    sc_map_init_s64(&assets->m_textures, MAX_TEXTURES, 0);
    sc_map_init_s64(&assets->m_sounds, MAX_SOUNDS, 0);
    sc_map_init_s64(&assets->m_levelpacks, MAX_LEVEL_PACK, 0);
    sc_map_init_s64(&assets->m_emitter_confs, MAX_EMITTER_CONF, 0);
    level_decompressor.ctx = ZSTD_createDCtx();
}

void free_all_assets(Assets_t* assets)
{
    for (uint8_t i = 0; i < n_loaded[AST_TEXTURE]; ++i)
    {
        UnloadTexture(textures[i].texture);
    }
    for (uint8_t i = 0; i < n_loaded[AST_SOUND]; ++i)
    {
        UnloadSound(sfx[i].sound);
    }
    for (uint8_t i = 0; i < n_loaded[AST_FONT]; ++i)
    {
        UnloadFont(fonts[i].font);
    }
    for (uint8_t i = 0; i < n_loaded[AST_LEVELPACK]; ++i)
    {
        unload_level_pack(levelpacks[i].pack);
    }

    sc_map_clear_s64(&assets->m_textures);
    sc_map_clear_s64(&assets->m_fonts);
    sc_map_clear_s64(&assets->m_sounds);
    sc_map_clear_s64(&assets->m_sprites);
    sc_map_clear_s64(&assets->m_levelpacks);
    sc_map_clear_s64(&assets->m_emitter_confs);
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
    sc_map_term_s64(&assets->m_emitter_confs);
    ZSTD_freeDCtx(level_decompressor.ctx);
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

EmitterConfig_t* get_emitter_conf(Assets_t* assets, const char* name)
{
    uint8_t emitter_idx = sc_map_get_s64(&assets->m_emitter_confs, name);
    if (sc_map_found(&assets->m_emitter_confs))
    {
        return &emitter_confs[emitter_idx].conf;
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

void draw_sprite(Sprite_t* spr, int frame_num, Vector2 pos, float rotation, bool flip_x)
{
    Rectangle rec = {
        spr->origin.x + spr->frame_size.x * frame_num,
        spr->origin.y,
        spr->frame_size.x * (flip_x ? -1:1),
        spr->frame_size.y
    };
    //DrawTextureRec(*spr->texture, rec, pos, WHITE);
    Rectangle dest = {
        .x = pos.x - spr->anchor.x,
        .y = pos.y - spr->anchor.y,
        .width = spr->frame_size.x,
        .height = spr->frame_size.y
    };
    DrawTexturePro(
        *spr->texture,
        rec,
        dest,
        spr->anchor,
        rotation, WHITE
    );
}
