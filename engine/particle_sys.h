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
    Vector2 position;
    Vector2 velocity;
    Vector2 accleration;
    float rotation;
    float angular_vel;
    float size;
    uint32_t timer;
    bool alive;
}Particle_t;

typedef void (*particle_update_func_t)(Particle_t* part, void* user_data);

typedef struct EmitterConfig
{
    float launch_range[2];
    float speed_range[2];
    uint32_t particle_lifetime[2];
    PartEmitterType_t type;
    void* user_data;
    particle_update_func_t update_func;
}EmitterConfig_t;

typedef struct ParticleEmitter
{
    EmitterConfig_t config;
    Vector2 position;
    Particle_t particles[MAX_PARTICLES];
    uint32_t n_particles;
    Texture2D* tex;
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
    ParticleEmitter_t emitters[MAX_PARTICLE_EMITTER + 1];
    IndexList_t emitter_list[MAX_PARTICLE_EMITTER + 1];
    struct sc_queue_64 free_list;
    uint32_t tail_idx;
    uint32_t n_configs;
    uint32_t n_emitters;
}ParticleSystem_t;

void init_particle_system(ParticleSystem_t* system);
void add_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter);
void update_particle_system(ParticleSystem_t* system);
void draw_particle_system(ParticleSystem_t* system);
void deinit_particle_system(ParticleSystem_t* system);
#endif // _PARTICLE_SYSTEM_H
