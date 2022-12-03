#ifndef __ASSETS_H
#define __ASSETS_H
#include "sc/map/sc_map.h"
#include "raylib.h"

typedef struct Animation
{
    Image* sprite;
    int frame_count;
    int current_frame;
    int speed;
    Vector2 size;
    char* name;
}Animation_t;

typedef struct Assets
{
}Assets_t;
#endif // __ASSETS_H

void add_texture(Assets_t *assets, char *name, char *path);
void add_animation(Assets_t *assets, char *name, char *path);
void add_sound(Assets_t *assets, char *name, char *path);
void add_font(Assets_t *assets, char *name, char *path);

Image* get_texture(Assets_t *assets);
Animation_t* get_animation(Assets_t *assets);
Sound* get_sound(Assets_t *assets);
Font* get_font(Assets_t *assets);
