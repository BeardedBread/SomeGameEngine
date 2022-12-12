#include "mempool.h"
#include "scene_impl.h"
#include <stdio.h>
#include <unistd.h>

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

int main(void)
{
    sc_queue_init(&key_buffer);
    InitWindow(320, 240, "raylib");
    SetTargetFPS(60);
    init_memory_pools();
    LevelScene_t scene;
    init_level_scene(&scene);
    Entity_t *p_ent = add_entity(&scene.scene.ent_manager, PLAYER_ENT_TAG);

    CBBox_t *p_bbox = add_component(&scene.scene.ent_manager, p_ent, CBBOX_COMP_T);
    p_bbox->size.x = 30;
    p_bbox->size.y = 30;
    add_component(&scene.scene.ent_manager, p_ent, CTRANSFORM_COMP_T);
    update_entity_manager(&scene.scene.ent_manager);
    //for (size_t step = 0; step < 6000; step++)
    while(true)
    {

        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib

        unsigned int sz = sc_queue_size(&key_buffer);
        // Process any existing pressed key
        for (size_t i=0; i<sz; i++)
        {
            int button = sc_queue_del_first(&key_buffer);
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (IsKeyReleased(button))
            {
                do_action(&scene.scene, action, false);
            }
            else
            {
                do_action(&scene.scene, action, true);
                sc_queue_add_last(&key_buffer, button);
            }
        }

        // Detect new key presses
        while(true)
        {
            int button = GetKeyPressed();
            if (button == 0) break;
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (!sc_map_found(&scene.scene.action_map)) continue;
            do_action(&scene.scene, action, true);
            sc_queue_add_last(&key_buffer, button);
        }

        update_scene(&scene.scene);
        // This is needed to advance time delta
        BeginDrawing();
            render_scene(&scene.scene);
            ClearBackground(RAYWHITE);
        EndDrawing();
        if (WindowShouldClose()) break;
    } 
    CloseWindow();
    free_level_scene(&scene);
    sc_queue_term(&key_buffer);
}
