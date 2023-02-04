#ifndef __GAME_SYSTEMS_H
#define __GAME_SYSTEMS_H
#include "scene_impl.h"
void init_level_scene_data(LevelSceneData_t *data);
void term_level_scene_data(LevelSceneData_t *data);

void player_movement_input_system(Scene_t* scene);
void player_bbox_update_system(Scene_t *scene);
void tile_collision_system(Scene_t *scene);
void global_external_forces_system(Scene_t *scene);
void movement_update_system(Scene_t* scene);
void player_ground_air_transition_system(Scene_t* scene);
void state_transition_update_system(Scene_t *scene);
void update_tilemap_system(Scene_t *scene);
#endif // __GAME_SYSTEMS_H
