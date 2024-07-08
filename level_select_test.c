#include "mempool.h"
#include "scene_impl.h"
#include "gui.h"
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
    init_UI();
    LevelSelectScene_t scene;
    init_level_select_scene(&scene);
    scene.scene.bg_colour = RAYWHITE;
    while(true)
    {

        Vector2 raw_mouse_pos = GetMousePosition();
        scene.scene.mouse_pos = raw_mouse_pos;
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
        ActionType_t action = sc_map_get_64(&scene.scene.action_map, MOUSE_BUTTON_LEFT);
        if (sc_map_found(&scene.scene.action_map))
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                do_action(&scene.scene, action, true);
            }
            else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                do_action(&scene.scene, action, false);
            }
        }

        float frame_time = GetFrameTime();
        float delta_time = fminf(frame_time, DT);
        update_scene(&scene.scene, delta_time);
        // This is needed to advance time delta
        render_scene(&scene.scene);
        if (WindowShouldClose()) break;
    } 
    free_level_select_scene(&scene);
    sc_queue_term(&key_buffer);
    CloseWindow();
}
