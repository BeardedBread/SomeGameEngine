#include "scene_impl.h"
#include "ent_impl.h"
#include "constants.h"

void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles, Rectangle view_zone)
//void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles)
{
    //data->game_viewport = LoadRenderTexture(VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE);
    //data->game_rec = (Rectangle){25, 25, VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE};
    data->game_viewport = LoadRenderTexture(view_zone.width, view_zone.height);
    data->game_rec = view_zone;
    data->cam = (Camera2D){0};
    data->cam.rotation = 0.0f;
    data->cam.zoom = 1.0f;

    data->tilemap.max_tiles = max_tiles;
    if (tiles != NULL)
    {
        data->tilemap.tiles = tiles;
    }
    else
    {
        data->tilemap.tiles = calloc(max_tiles, sizeof(Tile_t));
    }

    data->tilemap.width = DEFAULT_MAP_WIDTH;
    data->tilemap.height = DEFAULT_MAP_HEIGHT;
    data->tilemap.tile_size = TILE_SIZE;
    data->tilemap.n_tiles = data->tilemap.width * data->tilemap.height;
    memset(data->tile_sprites, 0, sizeof(data->tile_sprites));
    for (size_t i = 0; i < max_tiles;i++)
    {
        data->tilemap.tiles[i].solid = NOT_SOLID;
        data->tilemap.tiles[i].tile_type = EMPTY_TILE;
        data->tilemap.tiles[i].rotation = TILE_NOROTATE;
        data->tilemap.tiles[i].moveable = true;
        data->tilemap.tiles[i].max_water_level = 4;
        sc_map_init_64v(&data->tilemap.tiles[i].entities_set, 16, 0);
        data->tilemap.tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
    }
}

void term_level_scene_data(LevelSceneData_t* data)
{
    for (size_t i = 0; i < data->tilemap.max_tiles;i++)
    {
        sc_map_term_64v(&data->tilemap.tiles[i].entities_set);
    }
    UnloadRenderTexture(data->game_viewport); // Unload render texture
}

bool load_level_tilemap(LevelScene_t* scene, unsigned int level_num)
{
    if (level_num >= scene->data.level_pack->n_levels) return false;

    LevelMap_t lvl_map = scene->data.level_pack->levels[level_num];
    uint32_t n_tiles = lvl_map.width * lvl_map.height;
    if (n_tiles > scene->data.tilemap.max_tiles) return false;

    scene->data.current_level = level_num;
    scene->data.tilemap.width = lvl_map.width;
    scene->data.tilemap.height = lvl_map.height;
    scene->data.tilemap.n_tiles = n_tiles;

    clear_entity_manager(&scene->scene.ent_manager);
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        scene->data.tilemap.tiles[i].max_water_level = 4;
        scene->data.tilemap.tiles[i].solid = NOT_SOLID;
        scene->data.tilemap.tiles[i].tile_type = EMPTY_TILE;
        scene->data.tilemap.tiles[i].moveable = true;
        scene->data.tilemap.tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
        sc_map_clear_64v(&scene->data.tilemap.tiles[i].entities_set);

        switch (lvl_map.tiles[i].tile_type)
        {
            case 1:
                change_a_tile(&scene->data.tilemap, i, SOLID_TILE);
            break;
            default:
            break;
        }
        switch (lvl_map.tiles[i].entity_to_spawn)
        {
            case 1:
            {
                Entity_t* ent = create_player(&scene->scene.ent_manager, &scene->scene.engine->assets);
                CTransform_t* p_ct = get_component(ent, CTRANSFORM_COMP_T);
                p_ct->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                p_ct->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size;
            }
            break;
            default:
            break;
        }
        scene->data.tilemap.tiles[i].water_level = lvl_map.tiles[i].water;
    }
    return true;
}

void reload_level_tilemap(LevelScene_t* scene)
{
    load_level_tilemap(scene, scene->data.current_level);
}

void load_next_level_tilemap(LevelScene_t* scene)
{
    unsigned int lvl = scene->data.current_level;
    lvl++;
    if (lvl < scene->data.level_pack->n_levels)
    {
        load_level_tilemap(scene, lvl);
    }
}

void load_prev_level_tilemap(LevelScene_t* scene)
{
    unsigned int lvl = scene->data.current_level;
    lvl--;
    if (lvl >= 0)
    {
        load_level_tilemap(scene, lvl);
    }
}

void change_a_tile(TileGrid_t* tilemap, unsigned int tile_idx, TileType_t new_type)
{
    TileType_t last_type = tilemap->tiles[tile_idx].tile_type;
    tilemap->tiles[tile_idx].tile_type = new_type;

    switch (new_type)
    {
        case EMPTY_TILE:
            tilemap->tiles[tile_idx].solid = NOT_SOLID;
        break;
        case ONEWAY_TILE:
            tilemap->tiles[tile_idx].solid = ONE_WAY;
        break;
        case LADDER:
        {
            int up_tile = tile_idx - tilemap->width;
            if (up_tile > 0 && tilemap->tiles[up_tile].tile_type != LADDER)
            {
                tilemap->tiles[tile_idx].solid = ONE_WAY;
            }
            else
            {
                tilemap->tiles[tile_idx].solid = NOT_SOLID;
            }
            int down_tile = tile_idx + tilemap->width;
            if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
            {
                tilemap->tiles[down_tile].solid = (tilemap->tiles[tile_idx].tile_type != LADDER)? ONE_WAY : NOT_SOLID;
            }
        }
        break;
        case SPIKES:
            tilemap->tiles[tile_idx].solid = NOT_SOLID;
        break;
        case SOLID_TILE:
            tilemap->tiles[tile_idx].solid = SOLID;
        break;
    }

    if (last_type == LADDER && new_type != LADDER)
    {
        int down_tile = tile_idx + tilemap->width;
        if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
        {
            tilemap->tiles[down_tile].solid = ONE_WAY;
        }

    }

    tilemap->tiles[tile_idx].rotation = TILE_NOROTATE;
    if (new_type == SPIKES)
    {
        // Priority: Down, Up, Left, Right
        if (tile_idx + tilemap->width < tilemap->n_tiles && tilemap->tiles[tile_idx + tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,16};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
        }
        else if (tile_idx - tilemap->width >= 0 && tilemap->tiles[tile_idx - tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
            tilemap->tiles[tile_idx].rotation = TILE_180ROT;
        }
        else if (tile_idx % tilemap->width != 0 && tilemap->tiles[tile_idx - 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
            tilemap->tiles[tile_idx].rotation = TILE_90CWROT;
        }
        else if ((tile_idx + 1) % tilemap->width != 0 && tilemap->tiles[tile_idx + 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){16,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
            tilemap->tiles[tile_idx].rotation = TILE_90CCWROT;
        }
        else
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,16};
            tilemap->tiles[tile_idx].size = (Vector2){32,16};
        }
    }
    else if (new_type == ONEWAY_TILE)
    {
        tilemap->tiles[tile_idx].offset = (Vector2){0,0};
        tilemap->tiles[tile_idx].size = (Vector2){32,10};
    }
    else
    {
        tilemap->tiles[tile_idx].offset = (Vector2){0,0};
        tilemap->tiles[tile_idx].size = (Vector2){32,32};
    }

    tilemap->tiles[tile_idx].moveable = (
        tilemap->tiles[tile_idx].tile_type == EMPTY_TILE
        || tilemap->tiles[tile_idx].tile_type == SPIKES
    );
    tilemap->tiles[tile_idx].def = (tilemap->tiles[tile_idx].tile_type == SOLID_TILE) ? 5: 2;

}
