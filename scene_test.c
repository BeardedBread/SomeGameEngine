#include "mempool.h"
#include "scene_impl.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    InitWindow(320, 240, "raylib");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    init_memory_pools();
    LevelScene_t scene;
    init_level_scene(&scene);
    Entity_t *p_ent = add_entity(&scene.scene.ent_manager, PLAYER_ENT_TAG);
    CBBox_t *p_bbox = add_component(&scene.scene.ent_manager, p_ent, CBBOX_COMP_T);
    p_bbox->size.x = 30;
    p_bbox->size.y = 30;
    CTransform_t * p_ctran = (CTransform_t *)add_component(&scene.scene.ent_manager, p_ent, CTRANSFORM_COMP_T);
    p_ctran->accel.x = 20;
    p_ctran->accel.y = 10;
    update_entity_manager(&scene.scene.ent_manager);
    for (size_t step = 0; step < 120; step++)
    {
        puts("======");
        printf("Step %lu\n", step);
        update_scene(&scene.scene);
        printf("Position: %f,%f\n", p_ctran->position.x, p_ctran->position.y);
        printf("Velocity: %f,%f\n", p_ctran->velocity.x, p_ctran->velocity.y);
        printf("Accel: %f,%f\n", p_ctran->accel.x, p_ctran->accel.y);
        // This is needed to advance time delta
        BeginDrawing();
            // TODO: Call the current scene Render function
            render_scene(&scene.scene);
            ClearBackground(RAYWHITE);
        EndDrawing();
    } 
    CloseWindow();
    free_level_scene(&scene);
}
