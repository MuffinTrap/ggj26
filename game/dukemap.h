#pragma once
		#include <mgdl/ccVector/ccVector.h>

struct Wall
{
    vec2 start;
    vec2 end;
    int neighborSector;

};
typedef struct Wall Wall;

struct Sector
{
    int pointAmount;
    int wallAmount;
    Wall* walls;
    int ceilingZ;
    int floorZ;
};
typedef struct Sector Sector;


struct DukeMap
{
    Sector* sectors;
    int sectorAmount;
};

typedef struct DukeMap DukeMap;

#ifdef __cplusplus
extern "C" {
#endif

Sector* Map_GetSector(DukeMap* map, int sectorNumber);
vec2 Sector_GetVertex(int vi);
Wall* Sector_GetWall(int wi);

#ifdef __cplusplus
}
#endif
