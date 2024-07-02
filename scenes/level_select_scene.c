#include "scene_impl.h"
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
    BeginTextureMode(scene->data.level_display);
        ClearBackground(GRAY);
        for (unsigned int i = 0; i < n_elems; ++i)
        {
            char buf[32];
            sprintf(buf, "Level %u", i); 
            DrawText(buf, 0, (12+3) * i, 12, BLACK);
        }
    EndTextureMode();

    sc_array_add(&scene->scene.systems, &level_select_render_func);
    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
}
void free_level_select_scene(LevelSelectScene_t* scene)
{
    free_scene(&scene->scene);
}
