#include "scene_impl.h"
#include "assets_tag.h"
#include "gui.h"
#include "raymath.h"
#include <stdio.h>

static void level_select_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(BLANK);
        DrawText("Level Select", 10, 10, 40, BLACK);
        vert_scrollarea_render(&data->scroll_area);
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

#define START_X 300
#define START_Y 100
#define FONT_SIZE 30
#define TEXT_PADDING 3
#define DISPLAY_AREA_HEIGHT 400
#define SCROLL_TOTAL_HEIGHT 800
void init_level_select_scene(LevelSelectScene_t* scene)
{
    init_scene(&scene->scene, &level_select_do_action);
    add_scene_layer(
        &scene->scene, 400, 800,
        (Rectangle){START_X, START_Y, 400, 800}
    );
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
