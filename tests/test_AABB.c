#include "AABB.h"
#include <stdio.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

static void test_line_AABB(void **state)
{
    (void) state;

    Vector2 p1 = {0, 0};
    Vector2 p2 = {20, 20};

    Rectangle box = {5, 0, 10, 20};
    assert_true(line_in_AABB(p1, p2, box));

    p1.y = 20;
    assert_false(line_in_AABB(p1, p2, box));

    p1.y = 19;
    p2 = (Vector2){19, 19};
    assert_true(line_in_AABB(p1, p2, box));

    p1.y = 0;
    p2.y = 0;
    assert_false(line_in_AABB(p1, p2, box));
    p2.y = 1;
    assert_true(line_in_AABB(p1, p2, box));

    p1 = (Vector2){5, 0};
    p2 = (Vector2){5, 10};
    assert_false(line_in_AABB(p1, p2, box));
    p2 = (Vector2){6, 10};
    assert_true(line_in_AABB(p1, p2, box));

    p1 = (Vector2){14, 0};
    p2 = (Vector2){14, 10};
    assert_true(line_in_AABB(p1, p2, box));

    p1 = (Vector2){15, 0};
    p2 = (Vector2){15, 10};
    assert_false(line_in_AABB(p1, p2, box));

    p1 = (Vector2){0, 30};
    p2 = (Vector2){6, 35};
    assert_false(line_in_AABB(p1, p2, box));
}

static void test_point_AABB(void **state)
{
    (void) state;

    Rectangle box = {0, 0, 20, 20};
    Vector2 p = {-5, -5};
    assert_false(point_in_AABB(p, box));

    p.x = 10;
    p.y = 10;
    assert_true(point_in_AABB(p, box));

    p.x = 0;
    p.y = 0;
    assert_true(point_in_AABB(p, box));

    p.x = 20;
    p.y = 20;
    assert_false(point_in_AABB(p, box));

}

static void test_AABB_overlap(void **state)
{
    (void) state;
    Vector2 p1 = {10, 10};
    Vector2 sz1 = {10, 10};
    Vector2 p2 = {25, 25};
    Vector2 sz2 = {20, 20};

    Vector2 overlap = {0};
    assert_int_equal(find_AABB_overlap(p1, sz1, p2, sz2, &overlap), 0);
    assert_int_equal(find_AABB_overlap(p2, sz2, p1, sz1, &overlap), 0);

    p2.x = 10;
    p2.y = 10;
    assert_int_equal(find_AABB_overlap(p1, sz1, p2, sz2, &overlap), 2); // Smaller one is complete overlap
    assert_int_equal(find_AABB_overlap(p2, sz2, p1, sz1, &overlap), 2); // This is also considered complete overlap

    p2.x = 15;
    p2.y = 15;
    assert_int_equal(find_AABB_overlap(p1, sz1, p2, sz2, &overlap), 1);
    assert_float_equal(overlap.x, -5, 1e-5);
    assert_float_equal(overlap.y, -5, 1e-5);
    assert_int_equal(find_AABB_overlap(p2, sz2, p1, sz1, &overlap), 1);
    assert_float_equal(overlap.x, 5, 1e-5);
    assert_float_equal(overlap.y, 5, 1e-5);
}

static void test_1D_overlap(void **state)
{
    (void) state;
    Vector2 a = {0, 5};
    Vector2 b = {6, 9};
    float overlap = 0;
    assert_int_equal(find_1D_overlap(a, b, &overlap), 0);

    a.y = 6;
    assert_int_equal(find_1D_overlap(a, b, &overlap), 0);
    assert_int_equal(find_1D_overlap(b, a, &overlap), 0);

    a.y = 7;
    assert_int_equal(find_1D_overlap(a, b, &overlap), 1);
    assert_float_equal(overlap, -1, 1e-5);
    assert_int_equal(find_1D_overlap(b, a, &overlap), 1);
    assert_float_equal(overlap, 1, 1e-5);
    
    a.x = 7;
    a.y = 9;
    assert_int_equal(find_1D_overlap(a, b, &overlap), 2);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_1D_overlap),
        cmocka_unit_test(test_AABB_overlap),
        cmocka_unit_test(test_point_AABB),
        cmocka_unit_test(test_line_AABB),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
