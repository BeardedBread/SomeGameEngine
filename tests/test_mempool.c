#include "mempool.h"
#include <stdio.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

static int setup_mempool(void** state)
{
    (void)state;

    init_memory_pools();
    return 0;
}

static int teardown_mempool(void** state)
{
    (void)state;

    free_memory_pools();
    return 0;
}

static void test_simple_get_and_free(void **state)
{
    (void)state;

    unsigned long idx;
    Entity_t* ent = new_entity_from_mempool(&idx);

    Entity_t* q_ent = get_entity_wtih_id(idx);

    assert_memory_equal(ent, q_ent, sizeof(Entity_t));

    free_entity_to_mempool(idx);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_simple_get_and_free, setup_mempool, teardown_mempool),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
