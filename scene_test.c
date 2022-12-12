#include "mempool.h"
#include "scene_impl.h"
#include <stdio.h>
#include <unistd.h>

// Use double buffer to handle key presses
// Can do single buffer, but need careful handling
unsigned int curr_keybuf = 0;
unsigned int next_keybuf = 1;
struct sc_queue_32 key_buffer[2];

int main(void)
{
    sc_queue_init(key_buffer);
    sc_queue_init(key_buffer + 1);
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

        // Process any existing pressed key
        while (!sc_queue_empty(key_buffer + curr_keybuf))
        {
            int button = sc_queue_del_first(key_buffer + curr_keybuf);
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (IsKeyReleased(button))
            {
                do_action(&scene.scene, action, false);
            }
            else
            {
                do_action(&scene.scene, action, true);
                sc_queue_add_last(key_buffer + next_keybuf, button);
            }
        }

        // Detect new key presses
        while(true)
        {
            int button = GetKeyPressed();
            printf("Button pressed: %d\n", button);
            if (button == 0) break;
            ActionType_t action = sc_map_get_64(&scene.scene.action_map, button);
            if (!sc_map_found(&scene.scene.action_map)) continue;
            printf("Action mapped: %d\n", action);
            do_action(&scene.scene, action, true);
            sc_queue_add_last(key_buffer + next_keybuf, button);
        }

        // Swap keystroke buffer;
        {
            unsigned int tmp = curr_keybuf;
            curr_keybuf = next_keybuf;
            next_keybuf = tmp;
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
    sc_queue_term(key_buffer);
    sc_queue_term(key_buffer + 1);
}
