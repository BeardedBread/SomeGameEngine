#include "AABB.h"

bool find_1D_overlap(Vector2 l1, Vector2 l2, float* overlap)
{
   // No Overlap
    if (l1.y < l2.x || l2.y < l1.x) return false;

    if (l1.x >= l2.x && l1.y <= l2.y)
    {
        // Complete Overlap, not sure what to do tbh
        //*overlap = l2.y-l2.x + l1.y-l1.x;
        float l1_mag = l1.y - l2.x;
        float l2_mag = l2.y - l2.x;
        float c1 = l2.y - l1.y;
        float c2 = l1.x - l2.x;
        float len_to_add = l1_mag;
        if (l1_mag > l2_mag)
        {
            len_to_add = l2_mag;
            c1 *= -1;
            c2 *= -1;
        }

        if (c1 < c2)
        {
            *overlap = -len_to_add - c1;
        }
        else
        {
            *overlap = len_to_add + c2;
        }
    }
    else
    {
        //Partial overlap
        // x is p1, y is p2
        *overlap =  (l2.y >= l1.y)? l2.x - l1.y : l2.y - l1.x;
    }
    //if (fabs(*overlap) < 0.01) // Use 2 dp precision
    //{
    //    *overlap = 0;
    //    return false;
    //}
    return true;
}

bool find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2* overlap)
{
    // Note that we include one extra pixel for checking
    // This avoid overlapping on the border
    Vector2 l1, l2;
    bool overlap_x, overlap_y;
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

    return overlap_x && overlap_y;
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
