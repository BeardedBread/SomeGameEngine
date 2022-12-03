#include "raylib.h"
#include "scene.h"
#define N_SCENES 3

Scene_t scenes[N_SCENES];
unsigned int current_scene;

const int screenWidth = 800;
const int screenHeight = 450;

static void load_assets(void)
{
}

int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    //char dir[7]; 

    InitWindow(screenWidth, screenHeight, "raylib");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    load_assets();
    //Camera2D camera = { 0 };
    //camera.offset = (Vector2){0,0};
    //camera.rotation = 0.0f;
    //camera.zoom = 1.0f;
    while (!WindowShouldClose())
    {
        // TODO: Handle keystrokes/input here
        // TODO: Update Scene here or scene changes

        BeginDrawing();
            // TODO: Call the current scene Render function
            ClearBackground(RAYWHITE);
        EndDrawing();
    }
    CloseWindow(); 
}
