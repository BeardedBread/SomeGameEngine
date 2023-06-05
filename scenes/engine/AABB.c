#include "AABB.h"

uint8_t find_1D_overlap(Vector2 l1, Vector2 l2, float* overlap)
{
   // No Overlap
    if (l1.y < l2.x || l2.y < l1.x) return 0;

    if (l1.x >= l2.x && l1.y <= l2.y)
    {
        // Complete Overlap, any direction is possible
        // Cannot give a singular value, but return something anyways
        *overlap = l2.y-l2.x + l1.y-l1.x;
        return 2;
    }
    //Partial overlap
    // x is p1, y is p2
    *overlap =  (l2.y >= l1.y)? l2.x - l1.y : l2.y - l1.x;
    return 1;
}

uint8_t find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2* overlap)
{
    // Note that we include one extra pixel for checking
    // This avoid overlapping on the border
    Vector2 l1, l2;
    uint8_t overlap_x, overlap_y;
    l1.x = tl1.x;
    l1.y = tl1.x + sz1.x;
    l2.x = tl2.x;
    l2.y = tl2.x + sz2.x;

    overlap_x = find_1D_overlap(l1, l2, &overlap->x);
    l1.x = tl1.y;
    l1.y = tl1.y + sz1.y;
    l2.x = tl2.y;
    l2.y = tl2.y + sz2.y;
    overlap_y = find_1D_overlap(l1, l2, &overlap->y);

    if (overlap_x == 2 && overlap_y == 2) return 2;
    return (overlap_x < overlap_y) ? overlap_x : overlap_y;
}

bool point_in_AABB(Vector2 point, Rectangle box)
{
    return (
        point.x > box.x
        && point.y > box.y
        && point.x < box.x + box.width
        && point.y < box.y + box.height
    );
}
