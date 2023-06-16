#include "AABB.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_no_overlap(void **state)
{
    (void) state;
    Vector2 a = {0, 5};
    Vector2 b = {6, 9};
    float overlap = 0;
    assert_int_equal(find_1D_overlap(a, b, &overlap), 0);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_no_overlap),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
