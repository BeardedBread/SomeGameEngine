#include "assets_loader.h"
#include <stdio.h>
#include <string.h>

typedef enum AssetInfoType
{
    TEXTURE_INFO,
    SPRITE_INFO,
    LEVELPACK_INFO,
    INVALID_INFO
}AssetInfoType_t;

typedef struct SpriteInfo
{
    char* tex;
    Vector2 origin;
    Vector2 frame_size;
    int speed;
    int frame_count;
}SpriteInfo_t;

static bool parse_sprite_info(char* sprite_info_str, SpriteInfo_t* spr_info)
{
    char* tex_name = strtok(sprite_info_str, ",");
    if (tex_name == NULL) return false;
    spr_info->tex = tex_name;
    char* spr_data = tex_name + strlen(tex_name) + 1;
    int data_count = sscanf(
        spr_data, "%f,%f,%f,%f,%d,%d",
        &spr_info->origin.x, &spr_info->origin.y,
        &spr_info->frame_size.x, &spr_info->frame_size.y,
        &spr_info->frame_count, &spr_info->speed
    );
    return data_count == 6;
}

bool load_from_rres(const char* file, Assets_t* assets)
{
    RresFileInfo_t rres_file;
    rres_file.dir = rresLoadCentralDirectory(file);
    rres_file.fname = file;

    if (rres_file.dir.count == 0)
    {
        puts("Empty central directory");
        return false;
    }

    int res_id = rresGetResourceId(rres_file.dir, "assets.info");
    rresResourceChunk chunk = rresLoadResourceChunk(file, res_id); // Hardcoded
    bool okay = false;

    if (chunk.info.id == res_id)
    {
        FILE* in_file = fmemopen(chunk.data.raw, chunk.info.baseSize, "rb");

        char buffer[256];
        char* tmp;
        size_t line_num = 0;
        AssetInfoType_t info_type = INVALID_INFO;

        while (true)
        {
            tmp = fgets(buffer, 256, in_file);
            if (tmp == NULL) break;
            tmp[strcspn(tmp, "\r\n")] = '\0';

            if (tmp[0] == '-')
            {
                tmp++;
                if (strcmp(tmp, "Texture") == 0)
                {
                    info_type = TEXTURE_INFO;
                }
                else if (strcmp(tmp, "Sprite") == 0)
                {
                    info_type = SPRITE_INFO;
                }
                else if (strcmp(tmp, "LevelPack") == 0)
                {
                    info_type = LEVELPACK_INFO;
                }
                else
                {
                    info_type = INVALID_INFO;
                }
            }
            else
            {
                char* name = strtok(buffer, ":");
                char* info_str = strtok(NULL, ":");
                if (name == NULL || info_str == NULL) continue;

                while(*name == ' ' || *name == '\t') name++;
                while(*info_str == ' ' || *info_str == '\t') info_str++;
                switch(info_type)
                {
                    case TEXTURE_INFO:
                    {
                        //if (add_texture(assets, name, info_str) == NULL)
                        if (add_texture_rres(assets, name, info_str, &rres_file) == NULL)
                        {
                            printf("Unable to add texture at line %lu\n", line_num);
                            break;
                        }
                        printf("Added texture %s as %s\n", info_str, name);
                        //strcpy(tmp2, name);
                    }
                    break;
                    case LEVELPACK_INFO:
                    {
                        //if (add_level_pack(assets, name, info_str) == NULL)
                        if (add_level_pack_rres(assets, name, info_str, &rres_file) == NULL)
                        {
                            printf("Unable to add level pack at line %lu\n", line_num);
                            break;
                        }
                        printf("Added level pack %s as %s\n", info_str, name);
                    }
                    break;
                    case SPRITE_INFO:
                    {
                        SpriteInfo_t spr_info = {0};
                        if (!parse_sprite_info(info_str, &spr_info))
                        {
                            printf("Unable to parse info for sprite at line %lu\n", line_num);
                            break;
                        }
                        //printf("Compare %s,%s = %d\n", tmp2, spr_info.tex, strcmp(tmp2, spr_info.tex));
                        Texture2D* tex = get_texture(assets, spr_info.tex);
                        if (tex == NULL)
                        {
                            printf("Unable to get texture info %s for sprite %s\n", spr_info.tex, name);
                            break;
                        }
                        printf("Added Sprite %s from texture %s\n", name, spr_info.tex);
                        Sprite_t* spr = add_sprite(assets, name, tex);
                        spr->origin = spr_info.origin;
                        spr->frame_size = spr_info.frame_size;
                        spr->frame_count = spr_info.frame_count;
                        spr->speed = spr_info.speed;
                    }
                    break;
                    default:
                    break;
                }
            }
            line_num++; 
        }
        fclose(in_file);
        okay = true;
    }
    rresUnloadResourceChunk(chunk);
    rresUnloadCentralDirectory(rres_file.dir);
    return okay;
}

