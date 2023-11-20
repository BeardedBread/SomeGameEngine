#include "particle_sys.h"
#include "assets.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

void init_particle_system(ParticleSystem_t* system)
{
    memset(system, 0, sizeof(ParticleSystem_t));
    sc_queue_init(&system->free_list);
    for ( uint32_t i = 1; i <= MAX_ACTIVE_PARTICLE_EMITTER; ++i)
    {
        sc_queue_add_last(&system->free_list, i);
    }
    system->tail_idx = 0;
}

uint16_t get_number_of_free_emitter(ParticleSystem_t* system)
{
    return sc_queue_size(&system->free_list);
}

static inline void spawn_particle(ParticleEmitter_t* emitter, uint32_t idx)
{
    uint32_t lifetime = (emitter->config->particle_lifetime[1] - emitter->config->particle_lifetime[0]);
    emitter->particles[idx].timer = emitter->config->particle_lifetime[0];
    emitter->particles[idx].timer += rand() % lifetime;
    emitter->particles[idx].alive = true;

    float angle = emitter->config->launch_range[1] - emitter->config->launch_range[0];
    angle *= (float)rand() / (float)RAND_MAX;
    angle += emitter->config->launch_range[0];
    if(angle > 360) angle -= 360;
    if(angle < -360) angle += 360;

    float speed = emitter->config->speed_range[1] - emitter->config->speed_range[0];
    speed *= (float)rand() / (float)RAND_MAX;
    speed += emitter->config->speed_range[0];

    emitter->particles[idx].velocity.x = speed * cos(angle * PI / 180);
    emitter->particles[idx].velocity.y = speed * sin(angle * PI / 180);
    emitter->particles[idx].position = emitter->position;
    emitter->particles[idx].rotation = angle;
    emitter->particles[idx].angular_vel = -10 + 20 * (float)rand() / (float)RAND_MAX;
    emitter->particles[idx].size = 10 + 20 * (float)rand() / (float)RAND_MAX;
    emitter->particles[idx].spawned = true;
;
}

uint16_t load_in_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter)
{
    if (in_emitter == NULL) return 0;
    if (in_emitter->config == NULL) return 0 ;
    if (in_emitter->config->type == EMITTER_UNKNOWN) return 0;

    if (sc_queue_empty(&system->free_list)) return 0;
    uint16_t idx = sc_queue_del_first(&system->free_list);

    system->emitters[idx] = *in_emitter;
    system->emitters[idx].active = true;
    if (system->emitters[idx].n_particles > MAX_PARTICLES)
    {
        system->emitters[idx].n_particles = MAX_PARTICLES;
    }
    system->emitter_list[idx].playing = false;
    return idx;
}

void play_emitter_handle(ParticleSystem_t* system, uint16_t handle)
{
    if (handle == 0) return;
    if (!system->emitter_list[handle].playing)
    {
        ParticleEmitter_t* emitter = system->emitters + handle;
        if (emitter->config->type == EMITTER_BURST)
        {
            for (uint32_t i = 0; i < emitter->n_particles; ++i)
            {
                spawn_particle(emitter, i);
            }
        }
        else if (emitter->config->type == EMITTER_STREAM)
        {
            // TODO: deal with stream type
            //spawn_particle(emitter, 0);
            uint32_t incr = 0;
            for (uint32_t i = 0; i < emitter->n_particles; ++i)
            {
                emitter->particles[i].timer = incr;
                emitter->particles[i].alive = false;
                emitter->particles[i].spawned = false;
                incr += emitter->config->initial_spawn_delay;
            }
        }
        system->emitter_list[system->tail_idx].next = handle;
        system->tail_idx = handle;
        system->emitter_list[handle].next = 0;
        system->emitter_list[handle].playing = true;
    }
    system->emitters[handle].active = true;

}

// An emitter cannot be unloaded or paused mid-way when particles to still
// emitting, so defer into update function to do so
void stop_emitter_handle(ParticleSystem_t* system, EmitterHandle handle)
{
    if (handle == 0) return;
    //if (!system->emitter_list[handle].playing) return;
    
    system->emitters[handle].active = false;
}

void update_emitter_handle_position(ParticleSystem_t* system, EmitterHandle handle, Vector2 pos)
{
    if (handle == 0) return;
    
    system->emitters[handle].position = pos;
}

void unload_emitter_handle(ParticleSystem_t* system, EmitterHandle handle)
{
    if (handle == 0) return;

    system->emitters[handle].active = false;
    system->emitters[handle].finished = true;
}

bool is_emitter_handle_alive(ParticleSystem_t* system, EmitterHandle handle)
{
    if (handle == 0) return false;

    return !system->emitters[handle].finished;
}

EmitterHandle play_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter)
{
    EmitterHandle idx = load_in_particle_emitter(system, in_emitter);
    if (idx == 0) return 0 ;

    play_emitter_handle(system, idx);
    return idx;
}

void update_particle_system(ParticleSystem_t* system)
{
    uint32_t emitter_idx = system->emitter_list[0].next;
    uint32_t prev_idx = 0;

    while (emitter_idx != 0)
    {
        ParticleEmitter_t* emitter = system->emitters + emitter_idx;
        uint32_t inactive_count = 0;

        if (emitter->emitter_update_func != NULL && emitter->active)
        {
            emitter->active = emitter->emitter_update_func(emitter);
        }

        for (uint32_t i = 0; i < emitter->n_particles; ++i)
        {
            // TODO: If a particle is not spawned, run its timer. Spawn on zero

            // Otherwise do the usual check
            if (emitter->particles[i].alive)
            {
                if (emitter->update_func != NULL)
                {
                    emitter->update_func(emitter->particles + i, emitter->user_data);
                }

            }
            // Lifetime update
            if (emitter->particles[i].timer > 0) emitter->particles[i].timer--;
            if (emitter->particles[i].timer == 0)
            {
                if (emitter->particles[i].spawned)
                {
                    emitter->particles[i].alive = false;
                }
                else
                {
                    emitter->particles[i].spawned = true;
                }
            }

            if (!emitter->particles[i].alive)
            {
                if (!emitter->active)
                {
                    inactive_count++;
                }
                else if (emitter->config->one_shot)
                {
                    inactive_count++;
                }
                else if (emitter->particles[i].spawned)
                {
                    // If not one shot, immediately revive the particle
                    spawn_particle(emitter, i); 
                }
            }
        }

        if (inactive_count == emitter->n_particles)
        {
            // Stop playing only if all particles is inactive
            if (!emitter->finished)
            {
                emitter->finished = true;
            }
            system->emitter_list[prev_idx].next = system->emitter_list[emitter_idx].next;
            system->emitter_list[emitter_idx].next = 0;
            system->emitter_list[emitter_idx].playing = false;
            if (system->tail_idx == emitter_idx)
            {
                system->tail_idx = prev_idx;
            }
            
            if (emitter->finished)
            {
                sc_queue_add_last(&system->free_list, emitter_idx);
                emitter_idx = prev_idx;
            }
        }

        prev_idx = emitter_idx;
        emitter_idx = system->emitter_list[emitter_idx].next;
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
                if (emitter->spr == NULL)
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
                    draw_sprite(emitter->spr, 0, part->position, part->rotation, false);
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
