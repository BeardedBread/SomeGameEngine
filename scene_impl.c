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

void init_level_scene(LevelScene_t *scene)
{
    init_scene(&scene->scene, LEVEL_SCENE, &level_scene_render_func);
    scene->scene.scene_data = &scene->data;
    scene->data.player = NULL;

    // insert level scene systems
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &screen_bounce_system);
}
void free_level_scene(LevelScene_t *scene)
{
    free_scene(&scene->scene);
}

void reload_level_scene(LevelScene_t *scene)
{
}
