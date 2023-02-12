#ifndef __MENU_IMPL_H
#define __MENU_IMPL_H
#include "scene.h"
#include "gui.h"

typedef enum GuiMode
{
    KEYBOARD_MODE,
    MOUSE_MODE
}GuiMode_t;

typedef struct MenuSceneData
{
    UIComp_t buttons[3];
    int selected_comp;
    int max_comp;
    GuiMode_t mode;
}MenuSceneData_t;

typedef struct MenuScene
{
    Scene_t scene;
    MenuSceneData_t data;
}MenuScene_t;

void init_menu_scene(MenuScene_t *scene);
void free_menu_scene(MenuScene_t *scene);
#endif
