#pragma once
#include <mgdl/ccVector/ccVector.h>
#include <mgdl/mgdl-types.h>

extern const vec3 WORLD_UP;
extern const vec3 WORLD_RIGHT;
extern const vec3 WORLD_FORWARD;

struct RenderSettingsOpenGL;

/**
 * @brief Returns the Y of the cross product of 2 2D XZ vectors. used for line point check
 */
float Vec2XZCrossToY(vec2 a, vec2 b);

/**
 * @brief Returns the cross product xy of a x WORLD_UP
 */
vec2 Vec2XZCrossWithY(vec2 a);

vec2 Vec2Project(vec2 move, vec2 wall);

vec2 Vec2XZRotateY(vec2 p, float angle);

/**
 * @brief Converts duke angle to radians
 * @param angleInt Angle between [0,2047]
 */
float Math_DukeAngleToRad(s16 angleInt);

vec3 Vec3DukePosToOpenGL(vec3 dukepos, RenderSettingsOpenGL* settings3D);
vec2 Vec2DukePosToOpenGL(vec2 dukepos, RenderSettingsOpenGL* settings3D);
