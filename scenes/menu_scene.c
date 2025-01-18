#include "scene_impl.h"
#include "assets_tag.h"
#include "raymath.h"
#include <stdio.h>

static void menu_scene_render_func(Scene_t* scene)
{
    MenuSceneData_t* data = &(CONTAINER_OF(scene, MenuScene_t, scene)->data);

    Sprite_t* spr = get_sprite(&scene->engine->assets, "title_spr");
    Sprite_t* title_spr = get_sprite(&scene->engine->assets, "title_board");
    Sprite_t* title_select = get_sprite(&scene->engine->assets, "title_select");
    Rectangle render_rec = scene->layers.render_layers[0].render_area;
    Font* menu_font = get_font(&scene->engine->assets, "MenuFont");
    BeginTextureMode(scene->layers.render_layers[0].layer_tex);
        ClearBackground(RAYWHITE);
        draw_sprite(spr, 0, (Vector2){0, 0}, 0, false);
        draw_sprite(title_spr, 0, (Vector2){32, 10}, 0, false);
        Vector2 title_sz = MeasureTextEx(*menu_font, "Bunny's Spelunking Adventure", 56, 0);
        Vector2 title_pos = {
            .x = (render_rec.width - title_sz.x) / 2,
            .y = 32 + (title_spr->frame_size.y - title_sz.y) / 2
        };
        DrawTextEx(*menu_font, "Bunny's Spelunking Adventure", title_pos, 56, 0, BLACK);

        const char* OPTIONS[3] = {"Start", "Credits", "Exit"};
        for (uint8_t i = 0; i < 3; ++i)
        {
            Vector2 pos = (Vector2){data->buttons[i].bbox.x, data->buttons[i].bbox.y};
            pos = Vector2Add(
                pos,
                shift_bbox(
                    (Vector2){
                        data->buttons[i].bbox.width,data->buttons[i].bbox.height
                    },
                title_select->frame_size,
                AP_MID_CENTER
                )
            );
            draw_sprite(title_select, 0, pos, 0, false);
            UI_button(data->buttons + i, OPTIONS[i]);
        }
    EndTextureMode();
}

static void exec_component_function(Scene_t* scene, int sel)
{
    switch(sel)
    {
        case 0:
            change_scene(scene->engine, LEVEL_SELECT_SCENE);
        break;
        case 2:
            scene->state = 0;
        break;
        default:
        break;
    }
}

static void menu_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    MenuSceneData_t* data = &(CONTAINER_OF(scene, MenuScene_t, scene)->data);
    unsigned int new_selection = data->selected_comp;
    if (!pressed)
    {
        if (data->mode == MOUSE_MODE)
        {
            data->mode = KEYBOARD_MODE;
        }
        else
        {
            switch(action)
            {
                case ACTION_UP:
                    if (new_selection == 0)
                    {
                        new_selection = data->max_comp - 1;
                    }
                    else
                    {
                        new_selection--;
                    }
                break;
                case ACTION_DOWN:
                    new_selection++;
                    if (new_selection == data->max_comp)
                    {
                        new_selection = 0;
                    }
                break;
                case ACTION_LEFT:
                break;
                case ACTION_RIGHT:
                break;
                default:
                break;
            }
        }
        data->buttons[data->selected_comp].state = STATE_NORMAL;
        printf("new: %u, old %u\n", new_selection, data->selected_comp);

        data->buttons[new_selection].state = STATE_FOCUSED;
        data->selected_comp = new_selection;
        if (action == ACTION_CONFIRM && scene->engine != NULL)
        {
            exec_component_function(scene, data->selected_comp);
        }
    }
}

static void gui_loop(Scene_t* scene)
{
    MenuSceneData_t* data = &(CONTAINER_OF(scene, MenuScene_t, scene)->data);
    if (Vector2LengthSqr(GetMouseDelta()) > 1)
    {
        data->mode = MOUSE_MODE;
    }
    if (data->mode == KEYBOARD_MODE)
    {
    }
    else
    {
        data->buttons[data->selected_comp].state = STATE_NORMAL;
        for (size_t i = 0;i < data->max_comp; i++)
        {
           if ((data->buttons[i].state != STATE_DISABLED))
            {
                Vector2 mousePoint = GetMousePosition();

                // Check button state
                if (CheckCollisionPointRec(mousePoint, data->buttons[i].bbox))
                {
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                    {
                        data->buttons[i].state = STATE_PRESSED;
                    }
                    else
                    {
                        data->buttons[i].state = STATE_FOCUSED;
                    }
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                    {
                        data->buttons[i].pressed = true;
                        if (scene->engine != NULL)
                        {
                            exec_component_function(scene, i);
                        }
                    }
                    data->selected_comp = i;
                }
            }
        }
    }
}

void init_menu_scene(MenuScene_t* scene)
{
    init_scene(&scene->scene, &menu_do_action, 0);

    sc_array_add(&scene->scene.systems, &gui_loop);
    sc_array_add(&scene->scene.systems, &menu_scene_render_func);
    
    int button_x = scene->scene.engine->intended_window_size.x / 8;
    int button_y = scene->scene.engine->intended_window_size.y / 3;
    int spacing = 100;
    scene->data.buttons[0] = (UIComp_t) {
        .bbox = {button_x,button_y,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[1] = (UIComp_t) {
        .bbox = {button_x,button_y+spacing,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[2] = (UIComp_t) {
        .bbox = {button_x,button_y+spacing * 2,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[3] = (UIComp_t) {
        .bbox = {button_x,button_y+spacing * 3,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.max_comp = 4;
    scene->data.selected_comp = 0;
    scene->data.mode = MOUSE_MODE;
    add_scene_layer(
        &scene->scene, scene->scene.engine->intended_window_size.x,
        scene->scene.engine->intended_window_size.y,
        (Rectangle){
            0, 0,
            scene->scene.engine->intended_window_size.x,
            scene->scene.engine->intended_window_size.y
        }
    );

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, KEY_ENTER, ACTION_CONFIRM);
}

void free_menu_scene(MenuScene_t* scene)
{
    free_scene(&scene->scene);
}
