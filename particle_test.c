#include "particle_sys.h"
#include <stdio.h>
#include <unistd.h>
#include "raylib.h"
#include "raymath.h"
#include "constants.h"
static const Vector2 GRAVITY = {0, GRAV_ACCEL};

void simple_particle_system_update(Particle_t* part, void* user_data)
{

    float delta_time = DELTA_T; // TODO: Will need to think about delta time handling
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

    EmitterConfig_t conf ={
        .launch_range = {0, 360},
        .speed_range = {400, 2000},
        .particle_lifetime = {30, 110},
        .update_func = &simple_particle_system_update
    };

    ParticleEmitter_t emitter = {
        .config = conf,
        .n_particles = MAX_PARTICLES,
        .one_shot = true,
    };

    bool key_press = false;
    while(!WindowShouldClose())
    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            key_press = true;
        }
        else if (key_press && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            emitter.position = GetMousePosition();
            add_particle_emitter(&part_sys, &emitter);
            key_press = false;
        }
        update_particle_system(&part_sys);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            draw_particle_system(&part_sys);
        EndDrawing();
    } 
    CloseWindow();
}
