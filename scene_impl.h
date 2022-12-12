/* This implements the scene
 * ie it will insert the scene-specific data and systems
 * based on the function called
 * */
#ifndef __SCENE_IMPL_H
#define __SCENE_IMPL_H
#include "scene.h"
typedef struct Tile
{
    bool solid;
    // TODO: add a map?
}Tile_t;

typedef struct TileGrid
{
    unsigned int width;
    unsigned int height;
    Tile_t * const tiles;
}TileGrid_t;

typedef struct LevelSceneData
{
    Vector2 player_dir;
    TileGrid_t tilemap;
}LevelSceneData_t;

typedef struct LevelScene
{
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t *scene);
void free_level_scene(LevelScene_t *scene);
void reload_level_scene(LevelScene_t *scene);
#endif // __SCENE_IMPL_H
