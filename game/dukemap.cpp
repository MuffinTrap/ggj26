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
