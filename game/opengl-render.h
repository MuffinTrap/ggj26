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
 * @brief Sets up the rendering state for drawing walls and sprites
 */
void OpenGLRender_StartDrawingPolygons();


	void OpenGLRender_DrawWall(DukeMap* map, Wall* w, float floorY, float ceilingY, RenderSettingsOpenGL* settings);
	void OpenGLRender_DrawSprite(vec3 position, float width, float height, float spriteAngle, float playerAngle, SpriteAlignment alignment, SpritePivot pivot, s16 picnum, s8 brightnessOffset);

	void OpenGLRender_DrawQuad(vec2 start, vec2 end, vec2 normalXZ, float floorY, float ceilingY, s16 picnum, s8 brightnessOffset, RenderSettingsOpenGL* settings3D);
/**
 * @brief Finalizes the drawing
 */
void OpenGLRender_EndDrawingPolygons();


// Tesselate and store all floors to buffer
void OpenGLRender_StartCountingFloorBufferSize(s16 sectorAmount, s16 wallAmount);
/**
 * @brief Tesselates a floor of sector.
 */
void OpenGLRender_TesselateFloor(DukeMap* map, u16 sectorIndex);
void OpenGLRender_StopCountingFloorBufferSize();


/**
 * @brief Sets up rendering state to draw floors and ceilings from a buffer
 */
void OpenGLRender_StartDrawingFloorsFromBuffer();
void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, s16 sectorIndex, bool floor);

void OpenGLRender_Line2(int x1, int z1, int x2, int z2);
void OpenGLRender_Line3(vec3 start, vec3 end);

void OpenGLRender_AnimateSprites();

Texture* OpenGLRender_GetTexture(s16 picnum);
void OpenGLRender_SetColor(DefaultColor oc);
void OpenGLRender_SetColor4f(Color4f color);
void OpenGLRender_DrawDot(vec2 point, float size, DefaultColor color);

void SetDark(bool newDark);
