#include "assets.h"
#include <stdio.h>
#include <unistd.h>
#include "raylib.h"

int main(void)
{
    InitWindow(1280, 640, "raylib");
    InitAudioDevice();
    SetTargetFPS(60);

    Assets_t assets;
    init_assets(&assets);

    Texture2D* tex = add_texture(&assets, "testtex", "res/test_tex.png");

    Sprite_t* spr = add_sprite(&assets, "testspr1", tex);
    spr->origin = (Vector2){0, 0};
    spr->frame_size = (Vector2){32, 32};

    Sprite_t* spr2 = add_sprite(&assets, "testspr2", tex);
    spr2->frame_count = 4;
    spr2->origin = (Vector2){0, 0};
    spr2->frame_size = (Vector2){32, 32};
    spr2->speed = 15;

    Sound* snd = add_sound(&assets, "testsnd", "res/sound.ogg");
    Font* fnt = add_font(&assets, "testfont", "res/test_font.ttf");

    while(!WindowShouldClose())
    {
        if (IsKeyReleased(KEY_C))
        {
            PlaySound(*snd);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextEx(*fnt, "Press C to play a sound", (Vector2){64, 64}, 24, 1, RED);

            // Draw the static Sprite and animated Sprite
            draw_sprite(spr, (Vector2){64,128});
            draw_sprite(spr2, (Vector2){64,180});
        EndDrawing();

        // Update the animated Sprite
        spr2->elapsed++;
        if (spr2->elapsed == spr2->speed)
        {
            spr2->current_frame++;
            spr2->current_frame %= spr2->frame_count;
            spr2->elapsed = 0;
        }
    } 
    CloseWindow();
    term_assets(&assets);
}
