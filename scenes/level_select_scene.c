#include "scene_impl.h"
#include "assets_tag.h"
#include "raymath.h"
#include <stdio.h>

static void level_select_render_func(Scene_t* scene)
{
    LevelSelectSceneData_t* data = &(CONTAINER_OF(scene, LevelSelectScene_t, scene)->data);
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(RAYWHITE);
        Rectangle draw_rec = (Rectangle){
            0, data->scroll,
            scene->layers.render_layers[0].render_area.width,
            scene->layers.render_layers[0].render_area.height
        };
        Vector2 draw_pos = {50, 50};
        draw_rec.height *= -1;
        DrawTextureRec(
            data->level_display.texture,
            draw_rec,
            draw_pos,
            WHITE
        );
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
                data->scroll += 3;
                data->scroll = Clamp(data->scroll, 0, 400);
            }
        break;
        case ACTION_DOWN:
            if (pressed)
            {
                data->scroll -= 3;
                data->scroll = Clamp(data->scroll, 0, 400);
            }
        break;
        case ACTION_EXIT:
            if(scene->engine != NULL)
            {
                change_scene(scene->engine, MAIN_MENU_SCENE);
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
        &scene->scene, 300, 400,
        (Rectangle){0, 0, 300, 400}
    );
    scene->data.scroll = 400;
    scene->data.level_display = LoadRenderTexture(300, 800);
    const unsigned int n_elems = 800 / (12+3);
    char buf[32];
    BeginTextureMode(scene->data.level_display);
        ClearBackground(GRAY);
        if (scene->data.level_pack != NULL)
        {
            for (unsigned int i = 0; i < scene->data.level_pack->n_levels; ++i)
            {
                DrawText(scene->data.level_pack->levels[i].level_name, 0, (12+3) * i, 12, BLACK);
            }
            for (unsigned int i = scene->data.level_pack->n_levels; i < n_elems; ++i)
            {
                DrawText("---", 0, (12+3) * i, 12, BLACK);
            }
        }
        else
        {
            for (unsigned int i = 0; i < n_elems; ++i)
            {
                sprintf(buf, "Level %u", i); 
                DrawText(buf, 0, (12+3) * i, 12, BLACK);
            }
        }
    EndTextureMode();

    sc_array_add(&scene->scene.systems, &level_select_render_func);
    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_EXIT);
}
void free_level_select_scene(LevelSelectScene_t* scene)
{
    free_scene(&scene->scene);
}
