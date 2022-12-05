#ifndef __COMPONENTS_H
#define __COMPONENTS_H
#include "raylib.h"
// TODO: Look at sc to use macros to auto generate functions

#define N_COMPONENTS 2
enum ComponentEnum
{
    CBBOX_COMP_T,
    CTRANSFORM_COMP_T,
};
typedef enum ComponentEnum ComponentEnum_t;

typedef struct _CBBox_t
{
    Vector2 size;
}CBBox_t;

typedef struct _CTransform_t
{
    Vector2 position;
    Vector2 velocity;
    Vector2 accel;
}CTransform_t;
#endif // __COMPONENTS_H
