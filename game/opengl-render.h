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
struct Sprite;

#define RENDERER_PICNUM_DEFAULT 0

enum OpenGLRender_FloorDrawMode
{
	FloorDraw_Count, ///< When counting floor buffer size
	FloorDraw_Buffer, ///< When bufferint the floor vertices
	FloorDraw_Render, ///< When sending vertices to OpenGL or general vertex buffer
	FloorDraw_BufferRender ///< When drawing buffered floors and ceilings
};
typedef enum OpenGLRender_FloorDrawMode OpenGLRender_FloorDrawMode;

/**
 * @brief Init the renderer
 */
void OpenGLRender_Init();

void OpenGLRender_Deinit();

/**
 * @brief Registers a sprite to be used when a picnum is drawn
 * @details This will store the sprite pointer in array. picnum 0 is the default texture that is used if asked for unregistered picnum.
 * @param picnum What picnum does this Sprite match.
 * @param sprite The sprite to use
 * @returns True if registering succeeded: there was space in array
 */
bool OpenGLRender_RegisterSprite(s16 picnum, Sprite* sprite);
/**
 * @brief Registers a texture to be used when a picnum is drawn
 * @details This will create a new Sprite with one frame and store the sprite pointer in array
 * @param picnum What picnum does this Sprite match
 * @param texture The texture to use
 * @returns True if registering succeeded: there was space in array
 */
bool OpenGLRender_RegisterTexture(s16 picnum, Texture* texture);
// Dark version of texture
bool OpenGLRender_RegisterTexture_DARK(s16 picnum, Texture* texture);

/**
 * @brief Sets up the rendering state for drawing walls, floors, ceilings and sprites
 */
void OpenGLRender_StartDrawingPolygons();

	void OpenGLRender_DrawQuad(vec2 start, vec2 end, vec2 normalXZ, float floorY, float ceilingY, s16 picnum, s8 brightnessOffset, RenderSettingsOpenGL* settings3D);
	void OpenGLRender_DrawWall(DukeMap* map, Wall* w, float floorY, float ceilingY, RenderSettingsOpenGL* settings);
	void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, Sector* sector, s16 sectorIndex, bool floor);
	void OpenGLRender_DrawSprite(vec3 position, float width, float height, float spriteAngle, float playerAngle, SpriteAlignment alignment, SpritePivot pivot, s16 picnum, s8 brightnessOffset);

/**
 * @brief Finalizes the drawing
 */
void OpenGLRender_EndDrawingPolygons();


// Store floor vertices of each sector to buffer
void OpenGLRender_StartCountingFloorBufferSize(s16 sectorAmount);
void OpenGLRender_StopCountingFloorBufferSize();

void OpenGLRender_StartFillingFloorBuffer(s16 sectorAmount);
void OpenGLRender_StopFillingFloorBuffer();

void OpenGLRender_StartDrawingFloorsFromBuffer();
void OpenGLRender_StopDrawingFloorsFromBuffer();

void OpenGLRender_Line2(int x1, int z1, int x2, int z2);
void OpenGLRender_Line3(vec3 start, vec3 end);

void OpenGLRender_AnimateSprites();

Texture* OpenGLRender_GetTexture(s16 picnum);
void OpenGLRender_SetColor(DefaultColor oc);
void OpenGLRender_SetColor4f(Color4f color);
void OpenGLRender_DrawDot(vec2 point, float size, DefaultColor color);

void SetDark(bool newDark);
