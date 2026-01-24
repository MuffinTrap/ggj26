#include "dukemath.h"


float Vec2CrossToZ(vec2 a, vec2 b)
{
	// The A
	return a.x*b.y - a.y*b.x;
}

vec2 Vec2CrossWithZ(vec2 a)
{
	return vec2New( -a.y, a.x);
}
