#ifndef __AABB_H
#define __AABB_H
#include "raylib.h"
#include "raymath.h"
bool find_1D_overlap(Vector2 l1, Vector2 l2, float* overlap);
bool find_AABB_overlap(const Vector2 tl1, const Vector2 sz1, const Vector2 tl2, const Vector2 sz2, Vector2* overlap);
#endif // __AABB_H
