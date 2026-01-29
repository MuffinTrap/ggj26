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
	float xt = p.x*cos(angle) - p.y*sin(angle);
	float zt = p.x*sin(angle) + p.y*cos(angle);
    return vec2New(xt, zt);
}
vec3 Vec3XYZRotateY(vec3 p, float angle)
{
	float xt = p.x*cos(angle) - p.z*sin(angle);
	float zt = p.x*sin(angle) + p.z*cos(angle);
    return vec3New(xt, p.y, zt);
}


float Math_DukeAngleToRad(s16 angleInt)
{
    float ratio = (float)angleInt / (float)2048;
    return ratio * M_PI * 2.0f;
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
