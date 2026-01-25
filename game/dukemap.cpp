#include "dukemap.h"
#include <mgdl.h>
#include <mgdl/mgdl-vectorfunctions.h>
#include <mgdl/ccVector/ccVector.h>
#include "dukemath.h"
#include "player.h"
Sector* Map_GetSector(DukeMap* map, int sectorNumber)
{
    mgdl_assert_print((sectorNumber>= 0 && sectorNumber < map->sectorAmount),"Invalid sector for Map_GetSector");
    return &map->sectors[sectorNumber];
}
Wall* Map_GetWallInSector(DukeMap* map, s16 sector, s16 wi)
{
    Sector* s = &map->sectors[sector];
    wi += s->wallptr;
    mgdl_assert_print((wi>= 0 && wi < map->wallAmount),"Invalid wall index for Sector_GetWall");
    Wall* w = &map->walls[wi];
    Wall* w2 = &map->walls[w->point2];
    w->end = vec2New(w2->x, w2->y);
    return w;
}

void Map_InitPlayer(DukeMap* map, Player* player)
{
    player->positionOpenGL = map->startPosition;
    player->angleRad = Map_AngleToRad(map->startAngle - 512);
    player->sectorNumber = map->startingSector;
}

void Map_PrintInfo(DukeMap* map)
{
    Log_InfoF("Duke Map Version:%d Start pos:(%.2f,%.2f,%.2f), Start angle:%d Start Sector:%d\n",
              map->version,
              map->startPosition.x,
              map->startPosition.y,
              map->startPosition.z,
              map->startAngle,
              map->startingSector);
    Log_InfoF("Duke Map Sectors:%d Walls:%d, Sprites:%d\n", map->sectorAmount, map->wallAmount, map->spriteAmount);
    for (int i = 0; i < map->sectorAmount; i++)
    {
        Sector* s = &map->sectors[i];
        Log_InfoF("Sector n: %d Walls: %d first wall %d FloorZ %d CeilingZ %d\n", i, s->wallnum, s->wallptr, s->floorz, s->ceilingz);
    }

    for (int s = 0; s < map->wallAmount; s++)
    {
        Wall* w = &map->walls[s];
        Wall* w2 = &map->walls[w->point2];

        Log_InfoF("Wall n: %d (%d,%d) - (%d,%d)\n", s, w->x, w->y, w2->x, w2->y);
    }


    for (int i = 0; i < map->spriteAmount; i++)
    {

    }
}

float Map_AngleToRad(s16 angleInt)
{
    float ratio = (float)angleInt / (float)2048;
    return ratio * M_PI * 2.0f;
}

// Are we inside a sector
// NOTE FROM DUKE SOURCE CODE
// returns 1 when inside
bool Map_IsPointInsideSectorOG(DukeMap* map, vec2 point, int sectorNumber)
{
        Sector* sector = Map_GetSector(map, sectorNumber);
        u32 count = 0;

        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, sectorNumber, wi);
            vec2 start = w->start;
            vec2 end =  w->end;

            // Check if these are different signs
            s32 testY1 = (s32)start.y - (s32)point.y;
            s32 testY2 = (s32)end.y - (s32)point.y;
            if ((testY1^testY2) < 0)
            {
                // Different signs, point.y is between

                // Test if the whole line is on the right: both are positive
                // or negative
                s32 testX1 = (s32)start.x - (s32)point.x;
                s32 testX2 = (s32)end.x - (s32)point.x;
                if ((testX1^testX2) >= 0)
                {
                    // Both are on the right side: both are positive
                    // Or both are on left side : both are negative
                    // Toggle sign:
                    // 0 ^ 1 -> 1
                    // 1 ^ 1 -> 0
                    // 1 ^ 0 -> 1
                    // 0 ^ 0 -> 1
                    // Finds left: 0^1=1 then right: 1^0 = 1 : inside
                    // Finds left: 0^1=1 then left 1^1 = 0 : not inside
                    // Finds right: 0^0=0 then left 0^1 = 1 : inside
                    count ^= testX1;
                }
                else
                {
                    // Other x is left, other is right
                    // Do point on side of line test with cross product
                    // If on the right
                    //        this is negative when on right 1
                    //        y2 is positive if it was below 0 : so this is  1^0 = 1 : left
                    //        y2 is negative if it was above 1 : this is 1^1 = 0 : right
                    count ^= (testX1*testY2 - testX2*testY1)^testY2;
                }
            }
		}
		return  (count >> 31) > 0;
}
// Are we inside a sector
// NOTE FROM DUKE SOURCE CODE
bool Map_IsPointInsideSectorRay(DukeMap* map, vec2 point, int sectorNumber)
{
        Sector* sector = Map_GetSector(map, sectorNumber);
        u32 count = 0;

        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, sectorNumber, wi);
            vec2 start = w->start;
            vec2 end =  w->end;

            s32 testY1 = (s32)start.y - (s32)point.y;
            s32 testY2 = (s32)end.y - (s32)point.y;
            if ((testY1^testY2) < 0)
            {
                // Different signs, point.y is between
                s32 testX1 = (s32)start.x - (s32)point.x;
                s32 testX2 = (s32)end.x - (s32)point.x;
                if ((testX1^testX2) >= 0)
                {
                    // Change sign
                    count ^= testX1;
                }
                else
                {
                    // Magic
                    count ^= (testX1*testY2 - testX2*testY1)^testY2;
                }
            }
		}
		return  (count >> 31) >= 0;
}



