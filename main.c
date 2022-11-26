#include "mempool.h"
#include "entManager.h"
#include <stdio.h>

int main(void)
{
    init_memory_pools();

    puts("Init-ing manager and memory pool");
    EntityManager_t manager;
    init_entity_manager(&manager);

    puts("Creating two entities");
    Entity_t *p_ent = add_entity(&manager, "player");
    CBBox_t * p_bbox = (CBBox_t *)add_component(&manager, p_ent, CBBOX_COMP_T);
    p_bbox->x = 15;
    p_ent = add_entity(&manager, "enemy");
    p_bbox = (CBBox_t *)add_component(&manager, p_ent, CBBOX_COMP_T);
    p_bbox->x = 40;
    update_entity_manager(&manager);

    puts("Print and remove the entities");
    unsigned long idx = 0;
    sc_map_foreach(&manager.entities, idx, p_ent)
    {
        p_bbox = (CBBox_t *)get_component(&manager, p_ent, CBBOX_COMP_T);
        printf("BBOX x: %d\n", p_bbox->x);
        remove_entity(&manager, idx);
    }
    puts("");
    update_entity_manager(&manager);

    puts("Print again, should show nothing");
    sc_map_foreach(&manager.entities, idx, p_ent)
    {
        p_bbox = (CBBox_t *)get_component(&manager, p_ent, CBBOX_COMP_T);
        printf("BBOX x: %d\n", p_bbox->x);
        remove_entity(&manager, idx);
    }
    puts("");

    puts("Freeing manager and memory pool");
    free_entity_manager(&manager);

    free_memory_pools();
    return 0;
}
