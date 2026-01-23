#pragma once
		#include <mgdl/mgdl-vectorfunctions.h>
		#include <mgdl/mgdl-types.h>
// Forward def
struct Player;



struct DSprite
{
    // Game info

    // From file
    s32 x, y, z; ///< Sprite position in map
    u16 cstat;
    s16 picnum;
    s8 shade;
    u8 pal, clipdist, filler;
    u8 xrepeat, yrepeat;
    s8 xoffset, yoffset;
    s16 sectnum, statnum;
    s16 ang, owner, xvel, yvel, zvel;
    u16 lotag, hitag;
    s16 extra;
};
struct Wall
{
    vec2 start;
    vec2 end;

    // From file
    s32 x, y; ///< Coordinates of the left side. Right side is left side of next wall.
    s16 point2; ///< Index of next wall in sector's walls.
    s16 nextwall; ///< Index of wall on the other side or -1 if no sector there
    s16 nextsector; ///< Index of sector on the other side or -1
    u16 cstat; ///< Stats about wall
    s16 picnum, overpicnum;
    s8 shade;
    u8 pal, xrepeat, yrepeat, xpanning, ypanning;
    u16 lotag, hitag;
    s16 extra;

};
typedef struct Wall Wall;

struct Sector
{
    // From file
    s16 wallptr; /**< Index of first wall */
    s16 wallnum; /**< and amount of walls in this sector */
    s32 ceilingz, floorz; ///< Z of ceiling and floor of first point
    u16 ceilingstat, floorstat; ///< Stats about ceiling and floor
    s16 ceilingpicnum; ///< Texture of ceiling
    s16 ceilingheinum; ///< Sloping angle 0:flat, 4096: 45 degrees
    s8 ceilingshade;
    u8 ceilingpal, ceilingxpanning, ceilingypanning; ///< Palette index and texture olic ffsets
    s16 floorpicnum, floorheinum;
    s8 floorshade;
    u8 floorpal, floorxpanning, floorypanning;
    u8 visibility; ///< How distance affects shading
    u8 filler; ///< Padding byte
    u16 lotag, hitag; ///< Game specific info
    s16 extra;
};
typedef struct Sector Sector;


struct DukeMap
{
    s32 version;
    vec3 startPosition;
    s16 startAngle; /**< 0 - 2047. 0 : (0,1) 512 = (1,0) 1024: (0, -1) */

    s16 startingSector;

    s16 sectorAmount;
    Sector* sectors;
    s16 wallAmount;
    Wall* walls;
    s16 spriteAmount;
    DSprite* sprites;
};

typedef struct DukeMap DukeMap;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Convert the information loaded from the file into game units and enums.
     * @param map The map to convert.
     */
void Map_ConvertToGameUnits(DukeMap* map);
Sector* Map_GetSector(DukeMap* map, int sectorNumber);
Wall* Map_GetWallInSector(DukeMap* map, s16 sector, s16 wi);
void Map_PrintInfo(DukeMap* map);
void Map_InitPlayer(DukeMap* map, Player* player);

float Map_AngleToRad(s16 angleInt);

#ifdef __cplusplus
}
#endif