// This is from https://gist.github.com/TimSC/47203a0f5f15293d2099507ba5da44e6
// But it finds collisions with every wall
double Det(double a, double b, double c, double d)
{
    return (a)*(d)-(b)*(c);
}
bool Map_FindIntersectionWithWallGithub(
    float x1,
    float y1,
    float x2,
    float y2,
    float x3,
    float y3,
    float x4,
    float y4,
    vec2* pointOUT
     )
{
    double detL1 = Det(x1, y1, x2, y2);
	double detL2 = Det(x3, y3, x4, y4);
	double x1mx2 = x1 - x2;
	double x3mx4 = x3 - x4;
	double y1my2 = y1 - y2;
	double y3my4 = y3 - y4;

	double xnom = Det(detL1, x1mx2, detL2, x3mx4);
	double ynom = Det(detL1, y1my2, detL2, y3my4);
	double denom = Det(x1mx2, y1my2, x3mx4, y3my4);
	if(denom == 0.0)//Lines don't seem to cross
	{
		return false;
	}

	pointOUT->x = xnom / denom;
	pointOUT->y = ynom / denom;
	if(!isfinite(pointOUT->x) || !isfinite(pointOUT->y)) //Probably a numerical issue
		return false;

	return true; //All OK
}

// This is from Wikipedia and works half right
// it finds intersections on line extensions too
// which is bad :()

bool Map_FindIntersectionWithWallUT(
    float x1,
    float y1,
    float x2,
    float y2,
    float x3,
    float y3,
    float x4,
    float y4,
    vec2* pointOUT
     )
{
    float t = ((x1-x3)*(y3-y4) - (y1-y3)*(x3-x4)) / ((x1-x2)*(y3-y4) - (y1-y2)*(x3-x4));
    float u = ((x1-x2)*(y1-y3) - (y1-y2)*(x1-x3)) / ((x1-x2)*(y3-y4) - (y1-y2)*(x3-x4));
    if (( 0 <= t && t <= 1.0f ) && (-1.0f <= u && u <= 0.0f))
    {
        *pointOUT = vec2New(x1 + t*(x2-x1), y1 + t*(y2-y1));
        return true;
    }
    /*
    {
            *pointOUT = vec2New(x3 + t*(x4-x3), y3 + t*(y4-y3));
            return true;
    }
    */
    // DEBUG write t and u instead
    *pointOUT = vec2New(t, u);
    return false;
}
// This is from bisqwit
// IntersectBox: Determine whether two 2D-boxes intersect.
// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))
#define vxs(x0,y0, x1,y1)    ((x0)*(y1) - (x1)*(y0))   // vxs: Vector cross product
#define Intersect(x1,y1, x2,y2, x3,y3, x4,y4) ((vec2New ( \
    vxs(vxs(x1,y1, x2,y2), (x1)-(x2), vxs(x3,y3, x4,y4), (x3)-(x4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)), \
    vxs(vxs(x1,y1, x2,y2), (y1)-(y2), vxs(x3,y3, x4,y4), (y3)-(y4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)) )))

bool Map_FindIntersectionWithWallBisqwit(
    float x1,
    float y1,
    float x2,
    float y2,
    float x3,
    float y3,
    float x4,
    float y4,
    vec2* pointOut
     )
{
    *pointOut = Intersect(x1, y1, x2, y2, x3, y3, x4, y4);
    return (isfinite(pointOut->x) && isfinite(pointOut->y));
}


bool Map_FindIntersectionWithWall(vec2 moveStart, vec2 moveEnd, Wall* wall, vec2* pointOUT)
{
    float x1 = moveStart.x;
    float y1 = moveStart.y;
    float x2 = moveEnd.x;
    float y2 = moveEnd.y;

    float x3 = wall->start.x;
    float y3 = wall->start.y;
    float x4 = wall->end.x;
    float y4 = wall->end.y;
    return Map_FindIntersectionWithWallUT(x1, y1, x2, y2, x3, y3, x4, y4, pointOUT);
}

vec2 Wall_GetMiddle(Wall* w)
{
    return vec2Add(w->start, vec2Multiply( vec2Subtract(w->end, w->start), 0.5f));
}
vec2 Wall_GetNormal(Wall* w)
{
    vec2 wallVector = vec2Subtract(w->end, w->start);
    return vec2Normalize(Vec2CrossWithZ(wallVector));
}

bool Map_IsPointInsideWall(vec2 point, Wall* wall)
{
    // negative if on the right side of wall.
    // walls go clockwise

    vec2 wallVector = vec2Subtract(wall->end, wall->start);
    float crossZ = Vec2CrossToZ(wallVector, vec2Subtract(point, wall->start));
    // DANGER Again, this code works differently TM
    return crossZ > 0.0f;
}
