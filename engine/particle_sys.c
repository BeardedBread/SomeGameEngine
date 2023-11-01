#include "particle_sys.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// TEMPORARY VARIABLE: NEED TO FIND A WAY TO DEAL WITH THIS
#define DELTA_T 0.017

void init_particle_system(ParticleSystem_t* system)
{
    memset(system, 0, sizeof(ParticleSystem_t));
    sc_queue_init(&system->free_list);
    for ( uint32_t i = 1; i <= MAX_PARTICLE_EMITTER; ++i)
    {
        sc_queue_add_last(&system->free_list, i);
    }
    system->tail_idx = 0;
}
void add_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter)
{
    if (sc_queue_empty(&system->free_list)) return;
    uint32_t idx = sc_queue_del_first(&system->free_list);
    system->emitter_list[system->tail_idx].next = idx;
    system->tail_idx = idx;
    system->emitter_list[idx].next = 0;
    system->emitters[idx] = *in_emitter;
    system->emitters[idx].active = true;
    if (system->emitters[idx].n_particles > MAX_PARTICLES)
    {
        system->emitters[idx].n_particles = MAX_PARTICLES;
    }
    ParticleEmitter_t* emitter = system->emitters + idx;
    // Generate particles based on type
    for (uint32_t i = 0; i < emitter->n_particles; ++i)
    {
        uint32_t lifetime = (emitter->config.particle_lifetime[1] - emitter->config.particle_lifetime[0]);
        emitter->particles[i].timer = emitter->config.particle_lifetime[0];
        emitter->particles[i].timer += rand() % lifetime;
        emitter->particles[i].alive = true;

        float angle = emitter->config.launch_range[1] - emitter->config.launch_range[0];
        angle *= (float)rand() / (float)RAND_MAX;
        angle += emitter->config.launch_range[0];
        if(angle > 360) angle -= 360;
        if(angle < -360) angle += 360;
        angle *= PI / 180;

        float speed = emitter->config.speed_range[1] - emitter->config.speed_range[0];
        speed *= (float)rand() / (float)RAND_MAX;
        speed += emitter->config.speed_range[0];

        emitter->particles[i].velocity.x = speed * cos(angle);
        emitter->particles[i].velocity.y = speed * sin(angle);
        emitter->particles[i].position = emitter->position;
        emitter->particles[i].rotation = angle;
        emitter->particles[i].angular_vel = -10 + 20 * (float)rand() / (float)RAND_MAX;
        emitter->particles[i].size = 10 + 20 * (float)rand() / (float)RAND_MAX;
;
    }
}
void update_particle_system(ParticleSystem_t* system)
{
    uint32_t emitter_idx = system->emitter_list[0].next;
    uint32_t last_idx = 0;

    while (emitter_idx != 0)
    {
        uint32_t next_idx = system->emitter_list[emitter_idx].next;
        ParticleEmitter_t* emitter = system->emitters + emitter_idx;
        uint32_t inactive_count = 0;
        for (uint32_t i = 0; i < emitter->n_particles; ++i)
        {
            if (emitter->particles[i].alive)
            {
                if (emitter->config.update_func != NULL)
                {
                    emitter->config.update_func(emitter->particles + i, emitter->config.user_data);
                }

                // Lifetime update
                if (emitter->particles[i].timer > 0) emitter->particles[i].timer--;
                if (emitter->particles[i].timer == 0)
                {
                    emitter->particles[i].alive = false;
                    inactive_count++;
                }
            }
            else
            {
                inactive_count++;
            }
        }
        if (inactive_count == emitter->n_particles)
        {
            emitter->active = false;
            system->emitter_list[last_idx].next = system->emitter_list[emitter_idx].next;
            system->emitter_list[emitter_idx].next = 0;
            if (system->tail_idx == emitter_idx)
            {
                system->tail_idx = last_idx;
            }
            sc_queue_add_last(&system->free_list, emitter_idx);
            emitter_idx = last_idx;
        }
        last_idx = emitter_idx;
        emitter_idx = next_idx;
    }
}
void draw_particle_system(ParticleSystem_t* system)
{
    uint32_t emitter_idx = system->emitter_list[0].next;

    while (emitter_idx != 0)
    {
        ParticleEmitter_t* emitter = system->emitters + emitter_idx;

        Particle_t* part = emitter->particles;
        for (uint32_t i = 0; i < emitter->n_particles; ++i, ++part)
        {
            if (part->alive)
            {
                if (emitter->tex == NULL || emitter->tex->width == 0)
                {
                    Rectangle rect = {
                        .x = part->position.x,
                        .y = part->position.y,
                        .width = part->size,
                        .height = part->size
                    };
                    Vector2 origin = (Vector2){
                        part->size / 2, 
                        part->size / 2
                    };
                    DrawRectanglePro(rect, origin, part->rotation, BLACK);
                }
                else
                {
                    Rectangle source = {
                        0.0f, 0.0f,
                        (float)emitter->tex->width, (float)emitter->tex->height
                    };
                    
                    Rectangle dest = {
                        part->position.x, part->position.y,
                        (float)emitter->tex->width, (float)emitter->tex->height
                    };
                    Vector2 origin = { (float)emitter->tex->width / 2, (float)emitter->tex->height / 2 };
                    DrawTexturePro(
                        *emitter->tex,
                        source, dest, origin,
                        part->rotation, WHITE
                    );
                }
            }
        }
        emitter_idx = system->emitter_list[emitter_idx].next;
    }
}
void deinit_particle_system(ParticleSystem_t* system)
{
    sc_queue_term(&system->free_list);
}
