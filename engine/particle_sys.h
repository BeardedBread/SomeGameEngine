#ifndef _PARTICLE_SYSTEM_H
#define _PARTICLE_SYSTEM_H
#include "raylib.h"
#include "engine_conf.h"
#include "sc_queue.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum PartEmitterType
{
    EMITTER_BURST = 0,
} PartEmitterType_t;

typedef struct Particle
{
    Texture2D* tex;
    Vector2 position;
    Vector2 velocity;
    Vector2 accleration;
    float rotation;
    bool alive;
    uint32_t timer;
}Particle_t;

typedef struct EmitterConfig
{
    Vector2 launch_range;
    Vector2 speed_range;
    Vector2 position;
    uint32_t particle_lifetime;
    Vector2 gravity;
    Vector2 friction_coeff;
    PartEmitterType_t type;
}EmitterConfig_t;

typedef struct ParticleEmitter
{
    EmitterConfig_t* config;
    Particle_t particles[MAX_PARTICLES];
    uint32_t n_particles;
    uint32_t timer;
    bool one_shot;
    bool finished;
    bool active;
}ParticleEmitter_t;

typedef struct IndexList
{
    uint32_t next;
}IndexList_t;

typedef struct ParticleSystem
{
    EmitterConfig_t emitter_configs[MAX_PARTICLE_CONF];
    ParticleEmitter_t emitters[MAX_PARTICLE_EMITTER];
    IndexList_t emitter_list[MAX_PARTICLE_EMITTER + 1];
    struct sc_queue_64 free_list;
    uint32_t tail_idx;
    uint32_t n_configs;
    uint32_t n_emitters;
}ParticleSystem_t;

void init_particle_system(ParticleSystem_t* system);
void add_particle_emitter(ParticleSystem_t* system, ParticleEmitter_t* emitter);
void update_particle_system(ParticleSystem_t* system);
void draw_particle_system(ParticleSystem_t* system);
void deinit_particle_system(ParticleSystem_t* system);
#endif // _PARTICLE_SYSTEM_H
