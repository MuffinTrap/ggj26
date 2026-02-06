#pragma once
#include <mgdl/mgdl-types.h>
#include <mgdl/mgdl-color.h>
#include <mgdl/ccVector/ccVector.h>
#include "dukemap.h"
struct RenderSettingsOpenGL;
struct DukeMap;
struct Sector;
struct Wall;
struct Texture;

/**
 * @brief This code uses OpenGL to draw a build map
 */

void OpenGLRender_Init();

/**
 * @brief Sets up the rendering state for drawing walls, floors, ceilings and sprites
 */
void OpenGLRender_StartDrawingPolygons();

	void OpenGLRender_DrawQuad(vec2 start, vec2 end, vec2 normalXZ, float floorY, float ceilingY, s16 picnum, s8 brightnessOffset, RenderSettingsOpenGL* settings3D);
	void OpenGLRender_DrawWall(DukeMap* map, Wall* w, float floorY, float ceilingY, RenderSettingsOpenGL* settings);
	void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, Sector* sector, bool floor);
	void OpenGLRender_DrawSprite(vec3 position, float width, float height, float spriteAngle, float playerAngle, SpriteAlignment alignment, SpritePivot pivot, s16 picnum, s8 brightnessOffset);

/**
 * @brief Finalizes the drawing
 */
void OpenGLRender_EndDrawingPolygons();

void OpenGLRender_Line2(int x1, int z1, int x2, int z2);
void OpenGLRender_Line3(vec3 start, vec3 end);

void OpenGLRender_AnimateSprites();

Texture* OpenGLRender_GetTexture(s16 picnum);
void OpenGLRender_SetColor(DefaultColor oc);
void OpenGLRender_SetColor4f(Color4f color);
void OpenGLRender_DrawDot(vec2 point, float size, DefaultColor color);

void SetDark(bool newDark);
