#include "scene_impl.h"
#include "constants.h"

void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles)
{
    data->game_viewport = LoadRenderTexture(VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE);
    data->game_rec = (Rectangle){25, 25, VIEWABLE_MAP_WIDTH*TILE_SIZE, VIEWABLE_MAP_HEIGHT*TILE_SIZE};
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

static void clear_level_tilemap(LevelSceneData_t* data)
{
}

bool load_level_tilemap(LevelScene_t* scene, unsigned int level_num)
{
    return true;
}

void reload_level_tilemap(LevelScene_t* scene)
{
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
        }
        else if (tile_idx % tilemap->width != 0 && tilemap->tiles[tile_idx - 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
        }
        else if ((tile_idx + 1) % tilemap->width != 0 && tilemap->tiles[tile_idx + 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){16,0};
            tilemap->tiles[tile_idx].size = (Vector2){16,32};
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
