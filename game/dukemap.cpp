#include "dukemap.h"
#include <mgdl.h>
#include <mgdl/mgdl-vectorfunctions.h>
#include <mgdl/ccVector/ccVector.h>
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
