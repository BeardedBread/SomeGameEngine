#include "scene_impl.h"
#include "assets_tag.h"
#include "gui.h"
#include "raymath.h"
#include <stdio.h>

#define LEVEL_PREVIEW_SIZE 400
static void level_select_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    Sprite_t* level_board = get_sprite(&scene->engine->assets, "lvl_board");
    Sprite_t* level_select = get_sprite(&scene->engine->assets, "lvl_select");
    //Sprite_t* preview = get_sprite(&scene->engine->assets, "lvlprvw");
    Font* menu_font = get_font(&scene->engine->assets, "MenuFont");
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(BLANK);
        draw_sprite(level_select, 0, (Vector2){0,0},0, false);
        draw_sprite(level_board, 0, (Vector2){level_select->frame_size.x,0},0, false);

        Rectangle draw_rec = {0,0,LEVEL_PREVIEW_SIZE,LEVEL_PREVIEW_SIZE * -1};
        Vector2 draw_pos = {
            level_select->frame_size.x + (level_board->frame_size.x - LEVEL_PREVIEW_SIZE) / 2,
            (level_board->frame_size.y - LEVEL_PREVIEW_SIZE) / 2
        };
        DrawTextureRec(
            data->preview.texture,
            draw_rec,
            draw_pos,
            WHITE
        );

        DrawTextEx(*menu_font, "Level Select", (Vector2){60, 20}, 40, 4, BLACK);
        vert_scrollarea_render(&data->scroll_area);
    EndTextureMode();
}

