#ifndef __COMPONENTS_H
#define __COMPONENTS_H

// TODO: Look at sc to use macros to auto generate functions

#define N_COMPONENTS 1
enum ComponentEnum
{
    CBBOX_COMP_T,
};
typedef enum ComponentEnum ComponentEnum_t;

typedef struct _CBBox_t
{
    int x;
    int y;
    int width;
    int height;
}CBBox_t;
#endif // __COMPONENTS_H
