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
    w->start = vec2New(w->x, w->y);
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

    float t = ((x1-x3)*(y3-y4) - (y1-y3)*(x3-x4)) / ((x1-x2)*(y3-y4) - (y1-y2)*(x3-x4));
    if ( 0 <= t && t <= 1.0f )
    {
        *pointOUT = vec2New(x1 + t*(x2-x1), y1 + t*(y2-y1));
        return true;
    }
    else
    {
        float u = ((x1-x2)*(y1-y3) - (y1-y2)*(x1-x3)) / ((x1-x2)*(y3-y4) - (y1-y2)*(x3-x4));

        if (0 <= u && u <= 1.0f)
        {
            *pointOUT = vec2New(x3 + t*(x4-x3), y3 + t*(y4-y3));
            return true;
        }
    }
    return false;
}
/*
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

    float d = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);
    if (fabsf(d) < 0.00001f)
    {
        return false;
    }
    pointOUT->x = (x1*y2 - y1*x2)*(x3-x4) - (x1 - x2)*(x3*y4 - y3*x4) / d;
    pointOUT->y = (x1*y2 - y1*x2)*(y3-y4) - (y1 - y2)*(x3*y4 - y3*x4) / d;
    return true;
}
*/

bool Map_IsPointInsideWall(vec2 point, Wall* wall)
{
    // negative if on the right side of wall.
    // walls go clockwise

    vec2 wallVector = vec2Subtract(wall->end, wall->start);
    float crossZ = Vec2CrossToZ(wallVector, point);
    return crossZ < 0.0f;
}
