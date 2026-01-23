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
    float floor0Level;
    float heightPerDukeK; /**< How much height is 1024 duke units */
    float widthPerDukeK; /**< How much width is 1024 duke units */
    float mapZoom;
    vec2 mapOffset;
};
typedef struct RenderSettings2D RenderSettings2D;

struct RenderSettingsOpenGL
{
    float floor0Level;
    float heightPerDukeK; /**< How much height is 1024 duke units */
    float widthPerDukeK; /**< How much width is 1024 duke units */
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
void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettings2D* settings);

/** @brief Draws the map in 3D using OpenGL
 */
void BuildRender_DrawOpenGL(Player* player, DukeMap* map, RenderSettingsOpenGL* settings);
