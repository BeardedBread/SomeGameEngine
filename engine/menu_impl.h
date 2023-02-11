#ifndef __MENU_IMPL_H
#define __MENU_IMPL_H
#include "scene.h"

typedef struct MenuSceneData
{
    Entity_t *menus[8];
    Entity_t *menu_opts[32];
}MenuSceneData_t;

typedef struct MenuScene
{
    Scene_t scene;
    MenuSceneData_t data;
}MenuScene_t;

void init_menu_scene(MenuScene_t *scene);
void free_menu_scene(MenuScene_t *scene);
#endif
