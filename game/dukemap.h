#pragma once
		#include <mgdl/mgdl-vectorfunctions.h>
		#include <mgdl/mgdl-types.h>
// Forward def
struct Player;



struct DSprite
{
    // Game info

    // From file
    // s32 x, y, z; ///< Sprite position in map
    vec3 position;
    u16 cstat; ///< Type of sprite bitfield. Use DukeMap_GetSpriteAlignment
    s16 picnum; ///< Texture index
    s8 shade;
    u8 pal;
    u8 clipdist; ///< Size in map as square, only used in FACE alignment
    u8 filler;
    u8 xrepeat, yrepeat; ///< How many pixels wide and tall : Default 64x64
    s8 xoffset, yoffset; ///< Offset of center
    s16 sectnum;
    s16 statnum; ///< Status: inactive, bullet, monster etc...
    s16 ang; ///< Facing angle
    s16 owner; // Owning player index
    s16 xvel, yvel, zvel; ///< Velocity
    u16 lotag, hitag;
    s16 extra;
};
struct Wall
{
    // GLdouble glutVertices[3]; ///< GLUT tesselation needs this
    // From file
    s32 x, z; ///< Coordinates of the left side. Right side is left side of next wall.
    s16 point2; ///< Index of next wall in sector's walls.
    s16 nextwall; ///< Index of wall on the other side or -1 if no sector there
    s16 nextsector; ///< Index of sector on the other side or -1
    u16 cstat; ///< Stats about wall
    s16 picnum; ///< Texture number
    s16 overpicnum; ///< Texture number for masked and one-way walls
    s8 shade; /// Offset to shade: darker or brighter
    u8 pal;  ///< Palette index
    u8 xrepeat, yrepeat;  ///< Repeat texture more times
    u8 xpanning, ypanning; //< Texture offset
    u16 lotag, hitag;
    s16 extra;

};
typedef struct Wall Wall;

struct Sector
{
    // From file
    s16 wallptr; /**< Index of first wall */
    s16 wallnum; /**< and amount of walls in this sector */
    s32 ceilingy, floory; ///< Y of ceiling and floor of first point
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

    // For texture coordinates
    vec2 minXZPoint;
    vec2 sizeXZ;
    vec2 maxTexCoord;
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

enum MoveResult
{
    Move_Ok,
    Move_HitWall,
    Move_HitPortal,
    Move_Cancel
};
typedef enum MoveResult MoveResult;

enum SpriteAlignment
{
    Sprite_FACE, ///< Billboard
    Sprite_WALL, ///< Not billboard, drawn like wall
    Sprite_FLOOR ///< Flat on floor or ceiling
};
typedef enum SpriteAlignment SpriteAlignment;

#define SPRITE_PIVOT_BIT 7
enum SpritePivot
{
    Sprite_PivotCenter, // Center is position
    Sprite_PivotFoot    // center is position + height/2
};
typedef enum SpritePivot SpritePivot;

enum SpriteLOTAG
{
    Sprite_LOTAG_Multiplayer_Start = 90
};
typedef enum SpriteLOTAG SpriteLOTAG;

#define LEVEL_END_LOTAG 65535
#define TREASURE_LOTAG 100

typedef struct DukeMap DukeMap;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Convert the information loaded from the file into game units and enums.
     * @param map The map to convert.
     */
void Map_ConvertToGameUnits(DukeMap* map);
void Map_FindIslandSectors(DukeMap* map);
void Map_PrintInfo(DukeMap* map);
void Map_InitPlayer(DukeMap* map, Player* player);
void Map_InitPlayers(DukeMap* map, Player* players, int playerAmount);

Sector* Map_GetSector(DukeMap* map, s16 sectorNumber);
s32 Map_GetSectorFloorHeight(DukeMap* map, s16 sectorNumber);
s32 Map_GetSectorCeilingHeight(DukeMap* map, s16 sectorNumber);

Wall* Map_GetWallInSector(DukeMap* map, s16 sector, s16 wi);
Wall* Map_GetWallInSectorPtr(DukeMap* map, Sector* sector, s16 wi);
Wall* Map_GetWallEnd(DukeMap* map, Wall* w);
Wall* Map_GetWall(DukeMap* map, s16 wallIndex);
vec2 Map_GetWallMiddle(DukeMap* map, Wall* w);
vec2 Map_GetWallNormal(DukeMap* map, Wall* w);

SpriteAlignment Sprite_GetAlignment(DSprite* sprite);
SpritePivot Sprite_GetPivot(DSprite* sprite);
DSprite* Map_GetSprite(DukeMap* map, s16 spriteIndex);

/**
 * @brief Searches for and returns the first sprite with matching tags
 * @param lotag Lotag of the sprite
 * @param hitag Hitag of the sprite
 * @return First matching sprite or nullptr if none found
 */
DSprite* Map_FindSprite(DukeMap* map, s16 lotag, s16 hitag);

bool Map_IsPointInsideSectorOG(DukeMap* map, vec2 point, int sectorNumber);
bool Map_IsPointInsideSectorRay(DukeMap* map, vec2 point, int sectorNumber);
bool Map_IsPointInsideWall(DukeMap* map, vec2 point, Wall* wall);
bool Map_FindIntersectionWithWall(DukeMap* map, vec2 moveStart, vec2 moveEnd, Wall* wall, vec2* pointOUT);

MoveResult Map_MovePointInMap(DukeMap* map, vec2 start, vec2 end, s16 sector, vec2* positionOut, s16* sectorOut);


#ifdef __cplusplus
}
#endif