static void level_preview_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);

    if (!data->update_preview) return;

    LevelMap_t level = data->level_pack->levels[data->scroll_area.curr_selection];

    const uint32_t n_tiles = level.width * level.height;
    uint16_t max_dim = (level.width > level.height ? level.width : level.height);
    max_dim = (max_dim == 0)? 1: max_dim;
    uint32_t tile_size = LEVEL_PREVIEW_SIZE / max_dim;
    uint32_t tile_halfsize = tile_size >> 1;
    tile_halfsize = (tile_halfsize == 0)? 1 : tile_halfsize;

    const uint32_t lvl_width = level.width*tile_size-1;
    const uint32_t lvl_height = level.height*tile_size-1;

    const uint32_t x_offset = (LEVEL_PREVIEW_SIZE - lvl_width) / 2;
    const uint32_t y_offset = (LEVEL_PREVIEW_SIZE - lvl_height) / 2;
    const Color danger_col = {239,79,81,255};
    BeginTextureMode(data->preview);
        ClearBackground((Color){255,255,255,0});
        DrawRectangle(
            x_offset, y_offset, lvl_width, lvl_height,
            (Color){64,64,64,255}
        );
        for (uint32_t i = 0; i < n_tiles; ++i)
        {
            uint32_t pos_x = tile_size * (i % level.width) + x_offset;
            uint32_t pos_y = tile_size * (i / level.width) + y_offset;
            if (level.tiles[i].tile_type >= 8 && level.tiles[i].tile_type < 20)
            {
                uint32_t tmp_idx = level.tiles[i].tile_type - 8;
                uint32_t item_type = tmp_idx % 6;
                Color col = (tmp_idx > 5)? (Color){110,110,110,255} : (Color){160,117,48,255};
                DrawRectangle(pos_x, pos_y, tile_size, tile_size, col);
                switch (item_type)
                {
                    case 1:
                        DrawLine(pos_x, pos_y + tile_halfsize, pos_x + tile_halfsize, pos_y + tile_halfsize, danger_col);
                    break;
                    case 2:
                        DrawLine(pos_x + tile_halfsize, pos_y + tile_halfsize, pos_x + tile_size, pos_y + tile_halfsize, danger_col);
                    break;
                    case 3:
                        DrawLine(pos_x + tile_halfsize, pos_y, pos_x + tile_halfsize, pos_y + tile_halfsize, danger_col);
                    break;
                    case 4:
                        DrawLine(pos_x + tile_halfsize, pos_y + tile_halfsize, pos_x + tile_halfsize, pos_y + tile_size, danger_col);
                    break;
                    case 5:
                        DrawLine(pos_x, pos_y, pos_x + tile_size, pos_y + tile_size, danger_col);
                        DrawLine(pos_x, pos_y + tile_size, pos_x + tile_size, pos_y, danger_col);
                    break;
                }
                DrawRectangleLines(pos_x, pos_y, tile_size, tile_size, (Color){0,0,0,64});
            }
            else
            {

                switch(level.tiles[i].tile_type) {
                    case SOLID_TILE:
                        DrawRectangle(pos_x, pos_y, tile_size, tile_size, BLACK);
                    break;
                    case ONEWAY_TILE:
                        DrawRectangle(pos_x, pos_y, tile_size, tile_halfsize, (Color){128,64,0,255});
                    break;
                    case LADDER:
                        DrawRectangleLines(pos_x, pos_y, tile_size, tile_size, (Color){214,141,64,255});
                    break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        // Copied from level generation
                        // Priority: Down, Up, Left, Right
                        if (i + level.width < n_tiles && level.tiles[i + level.width].tile_type == SOLID_TILE)
                        {
                            DrawRectangle(pos_x, pos_y + tile_halfsize , tile_size, tile_halfsize, danger_col);
                        }
                        else if (i >= level.width && level.tiles[i - level.width].tile_type == SOLID_TILE)
                        {
                            DrawRectangle(pos_x, pos_y, tile_size, tile_halfsize, danger_col);
                        }
                        else if (i % level.width != 0 && level.tiles[i - 1].tile_type == SOLID_TILE)
                        {
                            DrawRectangle(pos_x, pos_y, tile_halfsize, tile_size, danger_col);
                        }
                        else if ((i + 1) % level.width != 0 && level.tiles[i + 1].tile_type == SOLID_TILE)
                        {
                            DrawRectangle(pos_x + tile_halfsize, pos_y, tile_halfsize, tile_size, danger_col);
                        }
                        else
                        {
                            DrawRectangle(pos_x, pos_y + tile_halfsize , tile_size, tile_halfsize, danger_col);
                        }
                    break;
                    case 20:
                        DrawCircle(pos_x + tile_halfsize, pos_y + tile_halfsize, tile_halfsize, (Color){12,12,12,255});
                    break;
                    case 22:
                        DrawRectangle(pos_x, pos_y, tile_size, tile_size, (Color){255,0,255,255});
                    break;
                    case 23:
                        DrawRectangle(pos_x, pos_y, tile_size, tile_size, (Color){255,255,0,255});
                    break;
                    case 24:
                        DrawRectangle(pos_x, pos_y, tile_size, tile_size, (Color){0,255,0,255});
                    break;
                    case 25:
                        DrawCircle(pos_x + tile_halfsize, pos_y + tile_halfsize, tile_halfsize-1, danger_col);
                    break;
                }
            }
        }
    EndTextureMode();
    data->update_preview = false;
}

static void level_select_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    switch(action)
    {
        case ACTION_UP:
            if (!pressed)
            {
                if (data->scroll_area.curr_selection > 0)
                {
                    data->scroll_area.curr_selection--;
                    vert_scrollarea_refocus(&data->scroll_area);
                    data->update_preview = true;
                }
            }
        break;
        case ACTION_DOWN:
            if (!pressed)
            {
                if (data->scroll_area.curr_selection < data->level_pack->n_levels - 1)
                {
                    data->scroll_area.curr_selection++;
                    vert_scrollarea_refocus(&data->scroll_area);
                    data->update_preview = true;
                }
            }
        break;
        case ACTION_EXIT:
            if (!pressed)
            {
                if(scene->engine != NULL)
                {
                    change_scene(scene->engine, MAIN_MENU_SCENE);
                }
            }
        break;
        case ACTION_NEXT_SPAWN:
            if (!pressed)
            {
                unsigned int prev_sel = data->scroll_area.curr_selection;
                // TODO: Add scene offset to scroll area calculation
                if (vert_scrollarea_set_pos(&data->scroll_area, scene->mouse_pos) != data->scroll_area.max_items)
                {
                    vert_scrollarea_refocus(&data->scroll_area);
                
                    if (prev_sel == data->scroll_area.curr_selection)
                    {
                        if (data->level_pack != NULL && data->scroll_area.curr_selection < data->level_pack->n_levels)
                        {
                            // TODO: Need to load the current level
                            LevelScene_t* level_scene = (LevelScene_t*)change_scene(scene->engine, GAME_SCENE);
                            level_scene->data.level_pack = data->level_pack;
                            level_scene->data.current_level = data->scroll_area.curr_selection;
                            reload_level_tilemap(level_scene);

                        }
                    }
                }
            }
        break;
        case ACTION_CONFIRM:
            if (!pressed)
            {
                if (data->level_pack != NULL && data->scroll_area.curr_selection < data->level_pack->n_levels)
                {
                    // TODO: Need to load the current level
                    LevelScene_t* level_scene = (LevelScene_t*)change_scene(scene->engine, GAME_SCENE);
                    level_scene->data.level_pack = data->level_pack;
                    level_scene->data.current_level = data->scroll_area.curr_selection;
                    reload_level_tilemap(level_scene);

                }
            }
        break;
        default:
        break;
    }
}

