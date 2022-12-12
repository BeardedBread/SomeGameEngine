#include "scene_impl.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

static void level_scene_render_func(Scene_t* scene)
{
    Entity_t *p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CTransform_t* p_ct = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        DrawRectangle(p_ct->position.x, p_ct->position.y, p_bbox->size.x, p_bbox->size.y, RED);
    }
    return;
}

static void movement_update_system(Scene_t* scene)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    float mag = Vector2Length(data->player_dir);
    mag = (mag == 0)? 1 : mag;
    Entity_t * p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_player, CTRANSFORM_COMP_T);
        p_ctransform->accel.x = data->player_dir.x * 200 * 1.0 / mag;
        p_ctransform->accel.y = data->player_dir.y * 200 * 1.0 / mag;
    }
    data->player_dir.x = 0;
    data->player_dir.y = 0;

    float delta_time = GetFrameTime();
    CTransform_t * p_ctransform;
    sc_map_foreach_value(&scene->ent_manager.component_map[CTRANSFORM_COMP_T], p_ctransform)
    {
        p_ctransform->velocity = Vector2Add(
            p_ctransform->velocity,
            Vector2Scale(p_ctransform->accel, delta_time)
        );
        p_ctransform->position = Vector2Add(
            p_ctransform->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );
    }
}

static void screen_bounce_system(Scene_t *scene)
{
    Entity_t* p_ent;
    sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
    {
        CBBox_t* p_bbox = get_component(&scene->ent_manager, p_ent, CBBOX_COMP_T);
        CTransform_t* p_ctransform = get_component(&scene->ent_manager, p_ent, CTRANSFORM_COMP_T);
        if (p_bbox == NULL || p_ctransform == NULL) continue;

        if(p_ctransform->position.x < 0 || p_ctransform->position.x + p_bbox->size.x > 320)
        {
            p_ctransform->position.x = (p_ctransform->position.x < 0) ? 0 : p_ctransform->position.x;
            p_ctransform->position.x = (p_ctransform->position.x + p_bbox->size.x > 320) ? 320 - p_bbox->size.x : p_ctransform->position.x;
            p_ctransform->velocity.x *= -1;
        }
        
        if(p_ctransform->position.y < 0 || p_ctransform->position.y + p_bbox->size.y > 240)
        {
            p_ctransform->position.y = (p_ctransform->position.y < 0) ? 0 : p_ctransform->position.y;
            p_ctransform->position.y = (p_ctransform->position.y + p_bbox->size.y > 240) ? 240 - p_bbox->size.y : p_ctransform->position.y;
            p_ctransform->velocity.y *= -1;
        }
    }

}

void level_do_action(Scene_t *scene, ActionType_t action, bool pressed)
{
    LevelSceneData_t *data = (LevelSceneData_t *)scene->scene_data;
    if (pressed)
    {
        switch(action)
        {
            case ACTION_UP:
                data->player_dir.y = -1;
            break;
            case ACTION_DOWN:
                data->player_dir.y = 1;
            break;
            case ACTION_LEFT:
                data->player_dir.x = -1;
            break;
            case ACTION_RIGHT:
                data->player_dir.x = 1;
            break;
        }
    }

}

void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func, &level_do_action);
    scene->scene.scene_data = &scene->data;
    memset(&scene->data.player_dir, 0, sizeof(Vector2));

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &screen_bounce_system);

    sc_map_put_64(&scene->scene.action_map, KEY_UP, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_DOWN, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_LEFT, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_RIGHT, ACTION_RIGHT);
}

void free_level_scene(LevelScene_t *scene)
{
    free_scene(&scene->scene);
}

void reload_level_scene(LevelScene_t *scene)
{
}
