#ifndef __ENT_IMPL_H
#define __ENT_IMPL_H
#include "assets_tag.h"
#include "components.h"

bool init_player_creation(const char* info_file, Assets_t* assets);
bool init_player_creation_rres(const char* rres_file, const char* file, Assets_t* assets);
Entity_t* create_player(EntityManager_t* ent_manager);
Entity_t* create_dead_player(EntityManager_t* ent_manager);
Entity_t* create_player_finish(EntityManager_t* ent_manager);


bool init_item_creation(Assets_t* assets);
Entity_t* create_crate(EntityManager_t* ent_manager, bool metal, ContainerItem_t item);
Entity_t* create_boulder(EntityManager_t* ent_manager);
Entity_t* create_arrow(EntityManager_t* ent_manager, uint8_t dir);
Entity_t* create_bomb(EntityManager_t* ent_manager, Vector2 launch_dir);
Entity_t* create_explosion(EntityManager_t* ent_manager);
Entity_t* create_urchin(EntityManager_t* ent_manager);
Entity_t* create_chest(EntityManager_t* ent_manager);
Entity_t* create_level_end(EntityManager_t* ent_manager);

#endif // __ENT_IMPL_H
