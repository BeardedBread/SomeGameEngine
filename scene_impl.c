#include "scene_impl.h"
#include "raylib.h"

static void level_scene_render_func(Scene_t* scene)
{
    return;
}

static void movement_update_system(Scene_t* scene)
{
    return;
}
void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func);
    scene->scene.scene_data = &scene->data;
    scene->data.player = NULL;

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &movement_update_system);
}
void free_level_scene(LevelScene_t *scene)
{
    free_scene(&scene->scene);
}

void reload_level_scene(LevelScene_t *scene)
{
}
