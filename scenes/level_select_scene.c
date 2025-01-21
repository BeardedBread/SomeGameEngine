#include "scene_impl.h"
#include "assets_tag.h"
#include "gui.h"
#include "raymath.h"
#include <stdio.h>

static void level_select_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    Sprite_t* spr = get_sprite(&scene->engine->assets, "bunny_spr1");
    Sprite_t* level_board = get_sprite(&scene->engine->assets, "lvl_board");
    Sprite_t* level_select = get_sprite(&scene->engine->assets, "lvl_select");
    Sprite_t* preview = get_sprite(&scene->engine->assets, "lvlprvw");
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(BLANK);
        draw_sprite(level_select, 0, (Vector2){0,0},0, false);
        draw_sprite(level_board, 0, (Vector2){level_select->frame_size.x,0},0, false);

        draw_sprite(preview, data->scroll_area.curr_selection, (Vector2){
            level_select->frame_size.x + (level_board->frame_size.x - preview->frame_size.x) / 2,
            (level_board->frame_size.y - preview->frame_size.y) / 2,
        },0, false);
        DrawText("Level Select", 10, 10, 40, BLACK);
        vert_scrollarea_render(&data->scroll_area);
        draw_sprite(
            spr, 0, (Vector2){
                scene->engine->intended_window_size.x / 2,
                scene->engine->intended_window_size.y / 2,
            }, 0, false);
    EndTextureMode();
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
                }
            }
        break;
        case ACTION_DOWN:
            if (!pressed)
            {
                if (data->scroll_area.curr_selection < data->scroll_area.n_items - 1)
                {
                    data->scroll_area.curr_selection++;
                    vert_scrollarea_refocus(&data->scroll_area);
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
                if (vert_scrollarea_set_pos(&data->scroll_area, scene->mouse_pos) != data->scroll_area.n_items)
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

#define FONT_SIZE 15
#define TEXT_PADDING 3
#define DISPLAY_AREA_HEIGHT 400
#define SCROLL_TOTAL_HEIGHT 400
void init_level_select_scene(LevelSelectScene_t* scene)
{
    init_scene(&scene->scene, &level_select_do_action, 0);
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
    vert_scrollarea_init(&scene->data.scroll_area, (Rectangle){50, 100, 150, DISPLAY_AREA_HEIGHT - 100}, (Vector2){150, SCROLL_TOTAL_HEIGHT});
    vert_scrollarea_set_item_dims(&scene->data.scroll_area, FONT_SIZE, TEXT_PADDING);
    char buf[32];
    ScrollAreaRenderBegin(&scene->data.scroll_area);
        ClearBackground(BLANK);
        if (scene->data.level_pack != NULL)
        {
            scene->data.scroll_area.n_items = scene->data.level_pack->n_levels;
            for (unsigned int i = 0; i < scene->data.level_pack->n_levels; ++i)
            {
                vert_scrollarea_insert_item(&scene->data.scroll_area, scene->data.level_pack->levels[i].level_name, i);
            }
            for (unsigned int i = scene->data.level_pack->n_levels; i < scene->data.scroll_area.n_items; ++i)
            {
                vert_scrollarea_insert_item(&scene->data.scroll_area, "---", i);
            }
        }
        else
        {
            for (unsigned int i = 0; i < scene->data.scroll_area.n_items; ++i)
            {
                sprintf(buf, "Level %u", i); 
                vert_scrollarea_insert_item(&scene->data.scroll_area, buf, i);
            }
        }
    ScrollAreaRenderEnd();

    sc_array_add(&scene->scene.systems, &level_select_render_func);
    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);
    sc_map_put_64(&scene->scene.action_map, KEY_ENTER, ACTION_CONFIRM);
    sc_map_put_64(&scene->scene.action_map, MOUSE_LEFT_BUTTON, ACTION_NEXT_SPAWN); // Abuse an unused action
}
void free_level_select_scene(LevelSelectScene_t* scene)
{

    vert_scrollarea_free(&scene->data.scroll_area);
    free_scene(&scene->scene);
}
