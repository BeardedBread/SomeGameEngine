#ifndef _ENGINE_CONF_H
#define _ENGINE_CONF_H

// Take care tuning these params. Web build doesn't work
// if memory used too high
#define MAX_SCENES_TO_RENDER 8
#define MAX_RENDER_LAYERS 4
#define MAX_RENDERMANAGER_DEPTH 4
#define MAX_ENTITIES 2047
#define MAX_TEXTURES 16
#define MAX_SPRITES 127
#define MAX_SOUNDS 32
#define MAX_FONTS 4
#define MAX_N_TILES 16384
#define MAX_NAME_LEN 32
#define MAX_LEVEL_PACK 4
#define N_SFX 32
#define MAX_EMITTER_CONF 8
//#define MAX_PARTICLE_EMITTER 8
#define MAX_ACTIVE_PARTICLE_EMITTER 255
#define MAX_PARTICLES 32

#define MAX_TILE_TYPES 16
#define N_TAGS 10
#define N_COMPONENTS 20
#define MAX_COMP_POOL_SIZE MAX_ENTITIES
#endif // _ENGINE_CONF_H
