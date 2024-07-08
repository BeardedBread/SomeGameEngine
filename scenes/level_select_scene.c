#include "scene_impl.h"
#include "assets_tag.h"
#include "gui.h"
#include "raymath.h"
#include <stdio.h>

#define FONT_SIZE 30
#define TEXT_PADDING 3
#define TEXT_HEIGHT (FONT_SIZE+TEXT_PADDING)
#define DISPLAY_AREA_HEIGHT 400
#define SCROLL_TOTAL_HEIGHT 800
#define HIDDEN_AREA_HEIGHT (SCROLL_TOTAL_HEIGHT - DISPLAY_AREA_HEIGHT)
static void level_select_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(BLANK);
        vert_scrollarea_render(&data->scroll_area);
    EndTextureMode();
}

static void level_select_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    switch(action)
    {
        case ACTION_UP:
            if (pressed)
            {
                data->scroll_area.scroll_pos += 3;
                data->scroll_area.scroll_pos = Clamp(data->scroll_area.scroll_pos,data->scroll_area.scroll_bounds.x, data->scroll_area.scroll_bounds.y);
            }
        break;
        case ACTION_DOWN:
            if (pressed)
            {
                data->scroll_area.scroll_pos -= 3;
                data->scroll_area.scroll_pos = Clamp(data->scroll_area.scroll_pos,data->scroll_area.scroll_bounds.x, data->scroll_area.scroll_bounds.y);
            }
        break;
        case ACTION_EXIT:
            if(scene->engine != NULL)
            {
                change_scene(scene->engine, MAIN_MENU_SCENE);
            }
        break;
        case ACTION_CONFIRM:
            if (data->level_pack != NULL && data->scroll_area.curr_selection < data->level_pack->n_levels)
            {
                // TODO: Need to load the current level
                change_scene(scene->engine, LEVEL_SELECT_SCENE);

            }
        break;
        default:
        break;
    }
}

void init_level_select_scene(LevelSelectScene_t* scene)
{
    init_scene(&scene->scene, &level_select_do_action);
    add_scene_layer(
        &scene->scene, 400, DISPLAY_AREA_HEIGHT,
        (Rectangle){0, 0, 400, DISPLAY_AREA_HEIGHT}
    );
    vert_scrollarea_init(&scene->data.scroll_area, (Rectangle){50, 50, 150, DISPLAY_AREA_HEIGHT - 50}, (Vector2){150, SCROLL_TOTAL_HEIGHT});
    vert_scrollarea_set_item_dims(&scene->data.scroll_area, FONT_SIZE, TEXT_PADDING);
    char buf[32];
    ScrollAreaRenderBegin(&scene->data.scroll_area);
        ClearBackground(BLANK);
        if (scene->data.level_pack != NULL)
        {
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
}
void free_level_select_scene(LevelSelectScene_t* scene)
{

    vert_scrollarea_free(&scene->data.scroll_area);
    free_scene(&scene->scene);
}
