#include "particle_sys.h"

#include <string.h>

void init_particle_system(ParticleSystem_t* system)
{
    memset(system, 0, sizeof(ParticleSystem_t));
    sc_queue_init(&system->free_list);
    for ( uint32_t i = 0; i < MAX_PARTICLE_EMITTER; ++i)
    {
        sc_queue_add_last(&system->free_list, i);
    }
}
void add_particle_emitter(ParticleSystem_t* system, ParticleEmitter_t* emitter)
{
    if (sc_queue_empty(&system->free_list)) return;
    uint32_t idx = sc_queue_del_first(&system->free_list);
    system->emitter_list[system->tail_idx].next = idx;
    system->tail_idx = idx;
    system->emitter_list[idx].next = 0;
    system->emitters[idx] = *emitter;
}
void update_particle_system(ParticleSystem_t* system)
{
}
void draw_particle_system(ParticleSystem_t* system)
{
}
void deinit_particle_system(ParticleSystem_t* system)
{
    sc_queue_term(&system->free_list);
}
