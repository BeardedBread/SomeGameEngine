#include "menu_impl.h"


static void menu_scene_render_func(Scene_t *scene)
{
}

static void menu_do_action(Scene_t *scene, ActionType_t action, bool pressed)
{
}

void init_menu_scene(MenuScene_t *scene)
{
    init_scene(&scene->scene, MENU_SCENE, &menu_scene_render_func, &menu_do_action);
    scene->scene.scene_data = &scene->data;
}

void free_menu_scene(MenuScene_t *scene)
{
    free_scene(&scene->scene);
}
