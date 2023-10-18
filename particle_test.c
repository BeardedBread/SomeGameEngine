#include "particle_sys.h"
#include <stdio.h>
#include <unistd.h>
#include "raylib.h"

int main(void)
{
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    static ParticleSystem_t part_sys = {0
    };

    while(!WindowShouldClose())
    {
        update_particle_system(&part_sys);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            draw_particle_system(&part_sys);
        EndDrawing();
    } 
    CloseWindow();
}
