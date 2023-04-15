#ifndef __ENTITY_H
#define __ENTITY_H
#include <stdbool.h>
#include "sc/map/sc_map.h"

#define N_TAGS 4
typedef enum EntityTag {
    NO_ENT_TAG,
    PLAYER_ENT_TAG,
    ENEMY_ENT_TAG,
    CRATES_ENT_TAG,
} EntityTag_t;

typedef struct Entity {
    unsigned long m_id;
    EntityTag_t m_tag;
    bool m_alive;
    struct sc_map_64 components;
} Entity_t;
#endif // __ENTITY_H
