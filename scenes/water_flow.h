#ifndef __WATER_FLOW_H
#define __WATER_FLOW_H
#include "scene_impl.h"
#include "ent_impl.h"
Entity_t* create_water_runner(EntityManager_t* ent_manager, int32_t width, int32_t height, int32_t start_tile);
void free_water_runner(Entity_t** ent, EntityManager_t* ent_manager);

void update_water_runner_system(Scene_t* scene);
#endif // __WATER_FLOW_H

