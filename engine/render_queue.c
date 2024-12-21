#include "render_queue.h"
#include "raylib.h"

void init_render_manager(RenderManager *manager)
{
    memset(manager, 0, sizeof(*manager));
}

bool add_render_node(RenderManager *manager, RenderInfoNode *node, uint8_t layer_num)
{
    if (node->next != NULL)
    {
        return false;
    }
    if (node->spr == NULL) {
        return false;
    }
    layer_num = (layer_num >= MAX_RENDERMANAGER_DEPTH) ? MAX_RENDERMANAGER_DEPTH - 1 : layer_num;

    node->next = manager->layers[layer_num];
    manager->layers[layer_num] = node;
    return true;
}

void execute_render(RenderManager *manager) {
    for (uint8_t depth = 0; depth < MAX_RENDERMANAGER_DEPTH; ++depth)
    {
        RenderInfoNode* curr = manager->layers[depth];
        while (curr != NULL)
        {
            draw_sprite_pro(
                curr->spr, curr->frame_num, curr->pos,
                curr->rotation, curr->flip, curr->scale,
                curr->colour
            );
            RenderInfoNode* next = curr->next;
            curr->next = NULL;
            curr = next;
        }
    }
    reset_render_manager(manager);
}
