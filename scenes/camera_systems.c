#include "game_systems.h"
#include "ent_impl.h"
#include "raymath.h"

void camera_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &CONTAINER_OF(scene, LevelScene_t, scene)->data;
    Entity_t* p_player;
    const int width = data->game_rec.width;
    const int height =data->game_rec.height;
    data->camera.cam.offset = (Vector2){ width/2.0f, height/2.0f };

    if (data->camera.mode == CAMERA_FOLLOW_PLAYER)
    {
        Vector2 target_vel = {0};
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
        {
            CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_COMP_T);
            data->camera.target_pos.x = p_player->position.x;
            target_vel = p_ctransform->velocity;
            CMovementState_t* p_movement = get_component(p_player, CMOVEMENTSTATE_T);
            CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);
            if (data->camera.offset_dir != p_movement->x_dir) {
                data->camera.delay += scene->delta_time;
                if (data->camera.delay >= 0.25f) {
                    data->camera.delay = 0.0f;
                    data->camera.offset_dir = p_movement->x_dir;
                }
            }
            else 
            {
                data->camera.delay = 0.0f;
            }
            data->camera.target_pos.x += width * 1.0f * data->camera.offset_dir / 6;

            if (p_movement->ground_state == 0b01
                || (p_movement->water_state & 1)
                || (p_pstate->ladder_state & 1)
            )
            {
                data->camera.base_y = p_player->position.y;
            }
            if (p_player->position.y >= data->camera.base_y)
            {
                data->camera.target_pos.y = p_player->position.y;
                data->camera.target_pos.y += 
                    (p_ctransform->velocity.y > 0) ? p_ctransform->velocity.y * 0.2 : 0;
            }
        }
        data->camera.target_pos.x = Clamp(data->camera.target_pos.x, data->game_rec.width / 2,
               fmax(data->tilemap.width * data->tilemap.tile_size, data->game_rec.width) - data->game_rec.width / 2);
        data->camera.target_pos.y = Clamp(data->camera.target_pos.y, data->game_rec.height / 2,
               fmax(data->tilemap.height * data->tilemap.tile_size, data->game_rec.width) - data->game_rec.height / 2);
        
        // Mass-Spring damper update in x direction
        float x = data->camera.target_pos.x - data->camera.cam.target.x;
        float v = data->camera.current_vel.x - target_vel.x; 
        float F = data->camera.k * x - data->camera.c * v;
        // Kinematics update
        const float dt = scene->delta_time;
        float a_dt = F * dt / data->camera.mass;
        data->camera.cam.target.x += (data->camera.current_vel.x + a_dt * 0.5) * dt;

        data->camera.current_vel.x += a_dt;

        // Simple lerp for y direction
        float dy = (data->camera.target_pos.y - data->camera.cam.target.y);
        data->camera.cam.target.y += dy * 0.1f;


    }
    else
    {
        //sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
        //{
        //    break;
        //}
    }

    // Bound camera within level limits
    Vector2 max = GetWorldToScreen2D(
        (Vector2){
            fmax(data->tilemap.width * data->tilemap.tile_size, data->game_rec.width),
            fmax(data->tilemap.height * data->tilemap.tile_size, data->game_rec.height)
        },
        data->camera.cam
    );
    Vector2 min = GetWorldToScreen2D((Vector2){0, 0}, data->camera.cam);
    if (max.x < width)
    {
        //data->camera.cam.offset.x = width - (max.x - width/2.0f);
        data->camera.cam.target.x = fmax(data->tilemap.width * data->tilemap.tile_size, data->game_rec.width) - (width >> 1);
        data->camera.current_vel.x = 0;
    }
    if (min.x > 0)
    {
        //data->camera.cam.offset.x = (width >> 1) - min.x;
        data->camera.cam.target.x = (width >> 1);
        data->camera.current_vel.x = 0;
    }

    if (max.y < height)
    {
        //data->camera.cam.offset.y = height - (max.y - height/2.0f);
        data->camera.cam.target.y = fmax(data->tilemap.height * data->tilemap.tile_size, data->game_rec.height) - (height >> 1);
        data->camera.current_vel.y = 0;
    }
    if (min.y > 0)
    {
        //data->camera.cam.offset.y = (height >> 1) - min.y;
        data->camera.cam.target.y = (height >> 1);
        data->camera.current_vel.y = 0;
    }
}
