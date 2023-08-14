/* This implements the scene
 * ie it will insert the scene-specific data and systems
 * based on the function called
 * */
#ifndef __SCENE_IMPL_H
#define __SCENE_IMPL_H
#include "engine.h"
#include "gui.h"

#define CONTAINER_OF(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

typedef enum TileType {
    EMPTY_TILE = 0,
    SOLID_TILE,
    ONEWAY_TILE,
    LADDER,
    SPIKES,
} TileType_t;

typedef struct LevelSceneData {
    TileGrid_t tilemap;
    RenderTexture2D game_viewport;
    Rectangle game_rec;
    Camera2D cam;
    Sprite_t* tile_sprites[MAX_TILE_TYPES];
    LevelPack_t* level_pack;
    unsigned int current_level;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
void reload_level_scene(LevelScene_t* scene);
bool load_a_level(LevelScene_t* scene, unsigned int level_num);

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
