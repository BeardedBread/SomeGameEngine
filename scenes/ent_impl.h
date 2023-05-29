#ifndef __ENT_IMPL_H
#define __ENT_IMPL_H
#include "assets.h"

typedef enum EntityTag {
    NO_ENT_TAG = 0,
    PLAYER_ENT_TAG,
    ENEMY_ENT_TAG,
    CRATES_ENT_TAG,
    BOULDER_ENT_TAG,
} EntityTag_t;


bool init_player_creation(const char* info_file, Assets_t* assets);
Entity_t* create_player(EntityManager_t* ent_manager, Assets_t* assets);
Entity_t* create_crate(EntityManager_t* ent_manager, Assets_t* assets, bool metal);
Entity_t* create_boulder(EntityManager_t* ent_manager, Assets_t* assets);

#endif // __ENT_IMPL_H
