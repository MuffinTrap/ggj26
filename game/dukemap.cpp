#include "dukemap.h"
#include <mgdl.h>
Sector* Map_GetSector(DukeMap* map, int sectorNumber)
{
    mgdl_assert_print((sectorNumber>= 0 && sectorNumber < map->sectorAmount),"Invalid sector for Map_GetSector");
    return &map->sectors[sectorNumber];
}
Wall* Sector_GetWall(Sector* sector, int wi)
{
    mgdl_assert_print((wi>= 0 && wi < sector->wallAmount),"Invalid wall index for Sector_GetWall");
    return &sector->walls[wi];
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
        Log_InfoF("Sector n: %d FloorZ %d CeilingZ %d\n", i, s->floorz, s->ceilingz);
    }

    for (int s = 0; s < map->wallAmount; s++)
    {
        Wall* w = &map->walls[s];
        Log_InfoF("Wall n: %d (%d,%d)\n", s, w->x, w->y);
    }


    for (int i = 0; i < map->spriteAmount; i++)
    {

    }
}
