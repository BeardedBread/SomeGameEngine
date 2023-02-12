#include "menu_impl.h"


static void menu_scene_render_func(Scene_t *scene)
{
    MenuSceneData_t *data = (MenuSceneData_t *)scene->scene_data;
    DrawText("This is a game", 25, 220, 12, BLACK);
    UI_button(data->buttons, "Start");
    UI_button(data->buttons + 1, "Exit");
}

static void menu_do_action(Scene_t *scene, ActionType_t action, bool pressed)
{
}

static void gui_loop(Scene_t* scene)
{
    MenuSceneData_t *data = (MenuSceneData_t *)scene->scene_data;
    for (size_t i=0;i<2;i++)
    {
       if ((data->buttons[i].state != STATE_DISABLED))
        {
            Vector2 mousePoint = GetMousePosition();

            // Check button state
            if (CheckCollisionPointRec(mousePoint, data->buttons[i].bbox))
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) data->buttons[i].state = STATE_PRESSED;
                else data->buttons[i].state = STATE_FOCUSED;

                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) data->buttons[i].pressed = true;
            }
            else
            {
                data->buttons[i].state = STATE_NORMAL;
            }
        }
    }
}

void init_menu_scene(MenuScene_t *scene)
{
    init_scene(&scene->scene, MENU_SCENE, &menu_scene_render_func, &menu_do_action);
    scene->scene.scene_data = &scene->data;

    sc_array_add(&scene->scene.systems, &gui_loop);
    
    scene->data.buttons[0] = (UIComp_t)
    {
        .bbox = {25,255,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };

    scene->data.buttons[1] = (UIComp_t)
    {
        .bbox = {25,300,125,30},
        .state = STATE_NORMAL,
        .alpha = 1.0
    };
}

void free_menu_scene(MenuScene_t *scene)
{
    free_scene(&scene->scene);
}