bool load_from_infofile(const char* file, Assets_t* assets)
{
    FILE* in_file = fopen(file, "r");
    if (in_file == NULL)
    {
        printf("Unable to open file %s\n", file);
        return false;
    }
    
    char buffer[256];
    char* tmp;
    size_t line_num = 0;
    AssetInfoType_t info_type = INVALID_INFO;

    while (true)
    {
        tmp = fgets(buffer, 256, in_file);
        if (tmp == NULL) break;
        tmp[strcspn(tmp, "\r\n")] = '\0';

        if (tmp[0] == '-')
        {
            tmp++;
            if (strcmp(tmp, "Texture") == 0)
            {
                info_type = TEXTURE_INFO;
            }
            else if (strcmp(tmp, "Sprite") == 0)
            {
                info_type = SPRITE_INFO;
            }
            else if (strcmp(tmp, "LevelPack") == 0)
            {
                info_type = LEVELPACK_INFO;
            }
            else
            {
                info_type = INVALID_INFO;
            }
        }
        else
        {
            char* name = strtok(buffer, ":");
            char* info_str = strtok(NULL, ":");
            if (name == NULL || info_str == NULL) continue;

            while(*name == ' ' || *name == '\t') name++;
            while(*info_str == ' ' || *info_str == '\t') info_str++;
            switch(info_type)
            {
                case TEXTURE_INFO:
                {
                    if (add_texture(assets, name, info_str) == NULL)
                    {
                        printf("Unable to add texture at line %lu\n", line_num);
                        break;
                    }
                    printf("Added texture %s as %s\n", info_str, name);
                    //strcpy(tmp2, name);
                }
                break;
                case LEVELPACK_INFO:
                {
                    //if (add_level_pack(assets, name, info_str) == NULL)
                    if (uncompress_level_pack(assets, name, info_str) == NULL)
                    {
                        printf("Unable to add level pack at line %lu\n", line_num);
                        break;
                    }
                    printf("Added level pack %s as %s\n", info_str, name);
                }
                break;
                case SPRITE_INFO:
                {
                    SpriteInfo_t spr_info = {0};
                    if (!parse_sprite_info(info_str, &spr_info))
                    {
                        printf("Unable to parse info for sprite at line %lu\n", line_num);
                        break;
                    }
                    //printf("Compare %s,%s = %d\n", tmp2, spr_info.tex, strcmp(tmp2, spr_info.tex));
                    Texture2D* tex = get_texture(assets, spr_info.tex);
                    if (tex == NULL)
                    {
                        printf("Unable to get texture info %s for sprite %s\n", spr_info.tex, name);
                        break;
                    }
                    printf("Added Sprite %s from texture %s\n", name, spr_info.tex);
                    Sprite_t* spr = add_sprite(assets, name, tex);
                    spr->origin = spr_info.origin;
                    spr->frame_size = spr_info.frame_size;
                    spr->frame_count = spr_info.frame_count;
                    spr->speed = spr_info.speed;
                }
                break;
                default:
                break;
            }
        }
        line_num++; 
    }
    fclose(in_file);
    return true;
}
