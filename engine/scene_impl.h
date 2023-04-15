/* This implements the scene
 * ie it will insert the scene-specific data and systems
 * based on the function called
 * */
#ifndef __SCENE_IMPL_H
#define __SCENE_IMPL_H
#include "engine.h"
#include "gui.h"

typedef struct Tile {
    bool solid;
    unsigned int water_level;
    struct sc_map_64 entities_set;
}Tile_t;

typedef struct TileGrid {
    unsigned int width;
    unsigned int height;
    unsigned int n_tiles;
    Tile_t * tiles;
}TileGrid_t;

typedef struct LevelSceneData {
    TileGrid_t tilemap;
    struct sc_map_32 collision_events;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
void reload_level_scene(LevelScene_t* scene);

typedef enum GuiMode {
    KEYBOARD_MODE,
    MOUSE_MODE
} GuiMode_t;

typedef struct MenuSceneData {
    UIComp_t buttons[3];
    int selected_comp;
    int max_comp;
    GuiMode_t mode;
} MenuSceneData_t;

typedef struct MenuScene {
    Scene_t scene;
    MenuSceneData_t data;
} MenuScene_t;

void init_menu_scene(MenuScene_t* scene);
void free_menu_scene(MenuScene_t* scene);
#endif // __SCENE_IMPL_H
