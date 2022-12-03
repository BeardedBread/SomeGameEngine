/* This implements the scene
 * ie it will insert the scene-specific data and systems
 * based on the function called
 * */
#ifndef __SCENE_IMPL_H
#define __SCENE_IMPL_H
#include "scene.h"
typedef struct LevelSceneData
{
    Entity_t * player;
}LevelSceneData_t ;
typedef struct LevelScene
{
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t *scene);
void free_level_scene(LevelScene_t *scene);
void reload_level_scene(LevelScene_t *scene);
#endif // __SCENE_IMPL_H