#define FONT_SIZE 22
#define TEXT_PADDING 3
#define SCROLL_TOTAL_HEIGHT 800
void init_level_select_scene(LevelSelectScene_t* scene)
{
    init_scene(&scene->scene, &level_select_do_action, 0);
    scene->data.preview = LoadRenderTexture(LEVEL_PREVIEW_SIZE, LEVEL_PREVIEW_SIZE);
    scene->data.update_preview = true;
    add_scene_layer(
        &scene->scene, scene->scene.engine->intended_window_size.x,
        scene->scene.engine->intended_window_size.y,
        (Rectangle){
            0, 0,
            scene->scene.engine->intended_window_size.x,
            scene->scene.engine->intended_window_size.y
        }
    );
    scene->scene.bg_colour = BLACK;
    Sprite_t* level_select = get_sprite(&scene->scene.engine->assets, "lvl_select");
    vert_scrollarea_init(&scene->data.scroll_area, (Rectangle){50, 75, level_select->frame_size.x * 0.6, level_select->frame_size.y * 3 / 4}, (Vector2){level_select->frame_size.x * 0.6, SCROLL_TOTAL_HEIGHT});
    vert_scrollarea_set_item_dims(&scene->data.scroll_area, FONT_SIZE, TEXT_PADDING);
    Font* menu_font = get_font(&scene->scene.engine->assets, "MenuFont");
    if (menu_font != NULL) {
        scene->data.scroll_area.comp.font = *menu_font; 
    }
    char buf[32];
    ScrollAreaRenderBegin(&scene->data.scroll_area);
        ClearBackground(BLANK);
        if (scene->data.level_pack != NULL)
        {
            vert_scrollarea_n_items(&scene->data.scroll_area, scene->data.level_pack->n_levels);
            for (unsigned int i = 0; i < scene->data.level_pack->n_levels; ++i)
            {
                vert_scrollarea_insert_item(&scene->data.scroll_area, scene->data.level_pack->levels[i].level_name, i);
            }
            for (unsigned int i = scene->data.level_pack->n_levels; i < scene->data.scroll_area.max_items; ++i)
            {
                vert_scrollarea_insert_item(&scene->data.scroll_area, "---", i);
            }
        }
        else
        {
            for (unsigned int i = 0; i < scene->data.scroll_area.max_items; ++i)
            {
                sprintf(buf, "Level %u", i); 
                vert_scrollarea_insert_item(&scene->data.scroll_area, buf, i);
            }
        }
    ScrollAreaRenderEnd();

    sc_array_add(&scene->scene.systems, &level_preview_render_func);
    sc_array_add(&scene->scene.systems, &level_select_render_func);
    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);
    sc_map_put_64(&scene->scene.action_map, KEY_ENTER, ACTION_CONFIRM);
    sc_map_put_64(&scene->scene.action_map, MOUSE_LEFT_BUTTON, ACTION_NEXT_SPAWN); // Abuse an unused action
}
void free_level_select_scene(LevelSelectScene_t* scene)
{
    UnloadRenderTexture(scene->data.preview);
    vert_scrollarea_free(&scene->data.scroll_area);
    free_scene(&scene->scene);
}
