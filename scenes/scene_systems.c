#include "collisions.h"
#include "scene_impl.h"
#include "water_flow.h"
#include "ent_impl.h"
#include "constants.h"

#include "raymath.h"

void init_level_scene_data(LevelSceneData_t* data, uint32_t max_tiles, Tile_t* tiles, Rectangle view_zone)
{
    init_render_manager(&data->render_manager);

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

void clear_an_entity(Scene_t* scene, TileGrid_t* tilemap, Entity_t* p_ent)
{
    /* Use the helper function to remove any entity
     * This is because some components may have deinit steps
     * This function will also take care of the tilemap collision handling
     * */

    CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);
    if (p_emitter != NULL)
    {
        unload_emitter_handle(&scene->part_sys, p_emitter->handle);
    }
    if (get_component(p_ent, CWATERRUNNER_T)!= NULL)
    {
        free_water_runner(p_ent, &scene->ent_manager);
    }

    remove_entity_from_tilemap(&scene->ent_manager, tilemap, p_ent);
}

void clear_all_game_entities(LevelScene_t* scene)
{
    Entity_t* ent;
    sc_map_foreach_value(&scene->scene.ent_manager.entities, ent)
    {
        clear_an_entity(&scene->scene, &scene->data.tilemap, ent);
    }

    // This is unnecessary as the first pass should clear everything
    // For now, leave it in. This is not expected to call all the time
    // so not too bad
    clear_entity_manager(&scene->scene.ent_manager);
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        sc_map_clear_64v(&scene->data.tilemap.tiles[i].entities_set);
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

    #define N_SOLID_TILESETS 3
    static const char* SOLID_TILE_SELECTIONS[N_SOLID_TILESETS] = {
        "stile0", "stile1", "stile2"
    };
    scene->data.selected_solid_tilemap = lvl_map.flags;
    scene->data.solid_tile_sprites = get_sprite(&scene->scene.engine->assets, SOLID_TILE_SELECTIONS[lvl_map.flags]);

    clear_all_game_entities(scene);

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

        memset(scene->data.tilemap.render_nodes + i, 0, sizeof(RenderInfoNode));
        scene->data.tilemap.render_nodes[i].pos = (Vector2){
            (i % scene->data.tilemap.width) * TILE_SIZE,
            (i / scene->data.tilemap.width) * TILE_SIZE,
        };
        scene->data.tilemap.render_nodes[i].scale = (Vector2){1,1};
        scene->data.tilemap.render_nodes[i].colour = WHITE;

        if (lvl_map.tiles[i].tile_type == SOLID_TILE)
        {
            change_a_tile(&scene->data.tilemap, i, SOLID_TILE);
        }

        if (lvl_map.tiles[i].water > MAX_WATER_LEVEL) {
            scene->data.tilemap.tiles[i].max_water_level = 0;
        }
        else
        {
            scene->data.tilemap.tiles[i].max_water_level = 4;
            scene->data.tilemap.tiles[i].water_level = lvl_map.tiles[i].water;
            scene->data.tilemap.tiles[i].wet =  scene->data.tilemap.tiles[i].water_level > 0;
        }
    }

    // Two pass, because some tile depends on the solidity of the tiles
    for (size_t i = 0; i < scene->data.tilemap.n_tiles;i++)
    {
        if (lvl_map.tiles[i].tile_type == SOLID_TILE)
        {
            change_a_tile(&scene->data.tilemap, i, SOLID_TILE);
        }
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
                    scene->data.camera.target_pos.x = ent->position.x;
                    scene->data.camera.target_pos.y = ent->position.y;
                    scene->data.camera.cam.target.x = ent->position.x;
                    scene->data.camera.cam.target.y = ent->position.y;
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

            if (lvl_map.tiles[i].tile_type >= 25) 
            {
                Entity_t* ent = create_urchin(&scene->scene.ent_manager);
                if (ent != NULL)
                {
                    CBBox_t* p_bbox = get_component(ent, CBBOX_COMP_T);
                    ent->position.x = (i % scene->data.tilemap.width) * scene->data.tilemap.tile_size + (scene->data.tilemap.tile_size >> 1) - p_bbox->half_size.x;

                    ent->position.y = (int)(i / scene->data.tilemap.width) * scene->data.tilemap.tile_size + (scene->data.tilemap.tile_size >> 1) - p_bbox->half_size.y;

                    uint8_t spd_encoding = lvl_map.tiles[i].tile_type - 25;
                    float angle = 45.0f / 180.0f * PI * ((spd_encoding >> 2) & 7);
                    float mag = 75 * (spd_encoding & 3);

                    CTransform_t* p_ct = get_component(ent, CTRANSFORM_COMP_T);
                    p_ct->velocity = Vector2Scale(
                        (Vector2){cosf(angle), sinf(angle)}, mag
                    );
                }
            }
        }

        // This only works for static loading.
        // If a tilemap change change to some other arbitrary tile.
        // Then this should be done while changing a tile.
        if (lvl_map.tiles[i].tile_type == SOLID_TILE)
        {
            scene->data.tilemap.render_nodes[i].spr = scene->data.solid_tile_sprites;
            scene->data.tilemap.render_nodes[i].frame_num = CONNECTIVITY_TILE_MAPPING[
                scene->data.tilemap.tiles[i].connectivity
            ];
        }
        else
        {
            uint8_t tile_sprite_idx = scene->data.tilemap.tiles[i].tile_type + scene->data.tilemap.tiles[i].rotation;
            scene->data.tilemap.render_nodes[i].spr = scene->data.tile_sprites[
                tile_sprite_idx
            ];
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
            //int up_tile = tile_idx - tilemap->width;
            //if (up_tile > 0 && tilemap->tiles[up_tile].tile_type != LADDER)
            //{
            //    tilemap->tiles[tile_idx].solid = ONE_WAY;
            //}
            //else
            {
                tilemap->tiles[tile_idx].solid = NOT_SOLID;
            }
            //unsigned int down_tile = tile_idx + tilemap->width;
            //if (down_tile < tilemap->n_tiles && tilemap->tiles[down_tile].tile_type == LADDER)
            //{
            //    tilemap->tiles[down_tile].solid = (tilemap->tiles[tile_idx].tile_type != LADDER)? ONE_WAY : NOT_SOLID;
            //}
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
            tilemap->tiles[down_tile].solid = NOT_SOLID;
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
        else if (tile_idx >= tilemap->width && tilemap->tiles[tile_idx - tilemap->width].tile_type == SOLID_TILE)
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
            tilemap->tiles[tile_idx].offset = (Vector2){tilemap->tile_size - SPIKE_HITBOX_SHORTSIDE,0};
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
        || tilemap->tiles[tile_idx].tile_type == LADDER
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
