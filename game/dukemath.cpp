#include "dukemath.h"
#include <mgdl/mgdl-types.h>

#include "build-render.h"


// RIGHT HANDED COORDINATE SYSTEM
const vec3 WORLD_RIGHT = 	vec3New(1, 0, 0);
const vec3 WORLD_UP = 		vec3New(0,1,0);
const vec3 WORLD_FORWARD = 	vec3New(0, 0, -1);

float Vec2XZCrossToY(vec2 a, vec2 b)
{
	float ax = a.x;
	float az = a.y;
	//float ay = 0.0f;
	float bx = b.x;
	float bz = b.y;
	//float by = 0.0f;

	// The A
	// float x = ay*bz - az*by;
	float y = az*bx - ax*bz;
	// // float z = ax*by - ay*bx;
	return y;
}

vec2 Vec2XZCrossWithY(vec2 a)
{
	float ax = a.x;
	float ay = 0.0f;
	float az = a.y;

	float bx = 0.0f;
	float by = 1.0f;
	float bz = 0.0f;


	float x = ay*bz - az*by;
	// // float y = az*bx - ax*bz;
	float z = ax*by - ay*bx;
	return vec2New( x, z);
}


vec2 Vec2Project(vec2 move, vec2 wall)
{
	return vec2Multiply(wall, vec2DotProduct(move, wall)/vec2DotProduct(wall, wall));
}

vec2 Vec2XZRotateY(vec2 p, float angle)
{
	float sin_a = sin(angle);
	float cos_a = cos(angle);
	float xt = p.x * cos_a + p.y*sin_a;
	float zt = p.x *-sin_a + p.y*cos_a;
    return vec2New(xt, zt);
}
vec3 Vec3XYZRotateY(vec3 p, float angle)
{

	float sin_a = sin(angle);
	float cos_a = cos(angle);
	float xt = p.x * cos_a + p.z*sin_a;
	float yt = p.y;
	float zt = p.x *-sin_a + p.z*cos_a;
    return vec3New(xt, yt, zt);
}

/*
 * Duke angles are [0 , 2047]
 * Rotating the world forward should give
 * DA       Direction
 * 0     : ( 0, 0, 1)
 * 512   : ( 1, 0, 0)
 * 1024  : ( 0, 0,-1)
 * 1536  : (-1, 0, 0)
 *
 * World forward is : 0, 0, -1
 * Rotating that by 0 radians gives the same, so we need to subtract PI from
 * result
 * Positive angles rotate counter-clockwise
 * 0 dukes = 0 degrees
 * 0 degrees = (0, 0, -1)
 * 0 degrees -90 degrees = (1, 0, 0)
 *
*/
float Math_DukeAngleToRad(s16 angleInt)
{
	// Dukes turn clockwise
	// Radians turn counter-clockwise

	// How many radians to turn
    float ratio = (float)angleInt / (float)2048;
	float radians = -ratio * M_PI * 2.0f;
	// Adjust by PI
    return radians - M_PI;
}

vec3 Vec3DukePosToOpenGL(vec3 dukepos, RenderSettingsOpenGL* settings3D)
{
	dukepos.x = dukepos.x * settings3D->scaleXZ;
	dukepos.y = dukepos.y * settings3D->scaleY;
	dukepos.z = dukepos.z * settings3D->scaleXZ;
	return dukepos;
}
vec2 Vec2DukePosToOpenGL(vec2 dukepos, RenderSettingsOpenGL* settings3D)
{
	dukepos.x *= settings3D->scaleXZ;
	dukepos.y *= settings3D->scaleXZ;
	return dukepos;
}
