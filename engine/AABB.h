#ifndef __AABB_H
#define __AABB_H
#include "raylib.h"
#include "raymath.h"
#include <stdint.h>
uint8_t find_1D_overlap(Vector2 l1, Vector2 l2, float* overlap);
uint8_t find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2* overlap);
bool point_in_AABB(Vector2 point, Rectangle box);
bool line_in_AABB(Vector2 p1, Vector2 p2, Rectangle box);
#endif // __AABB_H
