#ifndef RENDER_QUEUE_H
#define RENDER_QUEUE_H
#include "assets.h"
#include "engine_conf.h"

typedef struct RenderInfoNode RenderInfoNode;
struct RenderInfoNode {
    // Intrusive Linked-list Node
    RenderInfoNode* next;

    Sprite_t* spr;
    Vector2 pos;
    int frame_num;
    float rotation;
    Vector2 scale;
    Color colour;
    uint8_t flip;
};

typedef struct RenderManager {
    RenderInfoNode* layers[MAX_RENDERMANAGER_DEPTH];
} RenderManager;

void init_render_manager(RenderManager* manager);
#define reset_render_manager init_render_manager

bool add_render_node(RenderManager* manager, RenderInfoNode* node, uint8_t layer_num);
void execute_render(RenderManager* manager);
#endif
