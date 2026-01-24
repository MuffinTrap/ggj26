#pragma once
#include "dukemap.h"

struct SectorRender
{
    int number;
    int leftX;
    int rightX;
};
typedef struct SectorRender SectorRender;

struct RenderSettings2D
{
    float scaleXZ;
    float mapZoom;
    bool mapYDown;
    vec2 mapOffset;
};
typedef struct RenderSettings2D RenderSettings2D;

struct RenderSettingsOpenGL
{
    float scaleXZ;
    float scaleY;
};
typedef struct RenderSettingsOpenGL RenderSettingsOpenGL;

extern int wallXPoints[20];
extern int wallYPoints[20];


void BuildRender_Init(DukeMap* map);
/** @brief Draws the map using lines
 */
void BuildRender_DrawFirstPerson(Player* player, DukeMap* map, RenderSettings2D* settings);
/** @brief Draws the map wireframe and player
 */
void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D);

/** @brief Draws the map in 3D using OpenGL
 */
void BuildRender_DrawOpenGL(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
