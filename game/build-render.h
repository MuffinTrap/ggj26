#pragma once
#include "dukemap.h"
#include "gameplay.h"
// Forward defs

struct SectorRender
{
    s16 number;
    float limitLeft; // Field of view limits or portal limits
    float limitRight;
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
    bool centerMapToCollisionPoint;

    int drawPlayersAmount; ///< How many players to draw
    int drawCollisionPointAmount;
    bool drawSectorNumbers; ///< Draw sector numbers in green if they are rendered
    bool drawPortals; ///< Draw portal walls
    bool drawNormals; ///< Draw wall normals
    bool drawSprites; ///< Draw Sprites
    bool drawTreasure; ///< Draws treasure sprite regardless of other sprites
    bool drawWallNumbers;
    bool drawPortalDrawLimits; // Shows where portal limits are on the screen
    bool drawOrigoAndAxii;

    Color4f wallColor;
    Color4f portalColor;
    Color4f gridColor;
    float gridLineLength; ///< 1 means connect nodes, 0 means draw very small crosses at nodes

    float gridSize; // Grid in OpenGL units
};
typedef struct RenderSettings2D RenderSettings2D;

struct RenderSettingsOpenGL
{
    float scale;
    float textureScale;
    float spriteDefaultWidth;
    float spriteDefaultHeight;

    // Camera information
    float FOVyDegrees;
    float near, far;
    float aspectRatio;

};
typedef struct RenderSettingsOpenGL RenderSettingsOpenGL;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Loads a given map and prepares to draw it. Can be called multiple times
 */
void BuildRender_Init(DukeMap* map, RenderSettingsOpenGL* settings3D);

/** @brief Draws the map wireframe and player(s) and other information defined in the settings
 */
void BuildRender_DrawTopDown(Player* players, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D);

/** @brief Draws the map in 3D using OpenGL, using the functions below
 * @param player The player whose point of view is used
 * @param map The map.
 * @param settings Rendering settings
 */
void BuildRender_Draw3D(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);

void BuildRender_DrawSectorWalls(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
void BuildRender_DrawSectorFloorsAndCeilings(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
void BuildRender_DrawSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings);
void BuildRender_DrawTempSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings);

/**
 * @brief Visualize how the sectors and portals are drawn
 */
void BuildRender_DrawSectorRequests(RenderSettingsOpenGL* settings3D);

SectorRender* BuildRender_GetDrawnSectorNumbers();
s16 BuildRender_GetDrawnSectorAmount();
bool BuildRender_WasSectorDrawn(s16 sectornumber);


void BuildRender_ExportCurrentMapToObj(DukeMap* map, const char* filename, RenderSettingsOpenGL* settings);

#ifdef __cplusplus
}
#endif
