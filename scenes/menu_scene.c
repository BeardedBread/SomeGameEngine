#include "scene_impl.h"
#include "raymath.h"
#include <stdio.h>


static void menu_scene_render_func(Scene_t* scene)
{
    MenuSceneData_t* data = &(CONTAINER_OF(scene, MenuScene_t, scene)->data);
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("This is a game", 25, 220, 12, BLACK);
        UI_button(data->buttons, "Start");
        UI_button(data->buttons + 1, "Sandbox");
        UI_button(data->buttons + 2, "Continue");
        UI_button(data->buttons + 3, "Exit");
    EndDrawing();
}

static void exec_component_function(Scene_t* scene, int sel)
{
    switch(sel)
    {
        case 0:
            change_scene(scene->engine, 1);
        break;
        case 1:
            change_scene(scene->engine, 2);
        break;
        case 3:
            scene->state = SCENE_ENDED;
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
    //init_scene(&scene->scene, MENU_SCENE, &menu_scene_render_func, &menu_do_action);
    init_scene(&scene->scene, &menu_scene_render_func, &menu_do_action);

    sc_array_add(&scene->scene.systems, &gui_loop);
    
    scene->data.buttons[0] = (UIComp_t) {
        .bbox = {25,255,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[1] = (UIComp_t) {
        .bbox = {25,300,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[2] = (UIComp_t) {
        .bbox = {25,345,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.buttons[3] = (UIComp_t) {
        .bbox = {25,390,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
    scene->data.max_comp = 4;
    scene->data.selected_comp = 0;
    scene->data.mode = MOUSE_MODE;

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
