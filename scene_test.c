#include "mempool.h"
#include "scene_impl.h"
#include <stdio.h>

int main(void)
{
    LevelScene_t scene;
    init_level_scene(&scene);
    update_scene(&scene.scene);
    free_level_scene(&scene);
}
