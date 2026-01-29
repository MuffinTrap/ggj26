#pragma once
#include "dukemap.h"
// Forward defs

struct SectorRender
{
    s16 number;
};
typedef struct SectorRender SectorRender;

struct RenderSettings2D
{
    float scaleXZ;
    float mapZoom;
    vec2 mapOffset;

    // Wall drawing debugging
    int drawOneWall; ///< if -1 no walls, if zero or positive that wall
    int drawOneSector; ///< if -1 no sectors, if zero or positive that sector

    // Collision test debugging
    vec2 collisionPoint;
    s16 collisionInsideSector;
    float collisionLength;
    float collisionAngleDeg;
    bool movePlayer;
    bool rotateMap;
    bool centerMapToPlayer;
};
typedef struct RenderSettings2D RenderSettings2D;

struct RenderSettingsOpenGL
{
    float scaleXZ;
    float scaleY;
    float textureScale;
    float spriteDefaultWidth;
    float spriteDefaultHeight;
};
typedef struct RenderSettingsOpenGL RenderSettingsOpenGL;

extern int wallXPoints[20];
extern int wallYPoints[20];


void BuildRender_Init(DukeMap* map, RenderSettingsOpenGL* settings3D);
/** @brief Draws the map wireframe and player
 */
void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D);

/** @brief Draws the map in 3D using OpenGL
 */
void BuildRender_Draw3D(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
void BuildRender_DrawSectors(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
void BuildRender_DrawSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings);


void BuildRender_TesselationTest();

SectorRender* BuildRender_GetDrawnSectorNumbers();
s16 BuildRender_GetDrawnSectorAmount();
bool BuildRender_WasSectorDrawn(s16 sectornumber);
