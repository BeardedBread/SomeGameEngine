#ifndef _PARTICLE_SYSTEM_H
#define _PARTICLE_SYSTEM_H
#include "raylib.h"
#include "engine_conf.h"
#include "sc_queue.h"
#include "assets.h"
#include <stdint.h>
#include <stdbool.h>

typedef uint16_t EmitterHandle;

typedef struct Particle
{
    Vector2 position;
    Vector2 velocity;
    Vector2 accleration;
    float rotation;
    float angular_vel;
    float size;
    float timer;
    bool alive;
    bool spawned;
}Particle_t;

typedef struct ParticleEmitter ParticleEmitter_t;

typedef void (*particle_update_func_t)(Particle_t* part, void* user_data, float delta_time);
typedef bool (*emitter_check_func_t)(const ParticleEmitter_t* emitter, float delta_time);

struct ParticleEmitter
{
    const EmitterConfig_t* config;
    Sprite_t* spr;
    Vector2 position;
    Particle_t particles[MAX_PARTICLES];
    uint32_t n_particles;
    float timer;
    bool finished;
    bool active;
    void* user_data;
    particle_update_func_t update_func;
    emitter_check_func_t emitter_update_func;
};

typedef struct IndexList
{
    uint32_t next;
    bool playing;
}IndexList_t;

typedef struct ParticleSystem
{
    ParticleEmitter_t emitters[MAX_ACTIVE_PARTICLE_EMITTER + 1];
    IndexList_t emitter_list[MAX_ACTIVE_PARTICLE_EMITTER + 1];
    struct sc_queue_64 free_list;
    uint32_t tail_idx;
    uint32_t n_configs;
    uint32_t n_emitters;
}ParticleSystem_t;

void init_particle_system(ParticleSystem_t* system);
uint16_t get_number_of_free_emitter(ParticleSystem_t* system);

// For one-shots
EmitterHandle play_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter);

EmitterHandle load_in_particle_emitter(ParticleSystem_t* system, const ParticleEmitter_t* in_emitter);
void play_emitter_handle(ParticleSystem_t* system, EmitterHandle handle);
void stop_emitter_handle(ParticleSystem_t* system, EmitterHandle handle);
void update_emitter_handle_position(ParticleSystem_t* system, EmitterHandle handle, Vector2 pos);
void unload_emitter_handle(ParticleSystem_t* system, EmitterHandle handle);
bool is_emitter_handle_alive(ParticleSystem_t* system, EmitterHandle handle);

void update_particle_system(ParticleSystem_t* system, float delta_time);
void draw_particle_system(ParticleSystem_t* system);
void deinit_particle_system(ParticleSystem_t* system);
#endif // _PARTICLE_SYSTEM_H
