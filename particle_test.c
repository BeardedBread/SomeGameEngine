#include "particle_sys.h"
#include <stdio.h>
#include <unistd.h>
#include "raylib.h"
#include "raymath.h"
#include "constants.h"
static const Vector2 GRAVITY = {0, GRAV_ACCEL};

void simple_particle_system_update(Particle_t* part, void* user_data)
{
    (void)user_data;

    float delta_time = DELTA_T; // TODO: Will need to think about delta time handling
    part->rotation += part->angular_vel;

    part->velocity =
        Vector2Add(
            part->velocity,
            Vector2Scale(GRAVITY, delta_time)
        );

    float mag = Vector2Length(part->velocity);
    part->velocity = Vector2Scale(
        Vector2Normalize(part->velocity),
        (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag
    );
    // 3 dp precision
    if (fabs(part->velocity.x) < 1e-3) part->velocity.x = 0;
    if (fabs(part->velocity.y) < 1e-3) part->velocity.y = 0;

    part->position = Vector2Add(
        part->position,
        Vector2Scale(part->velocity, delta_time)
    );

    // Level boundary collision
    {

        if(
            part->position.x + part->size < 0 || part->position.x - part->size > 1280
            || part->position.y + part->size < 0 || part->position.y - part->size > 640
        )
        {
            part->timer = 0;
        }
    }
}
int main(void)
{
    InitWindow(1280, 640, "raylib");
    SetTargetFPS(60);
    static ParticleSystem_t part_sys = {0};

    init_particle_system(&part_sys);
    Texture2D tex = LoadTexture("res/bomb.png");
    Sprite_t spr = {
        .texture = &tex,
        .frame_size = (Vector2){tex.width, tex.height},
        .origin = (Vector2){0, 0},
        .anchor = (Vector2){tex.width / 2, tex.height / 2},
        .frame_count = 0,
        .elapsed = 0,
        .speed = 0,
        .name = "test_spr"
    };

    EmitterConfig_t conf ={
        .one_shot = true,
        .launch_range = {0, 360},
        .speed_range = {400, 2000},
        .particle_lifetime = {30, 110},
        .type = EMITTER_BURST,
    };

    ParticleEmitter_t emitter = {
        .config = &conf,
        .n_particles = MAX_PARTICLES,
        .update_func = &simple_particle_system_update,
        .spr = (tex.width == 0) ? NULL : &spr,
    };

    EmitterConfig_t conf2 ={
        .one_shot = false,
        .launch_range = {45, 135},
        .speed_range = {300, 800},
        .particle_lifetime = {15, 30},
        .initial_spawn_delay = 5,
        .type = EMITTER_STREAM,
    };

    ParticleEmitter_t emitter2 = {
        .config = &conf2,
        .n_particles = MAX_PARTICLES,
        .update_func = &simple_particle_system_update,
        .spr = (tex.width == 0) ? NULL : &spr,
    };

    EmitterHandle han = load_in_particle_emitter(&part_sys, &emitter2);
    assert(han != 0);

    bool key_press = false;
    uint8_t key2_press = 0;
    char text_buffer[32];
    while(!WindowShouldClose())
    {
        Vector2 mouse_pos = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            key_press = true;
        }
        else if (key_press && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            emitter.position = mouse_pos;
            play_particle_emitter(&part_sys, &emitter);
            key_press = false;
        }

        key2_press <<= 1;
        key2_press |= IsMouseButtonDown(MOUSE_RIGHT_BUTTON)? 1: 0;
        key2_press &= 0b11;

        if (key2_press == 0b01)
        {
            update_emitter_handle_position(&part_sys, han, mouse_pos);
            play_emitter_handle(&part_sys, han);
        }
        else if(key2_press == 0b11)
        {
            update_emitter_handle_position(&part_sys, han, mouse_pos);
        }
        else if (key2_press == 0b10)
        {
            pause_emitter_handle(&part_sys, han);
        }

        update_particle_system(&part_sys);
        sprintf(text_buffer, "free: %u", get_number_of_free_emitter(&part_sys));
        BeginDrawing();
            ClearBackground(RAYWHITE);
            draw_particle_system(&part_sys);
            DrawText(text_buffer, 0, 0, 16, BLACK);
        EndDrawing();
    } 
    UnloadTexture(tex);
    CloseWindow();
    deinit_particle_system(&part_sys);
}
