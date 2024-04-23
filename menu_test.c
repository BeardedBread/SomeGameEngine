#include "mempool.h"
#include "scene_impl.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>

// Maintain own queue to handle key presses
struct sc_queue_32 key_buffer;

const float DT = 1.0f/60.0f;
int main(void)
{
    sc_queue_init(&key_buffer);
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    init_memory_pools();
    MenuScene_t scene;
    init_menu_scene(&scene);
    while(true)
    {

        // This entire key processing relies on the assumption that a pressed key will
        // appear in the polling of raylib

        unsigned int sz = sc_queue_size(&key_buffer);
        // Process any existing pressed key
        for (size_t i = 0; i < sz; i++)
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

        float frame_time = GetFrameTime();
        float delta_time = fminf(frame_time, DT);
        update_scene(&scene.scene, delta_time);
        update_entity_manager(&scene.scene.ent_manager);
        // This is needed to advance time delta
        BeginDrawing();
            render_scene(&scene.scene);
            ClearBackground(RAYWHITE);
        EndDrawing();
        if (WindowShouldClose()) break;
    } 
    free_menu_scene(&scene);
    sc_queue_term(&key_buffer);
    CloseWindow();
}
