#include "AABB.h"

uint8_t find_1D_overlap(Vector2 l1, Vector2 l2, float* overlap)
{
   // No Overlap
    if (l1.y <= l2.x || l2.y <= l1.x) return 0;

    if (
        (l1.x >= l2.x && l1.y <= l2.y)
        || (l2.x >= l1.x && l2.y <= l1.y)
    )
    {
        // Complete Overlap, any direction is possible
        // Cannot give a singular value, but return something anyways
        //*overlap = l2.y-l2.x + l1.y-l1.x;
        *overlap = fmin(fabs(l2.y-l2.x), fabs(l1.y-l1.x));
        return 2;
    }
    //Partial overlap
    // x is p1, y is p2
    *overlap =  (l2.y > l1.y)? l2.x - l1.y : l2.y - l1.x;
    return 1;
}

uint8_t find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2* overlap)
{
    // Note that we include one extra pixel for checking
    // This avoid overlapping on the border
    Vector2 l1, l2;
    Vector2 tmp = {0,0};
    uint8_t overlap_x, overlap_y;
    l1.x = tl1.x;
    l1.y = tl1.x + sz1.x;
    l2.x = tl2.x;
    l2.y = tl2.x + sz2.x;

    overlap_x = find_1D_overlap(l1, l2, &tmp.x);
    l1.x = tl1.y;
    l1.y = tl1.y + sz1.y;
    l2.x = tl2.y;
    l2.y = tl2.y + sz2.y;
    overlap_y = find_1D_overlap(l1, l2, &tmp.y);

    if (overlap) *overlap = tmp;
    if (overlap_x == 2 && overlap_y == 2) return 2;
    return (overlap_x < overlap_y) ? overlap_x : overlap_y;
}

bool point_in_AABB(Vector2 point, Rectangle box)
{
    return (
        point.x >= box.x
        && point.y >= box.y
        && point.x < box.x + box.width
        && point.y < box.y + box.height
    );
}

bool line_in_AABB(Vector2 p1, Vector2 p2, Rectangle box)
{
    float A = p2.y - p1.y;
    float B = p1.x - p2.x;
    float C = (p2.x * p1.y) - (p1.x * p2.y);

    Vector2 corners[3] = 
    {
        {box.x + box.width, box.y},
        {box.x + box.width, box.y + box.height},
        {box.x, box.y + box.height},
    };

    float F = (A * box.x + B * box.y + C);

    bool collide = false;
    uint8_t last_mode = 0;
    if (fabs(F) < 1e-3)
    {
        last_mode = 0;
    }
    else
    {
        last_mode = (F > 0) ? 1 : 2;
    }
    for (uint8_t i = 0; i < 3; ++i)
    {

        F = (A * corners[i].x + B * corners[i].y + C);
        uint8_t mode = 0;
        if (fabs(F) < 1e-3)
        {
            mode = 0;
        }
        else
        {
            mode = (F > 0) ? 1 : 2;
        }
        if (mode != last_mode)
        {
            collide = true;
            break;
        }
        last_mode = mode;
    }
    if (!collide) return false;

    //Projection check
    Vector2 overlap = {0};
    Vector2 l1, l2;
    l1.x = p1.x;
    l1.y = p2.x;
    l2.x = box.x;
    l2.y = box.x + box.width;
    uint8_t x_res = find_1D_overlap(l1, l2, &overlap.x);

    l1.x = p1.y;
    l1.y = p2.y;
    l2.x = box.y;
    l2.y = box.y + box.height;
    uint8_t y_res = find_1D_overlap(l1, l2, &overlap.y);

    return (x_res != 0 && y_res != 0);
}
