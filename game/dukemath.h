#pragma once
#include <mgdl/ccVector/ccVector.h>

/**
 * @brief Returns the Z of the cross product of 2 2D vectors. used for line point check
 */
float Vec2CrossToZ(vec2 a, vec2 b);

/**
 * @brief Returns the cross product xy of a x (0,0,1)
 */
vec2 Vec2CrossWithZ(vec2 a);

vec2 Vec2Project(vec2 move, vec2 wall);
