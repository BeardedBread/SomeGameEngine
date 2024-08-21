#include "scene_impl.h"
#include "water_flow.h"
#include "ent_impl.h"
#include "constants.h"

void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles, Rectangle view_zone)
{
    data->game_rec = view_zone;
    memset(&data->camera, 0, sizeof(LevelCamera_t));
    data->camera.cam.rotation = 0.0f;
    data->camera.cam.zoom = 1.0f;
    data->camera.mass = 0.2f;
    data->camera.k = 6.2f;
    data->camera.c = 2.2f;
    data->camera.range_limit = 200.0f;

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
    memset(&data->coins, 0, sizeof(data->coins));
    data->show_grid = false;

    memset(&data->sm, 0, sizeof(data->sm));
}

void term_level_scene_data(LevelSceneData_t* data)
{
    for (size_t i = 0; i < data->tilemap.max_tiles;i++)
    {
        sc_map_term_64v(&data->tilemap.tiles[i].entities_set);
    }
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
    scene->data.coins.current = 0;
    scene->data.coins.total = lvl_map.n_chests;

    clear_entity_manager(&scene->scene.ent_manager);

    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {

        scene->data.tilemap.tiles[i].solid = NOT_SOLID;
        scene->data.tilemap.tiles[i].tile_type = EMPTY_TILE;
        scene->data.tilemap.tiles[i].rotation = TILE_NOROTATE;
        scene->data.tilemap.tiles[i].connectivity = 0;
        scene->data.tilemap.tiles[i].moveable = true;
        scene->data.tilemap.tiles[i].offset = (Vector2){0, 0};
        scene->data.tilemap.tiles[i].size = (Vector2){TILE_SIZE, TILE_SIZE};
        scene->data.tilemap.tiles[i].def = 0;

        sc_map_clear_64v(&scene->data.tilemap.tiles[i].entities_set);
        
        if (lvl_map.tiles[i].tile_type == 1)
        {
            change_a_tile(&scene->data.tilemap, i, SOLID_TILE);
        }

        scene->data.tilemap.tiles[i].max_water_level = 4;
        scene->data.tilemap.tiles[i].water_level = lvl_map.tiles[i].water;
        scene->data.tilemap.tiles[i].wet =  scene->data.tilemap.tiles[i].water_level > 0;
    }
    // Two pass
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        if (lvl_map.tiles[i].tile_type >= 8 && lvl_map.tiles[i].tile_type < 20)
        {
            uint32_t tmp_idx = lvl_map.tiles[i].tile_type - 8;
            uint32_t item_type = tmp_idx % 6;
            ContainerItem_t item = CONTAINER_EMPTY;
            switch (item_type)
            {
                case 1: item = CONTAINER_LEFT_ARROW;break;
                case 2: item = CONTAINER_RIGHT_ARROW;break;
                case 3: item = CONTAINER_UP_ARROW;break;
                case 4: item = CONTAINER_DOWN_ARROW;break;
                case 5: item = CONTAINER_BOMB;break;
                default: break;
            }

            Entity_t* ent = create_crate(&scene->scene.ent_manager, tmp_idx > 5, item);
            ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
            ent->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size;
        }
        else
        {
        
            switch (lvl_map.tiles[i].tile_type)
            {
                case 2:
                    change_a_tile(&scene->data.tilemap, i, ONEWAY_TILE);
                break;
                case 3:
                    change_a_tile(&scene->data.tilemap, i, LADDER);
                break;
                case 4:
                case 5:
                case 6:
                case 7:
                    change_a_tile(&scene->data.tilemap, i, SPIKES);
                break;
                case 20:
                {
                    Entity_t* ent = create_boulder(&scene->scene.ent_manager);
                    ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                    ent->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                }
                break;
                case 21:
                {
                    create_water_runner(&scene->scene.ent_manager, lvl_map.width, lvl_map.height, i);
                }
                break;
                case 22:
                {
                    Entity_t* ent = create_player(&scene->scene.ent_manager);
                    ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                    ent->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                }
                break;
                case 23:
                {
                    Entity_t* ent = create_chest(&scene->scene.ent_manager);
                    ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                    ent->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                    CTransform_t* p_ctransform = get_component(ent, CTRANSFORM_COMP_T);
                    p_ctransform->active = true;
                }
                break;
                case 24:
                {
                    Entity_t* ent = create_level_end(&scene->scene.ent_manager);
                    if (ent != NULL)
                    {
                        ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size;
                        ent->position.y = (i / scene->data.tilemap.width) * scene->data.tilemap.tile_size + (scene->data.tilemap.tile_size >> 1);
                    }
                }
                break;
                default:
                break;
            }
        }
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
            unsigned int down_tile = tile_idx + tilemap->width;
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
        unsigned int down_tile = tile_idx + tilemap->width;
        if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
        {
            tilemap->tiles[down_tile].solid = ONE_WAY;
        }

    }

    tilemap->tiles[tile_idx].rotation = TILE_NOROTATE;

    const int SPIKE_HITBOX_LONGSIDE = 30;
    const int SPIKE_HITBOX_SHORTSIDE = 12;
    if (new_type == SPIKES)
    {
        // Priority: Down, Up, Left, Right
        if (tile_idx + tilemap->width < tilemap->n_tiles && tilemap->tiles[tile_idx + tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,tilemap->tile_size - SPIKE_HITBOX_SHORTSIDE};
            tilemap->tiles[tile_idx].size = (Vector2){SPIKE_HITBOX_LONGSIDE, SPIKE_HITBOX_SHORTSIDE};
        }
        else if (tile_idx - tilemap->width >= 0 && tilemap->tiles[tile_idx - tilemap->width].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){SPIKE_HITBOX_LONGSIDE, SPIKE_HITBOX_SHORTSIDE};
            tilemap->tiles[tile_idx].rotation = TILE_180ROT;
        }
        else if (tile_idx % tilemap->width != 0 && tilemap->tiles[tile_idx - 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){0,0};
            tilemap->tiles[tile_idx].size = (Vector2){SPIKE_HITBOX_SHORTSIDE, SPIKE_HITBOX_LONGSIDE};
            tilemap->tiles[tile_idx].rotation = TILE_90CWROT;
        }
        else if ((tile_idx + 1) % tilemap->width != 0 && tilemap->tiles[tile_idx + 1].tile_type == SOLID_TILE)
        {
            tilemap->tiles[tile_idx].offset = (Vector2){SPIKE_HITBOX_SHORTSIDE >> 1,0};
            tilemap->tiles[tile_idx].size = (Vector2){SPIKE_HITBOX_SHORTSIDE, SPIKE_HITBOX_LONGSIDE};
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

    tilemap->tiles[tile_idx].connectivity = 0;

    const uint8_t LEFT_BIT = 0;
    const uint8_t UP_BIT = 1;
    const uint8_t RIGHT_BIT = 2;
    const uint8_t DOWN_BIT = 3;
    {
        // Priority: Left, Up, Right, Down
        if (tile_idx % tilemap->width != 0)
        {
            tilemap->tiles[tile_idx].connectivity |= (tilemap->tiles[tile_idx - 1].tile_type == SOLID_TILE) ? (1 << LEFT_BIT) : 0;
            if (new_type == SOLID_TILE)
            {
                tilemap->tiles[tile_idx - 1].connectivity |= (1 << RIGHT_BIT);
            }
            else
            {
                tilemap->tiles[tile_idx - 1].connectivity &= ~(1 << RIGHT_BIT);
            }
        }

        if (tile_idx >= tilemap->width)
        {
            tilemap->tiles[tile_idx].connectivity |= (tilemap->tiles[tile_idx - tilemap->width].tile_type == SOLID_TILE) ? (1 << UP_BIT) : 0;
            if (new_type == SOLID_TILE)
            {
                tilemap->tiles[tile_idx - tilemap->width].connectivity |= (1 << DOWN_BIT);
            }
            else
            {
                tilemap->tiles[tile_idx - tilemap->width].connectivity &= ~(1 << DOWN_BIT);
            }
        }

        if (tile_idx + tilemap->width < tilemap->n_tiles)
        {
            tilemap->tiles[tile_idx].connectivity |= (tilemap->tiles[tile_idx + tilemap->width].tile_type == SOLID_TILE) ? (1 << DOWN_BIT) : 0;
            if (new_type == SOLID_TILE)
            {
                tilemap->tiles[tile_idx + tilemap->width].connectivity |= (1 << UP_BIT);
            }
            else
            {
                tilemap->tiles[tile_idx + tilemap->width].connectivity &= ~(1 << UP_BIT);
            }
        }

        if ((tile_idx + 1) % tilemap->width != 0)
        {
            tilemap->tiles[tile_idx].connectivity |= (tilemap->tiles[tile_idx + 1].tile_type == SOLID_TILE) ? (1 << RIGHT_BIT) : 0;
            if (new_type == SOLID_TILE)
            {
                tilemap->tiles[tile_idx + 1].connectivity |= (1 << LEFT_BIT);
            }
            else
            {
                tilemap->tiles[tile_idx + 1].connectivity &= ~(1 << LEFT_BIT);
            }
        }
    }

}
