#ifndef __ENTITY_H
#define __ENTITY_H
#include <stdbool.h>
#include "sc/map/sc_map.h"
#define MAX_TAG_LEN 32
typedef struct Entity
{
    unsigned long m_id;
    char m_tag[MAX_TAG_LEN];
    bool m_alive;
    struct sc_map_64 components;
}Entity_t;
#endif // __ENTITY_H
