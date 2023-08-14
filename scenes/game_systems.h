#ifndef __GAME_SYSTEMS_H
#define __GAME_SYSTEMS_H
#include "scene_impl.h"

void player_movement_input_system(Scene_t* scene);
void player_bbox_update_system(Scene_t* scene);
void player_pushing_system(Scene_t* scene);
void player_crushing_system(Scene_t* scene);
void tile_collision_system(Scene_t* scene);
void friction_coefficient_update_system(Scene_t* scene);
void moveable_update_system(Scene_t* scene);
void global_external_forces_system(Scene_t* scene);
void movement_update_system(Scene_t* scene);
void player_ground_air_transition_system(Scene_t* scene);
void state_transition_update_system(Scene_t* scene);
void update_tilemap_system(Scene_t* scene);
void hitbox_update_system(Scene_t* scene);
void sprite_animation_system(Scene_t* scene);
void boulder_destroy_wooden_tile_system(Scene_t* scene);
void camera_update_system(Scene_t* scene);
void container_destroy_system(Scene_t* scene);
void player_dir_reset_system(Scene_t* scene);
void player_respawn_system(Scene_t* scene);
void lifetimer_update_system(Scene_t* scene);
void spike_collision_system(Scene_t* scene);

#endif // __GAME_SYSTEMS_H
