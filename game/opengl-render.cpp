
#include "opengl-render.h"
#include <mgdl.h>
#include <mgdl/mgdl-cache.h>
#include <mgdl/mgdl-draw2d.h>
#include "dukemap.h"
#include "build-render.h"
#include "dukemath.h"
#include "tesselator.h"
#include "obj-export.h"

// Used to set normals when drawing floors and ceilings
static GLfloat floorNormal[3];
static GLfloat ceilingNormal[3];

// Store floor vertices of each sector to buffer
// This buffer needs to hold all the vertices of every floor
static GLfloat* floorBuffer = nullptr; // All vertices of all floors: 3 position 2 uv
static const u16 FLOOR_BUFFER_VERTEX_SIZE = 5; ///< How many floats per vertex
static u16 floorBufferSizeVertices = 0;

static GLushort* floorIndexBuffer = nullptr; // All indices of all floors
static u32 floorIndexBufferSize = 0;

// Store wall vertices of each wall to buffer
// This buffer needs to hold all the walls
static GLfloat* wallBuffer = nullptr; // All vertices of all walls: 3 position
static const u16 WALL_BUFFER_VERTEX_SIZE = 3; ///< How many floats per vertex
static u16 wallBufferSizeVertices = 0;
static u32 wallBufferVertexIndex = 0;

static GLushort* wallIndexBuffer = nullptr; // All indices of all walls
static u32 wallIndexBufferSize = 0;
static u32 wallIndexBufferIndex = 0;

static Tesselator_BufferIndices* floorStartIndices = nullptr; // Buffer end indices of each floor in vertex and index buffers: NOTE First floor starts at indices (0,0)
static s16 bufferedSectorAmount = 0; // How many floors are in the buffers

// Arrays for storing sprite pointers and matching picnums to Sprites
#define RENDERER_SPRITE_ARRAY_SIZE 128
#define RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE 2048
static sizetype nextFreeSpriteSlot = 0;
static Sprite** spritePtrArray = nullptr;
static u16* picnumToSpriteArray = nullptr;

// Default texture
Texture* checkers;


// Animation variables for animating sprites
int animationFrame = 0;
float animationRate = 0.05f;
float animationTimer = 0;


// TODO Alternative textures? Multiple Textures per sprite
#define RENDERER_PICNUM_TO_SPRITE_ARRAY_DARK_SIZE 2048
static u16* picnumToSpriteArray_DARK;
static bool dark = false;

// Buffer for drawing vertices of the walls and sprites
// 3 position + 2 texture coordinates
#define FULL_VERTEX_SIZE_FLOATS (3 + 2)
#define VERTEX_BUFFER_SIZE_VERTICES 16 // This will always contain a quad
#define VERTEX_BUFFER_SIZE_BYTES (FULL_VERTEX_SIZE_FLOATS * sizeof(float) * VERTEX_BUFFER_SIZE_VERTICES)
static GLfloat* vertexBuffer = nullptr;
#define VERTEX_INDEX_BUFFER_SIZE_INDICES 6
static int vertexBufferIndexVertices = 0;
static GLushort vertexIndexBuffer[VERTEX_INDEX_BUFFER_SIZE_INDICES];

// What uv limits are active
static RectF polygonUVLimits;
static RectF zeroOffset;

/**
 * @brief Sets OpenGL to draw from vertexBuffer
 */
static void ActivateVertexBuffer()
{
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 5, &vertexBuffer[0]);
    glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 5, &vertexBuffer[3]);
}


/**
 * @brief Sets OpenGL to draw from floorbuffer at given index
 */
static void ActivateFloorBuffer(u32 index)
{
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 5, &floorBuffer[index]);
    glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 5, &floorBuffer[index + 3]);
}


static GLuint lastTextureName = 0; // 0 is not a valid texture name
static void SetTexture(GLuint glTextureName)
{
    if (lastTextureName != glTextureName)
    {
        glBindTexture(GL_TEXTURE_2D, glTextureName);
        lastTextureName = glTextureName;
    }
}

RectF SetPicnum_GetUVoffset(s16 picnum)
{
    u16 spriteIndex = picnumToSpriteArray[picnum];
    if (dark)
    {
        // Not all picnums have dark version
        u16 darkspriteIndex = picnumToSpriteArray_DARK[picnum];
        if (darkspriteIndex > 0)
        {
            spriteIndex = darkspriteIndex;
        }
    }

    if (spriteIndex == 0)
    {
        Sprite* defaultSprite = spritePtrArray[spriteIndex];
        if (defaultSprite == nullptr)
        {
            Log_Error("No sprite registered for default picnum 0!");
        }
        SetTexture(defaultSprite->_font->_fontTexture->textureId);
        return Sprite_GetTextureCoordinates(defaultSprite, animationFrame % defaultSprite->_font->_characterCount);
    }
    Sprite* sprite = spritePtrArray[spriteIndex];
    SetTexture(sprite->_font->_fontTexture->textureId);
    return Sprite_GetTextureCoordinates(sprite, animationFrame % sprite->_font->_characterCount);
}

void OpenGLRender_SetColor(DefaultColor oc)
{
    Color4f* c = Color_GetDefaultColor(oc);
    glColor4fv(&c->red);
}
void OpenGLRender_SetColor4f(Color4f color)
{
    glColor4fv(&color.red);
}

void OpenGLRender_Line2(int x1, int z1, int x2, int z2)
{
    glVertex2i(x1, z1);
    glVertex2i(x2, z2);
}

void OpenGLRender_Line3(vec3 start, vec3 end)
{
	glVertex3f(start.x, start.y, start.z);
	glVertex3f(end.x, end.y, end.z);
}

/**
 * @brief Changes Duke shade value to grayscale
 * brightness. Used to color the vertices
 * @param brightnessOffset Positive values are darker. 32 is black. -1 is brighter but smaller values have no meaning
 */
static float BrightnessOffsetToColor(s8 brightnessOffset)
{
    static const float brightnessStep = 1.0f/32.0f;
    return clampF( (1.0f - (brightnessOffset * brightnessStep)), 0.0f, 1.0f);
}

/**
 * @brief Sets up opengl state to draw a polygon from vertex buffer
 */
static void BeginVertexBufferPolygon(const vec3 normal, const RectF uvLimits, const float brightness)
{
    polygonUVLimits = uvLimits;
    glNormal3f(normal.x, normal.y, normal.z);
    glColor3f(brightness, brightness, brightness);
    vertexBufferIndexVertices = 0;
}
static void EndVertexBufferPolygon()
{
    // Flush all written vertices
    mgdl_CacheFlushRange(vertexBuffer, VERTEX_BUFFER_SIZE_VERTICES * FULL_VERTEX_SIZE_FLOATS * sizeof(float));
    glDrawElements(GL_TRIANGLES, VERTEX_INDEX_BUFFER_SIZE_INDICES, GL_UNSIGNED_SHORT, vertexIndexBuffer);
    //glDrawArrays(GL_TRIANGLES, 0, vertexBufferIndexVertices);
}


/**
 * @brief Stores a vertex to vertexbuffer
 */
static void BufferVertex(const float x, const float y, const float z, const float u, const float v)
{
    GLfloat* vertex = &vertexBuffer[vertexBufferIndexVertices * FULL_VERTEX_SIZE_FLOATS];
    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = z;

    vertex[3] = polygonUVLimits.x + u * polygonUVLimits.w;
    vertex[4] = polygonUVLimits.y + v * polygonUVLimits.h;

    vertexBufferIndexVertices += 1;

    // if buffer becomes full, render the contents and start from beginning
    if (vertexBufferIndexVertices >= VERTEX_BUFFER_SIZE_VERTICES)
    {
        // Flush all written vertices
        EndVertexBufferPolygon();
        vertexBufferIndexVertices = 0;
    }
}

static void BufferVertexV(const vec3 position, const vec2 textureCoord)
{
    BufferVertex(position.x, position.y, position.z, textureCoord.x, textureCoord.y);
}

void OpenGLRender_StartDrawingPolygons()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

void OpenGLRender_EndDrawingPolygons()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glColor3f(1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    lastTextureName = 0;
}

/**
 * @brief Calculate the uv coordinates of a floor or ceiling vertex in a sector
 */
static vec2 CalculateFloorOrCeilingUV(const Sector* sector, float x, float z)
{
    float xrange = sector->sizeXZ.x;
    float zrange = sector->sizeXZ.y;
    float xdiff = x - sector->minXZPoint.x;
    float zdiff = z - sector->minXZPoint.y;
    float tx = xdiff/xrange * sector->maxTexCoord.x;
    float tz = zdiff/zrange * sector->maxTexCoord.y;
    return vec2New(tx, tz);
}

void SetWrap(GLuint textureName)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glDisable(GL_TEXTURE_2D);
}

void OpenGLRender_Init()
{
    zeroOffset.x = 0.0f;
    zeroOffset.y = 0.0f;
    zeroOffset.w = 1.0f;
    zeroOffset.h = 1.0f;

    floorNormal[0] = 0;
    floorNormal[1] = 1;
    floorNormal[2] = 0;
    ceilingNormal[0] = 0;
    ceilingNormal[1] = -1;
    ceilingNormal[2] = 0;

    checkers = Texture_GenerateCheckerBoard();

    if (vertexBuffer == nullptr)
    {
        vertexBuffer = (GLfloat*)mgdl_AllocateGraphicsMemory(VERTEX_BUFFER_SIZE_BYTES);
    }
    vertexIndexBuffer[0] = 0;
    vertexIndexBuffer[1] = 1;
    vertexIndexBuffer[2] = 2;
    vertexIndexBuffer[3] = 2;
    vertexIndexBuffer[4] = 3;
    vertexIndexBuffer[5] = 0;

    if (picnumToSpriteArray == nullptr)
    {
        picnumToSpriteArray = (u16*)mgdl_AllocateGeneralMemory(RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE * sizeof(u16));
        for (int i = 0; i < RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE; i++)
        {
            picnumToSpriteArray[i] = 0;
        }
    }
    if (picnumToSpriteArray_DARK == nullptr)
    {
        picnumToSpriteArray_DARK = (u16*)mgdl_AllocateGeneralMemory(RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE * sizeof(u16));
        for (int i = 0; i < RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE; i++)
        {
            picnumToSpriteArray_DARK[i] = 0;
        }
    }
    if (spritePtrArray == nullptr)
    {
        nextFreeSpriteSlot = 0;
        spritePtrArray = (Sprite**)mgdl_AllocateGeneralMemory(RENDERER_SPRITE_ARRAY_SIZE * sizeof(Sprite*));
        for (int i = 0; i < RENDERER_SPRITE_ARRAY_SIZE; i++)
        {
            spritePtrArray[i] = nullptr;
        }
    }
}

void OpenGLRender_Deinit()
{
    for (int i = 0; i < RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE; i++)
    {
        if (spritePtrArray[i] != nullptr)
        {
            mgdl_FreeGeneralMemory(spritePtrArray[i]);
        }
    }
    mgdl_FreeGeneralMemory(spritePtrArray);
    mgdl_FreeGeneralMemory(picnumToSpriteArray);
    mgdl_FreeGeneralMemory(picnumToSpriteArray_DARK);
}

bool OpenGLRender_RegisterSprite(s16 picnum, Sprite* sprite)
{
    if (nextFreeSpriteSlot < RENDERER_SPRITE_ARRAY_SIZE)
    {
        spritePtrArray[nextFreeSpriteSlot] = sprite;
        picnumToSpriteArray[picnum] = nextFreeSpriteSlot;
        nextFreeSpriteSlot += 1;
        SetWrap(sprite->_font->_fontTexture->textureId);
        return true;
    }
    else
    {
        return false;
    }
}

bool OpenGLRender_RegisterTexture_DARK(s16 picnum, Texture* texture)
{
    if (nextFreeSpriteSlot < RENDERER_SPRITE_ARRAY_SIZE)
    {
        Font* f = Font_Load(texture, texture->width, texture->height, 0);
        Sprite* sprite = Sprite_Load(f);
        spritePtrArray[nextFreeSpriteSlot] = sprite;
        picnumToSpriteArray_DARK[picnum] = nextFreeSpriteSlot;
        nextFreeSpriteSlot += 1;
        SetWrap(sprite->_font->_fontTexture->textureId);
        return true;
    }
    return false;

}

bool OpenGLRender_RegisterTexture(s16 picnum, Texture* texture)
{   if (nextFreeSpriteSlot < RENDERER_SPRITE_ARRAY_SIZE)
    {
        Font* f = Font_Load(texture, texture->width, texture->height, 0);
        Sprite* sprite = Sprite_Load(f);
        bool ok = OpenGLRender_RegisterSprite(picnum, sprite);
        if (ok == false)
        {
            free(sprite); // DANGER Should Sprite or mgdl handle this?
        }
        return ok;
    }
    else
    {
        return false;
    }
}

void DrawQuad(vec2 start, vec2 end, const vec2 normalXZ, float floorY, float ceilingY, s16 picnum, s8 brightnessOffset, RenderSettingsOpenGL* settings3D)
{
    // Keep texture aspect 1:1 unless told otherwise
    float width = vec2Length( vec2Subtract(end, start)) * settings3D->scale;
    float height = (ceilingY - floorY) * settings3D->scale;

    float aspect = width/height;
    float tex_x1 = 0.0f;
    float tex_x2 = aspect * height * settings3D->textureScale;
    float tex_bottom = 0.0f;
    float tex_top = 1.0 * height * settings3D->textureScale;
    vec3 normal = vec3New(normalXZ.x, 0.0f, normalXZ.y);

    ActivateVertexBuffer();

    BeginVertexBufferPolygon(normal, SetPicnum_GetUVoffset(picnum), BrightnessOffsetToColor(brightnessOffset));

        // Build Triangles for wall
        BufferVertex(start.x, floorY, start.y, tex_x1, tex_bottom); // 0
        BufferVertex(end.x, floorY, end.y, tex_x2, tex_bottom);
        BufferVertex(end.x, ceilingY, end.y, tex_x2, tex_top);
        BufferVertex(start.x, ceilingY, start.y, tex_x1, tex_top);

    EndVertexBufferPolygon();

}

void OpenGLRender_DrawWall(DukeMap* map, Wall* w, float floorY, float ceilingY, RenderSettingsOpenGL* settings)
{
    vec2 start = vec2New(w->x, w->z);
    Wall* wend = Map_GetWallEnd(map, w);
    vec2 end =  vec2New(wend->x, wend->z);
    vec2 normalXZ = Map_GetWallNormal(map, w);
    if (w->nextsector >= 0)
    {
        // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
        Sector* neighbor = Map_GetSector(map, w->nextsector);
        int n_floorY = neighbor->floory;
        int n_ceilingY = neighbor->ceilingy;

        // if this floor height is less than adjacent: Greate wall in between: goes up
        if (floorY < n_floorY)
        {
            DrawQuad(start, end, normalXZ, floorY, n_floorY, w->picnum, w->shade, settings);
        }

        // Ceiling:
        // If this ceiling is higher than adjacent: Greate wall in between: goes down
        if (ceilingY > n_ceilingY)
        {
            Wall* otherWall = Map_GetWall(map, w->nextwall);
            DrawQuad(start, end, normalXZ, n_ceilingY, ceilingY, otherWall->picnum, w->shade, settings);
        }
    }
    else
    {
        // TODO Masked walls
        // Draw the wall
        DrawQuad(start, end, normalXZ, floorY, ceilingY, w->picnum, w->shade, settings);
    }
}


void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, s16 sectorIndex, bool floor)
{
    Sector* sector = Map_GetSector(map, sectorIndex);
    float ceilingY = sector->ceilingy;
    float floorY = sector->floory;

    glPushMatrix();

    // Set translation offset, normal and color for the whole polygon
    if (floor)
    {
        glTranslatef(0.0f, floorY, 0.0f);
        float color = BrightnessOffsetToColor(sector->floorshade);
        RectF uv = SetPicnum_GetUVoffset(sector->floorpicnum);
        glNormal3f(floorNormal[0], floorNormal[1], floorNormal[2]);
        glColor3f(color, color, color);
    }
    else
    {
        glTranslatef(0.0f, ceilingY, 0.0f);
        float color = BrightnessOffsetToColor(sector->ceilingshade);
        RectF uv = SetPicnum_GetUVoffset(sector->ceilingpicnum);
        glNormal3f(ceilingNormal[0], ceilingNormal[1], ceilingNormal[2]);
        glColor3f(color, color, color);
    }

    if (!floor)
    {
        // Cull the ceiling faces the other way around
        glCullFace(GL_FRONT);
    }
        Tesselator_BufferIndices indices = floorStartIndices[sectorIndex];

        GLsizei count = indices.indexCount;
        //Log_InfoF("Draw %d count vertices\n", count);
        // Set element pointers to floor buffer
        ActivateFloorBuffer(0);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, &floorIndexBuffer[indices.indexIndex]);

        // Reset face culling
        if (!floor)
        {
            glCullFace(GL_BACK);
        }
    glPopMatrix();
}
    /*
    else  // No GLUTESS in use, only convex sectors without islands :(
    {
        if (floor)
        {
            BeginPolygon( vec3New(floorNormal[0], floorNormal[1], floorNormal[2] ), SetPicnum_GetUVoffset(sector->floorpicnum), BrightnessOffsetToColor(sector->floorshade));
        }
        else
        {
            BeginPolygon(vec3New(ceilingNormal[0], ceilingNormal[1], ceilingNormal[2]), SetPicnum_GetUVoffset(sector->ceilingpicnum), BrightnessOffsetToColor(sector->ceilingshade));
        }

        ActivateVertexBuffer();

        int outerWallStartPoint = sector->wallptr;

        vec2 middle = vec2New(
            sector->minXZPoint.x + sector->sizeXZ.x/2,
            sector->minXZPoint.y + sector->sizeXZ.y/2 );
        vec2 middleUV = CalculateFloorOrCeilingUV(sector, middle.x, middle.y);

        bool outerWallDone = false;
        // Take pair of vertices and make triangle with center of sector
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w1 = Map_GetWallInSectorPtr(map, sector, wi);
            s16 next;
            if (w1->point2 == outerWallStartPoint)
            {
                // Close the loop
                next = 0;
                outerWallDone = true;
            }
            else
            {
                next = (wi+1) %sector->wallnum;
            }
            Wall* w2 = Map_GetWallInSectorPtr(map, sector, next);
            vec2 uv1 = CalculateFloorOrCeilingUV(sector, w1->x, w1->z);
            vec2 uv2 = CalculateFloorOrCeilingUV(sector, w2->x, w2->z);
            if (floor)
            {
                BufferVertex( w2->x, floorY, w2->z, uv2.x, uv2.y);
                BufferVertex( w1->x, floorY, w1->z, uv1.x, uv1.y);
                BufferVertex( middle.x, floorY, middle.y, middleUV.x, middleUV.y);
            }
            else
            {
                BufferVertex( w1->x, ceilingY, w1->z, uv1.x, uv1.y);
                BufferVertex( w2->x, ceilingY, w2->z, uv2.x, uv2.y);
                BufferVertex( middle.x, ceilingY, middle.y, middleUV.x, middleUV.y);
            }
            if (outerWallDone)
            {
                break;
            }
        }
        EndPolygon();
    }
    glPopMatrix();
    */

void OpenGLRender_DrawSprite(vec3 position, float width, float height, float spriteAngle, float playerAngle, SpriteAlignment alignment, SpritePivot pivot, s16 picnum, s8 brightnessOffset)
{

    static const float pushOut = 1.0f; // In Duke units: 1024 is one meter
	if (alignment == Sprite_FACE)
	{
		spriteAngle = playerAngle + Deg2Rad(180);
	}

	vec3 spriteForward = Vec3XYZRotateY(WORLD_FORWARD, spriteAngle);
	vec3 spriteRight = Vec3XYZRotateY(spriteForward, -M_PI_2);

    // Sprite right is on the left side when looking
    // from the player
	vec3 toRight = vec3Multiply(spriteRight, width/2);

    // These are from player's point of view
    vec3 bottomRight, bottomLeft, topLeft, topRight;
	if (alignment == Sprite_FLOOR)
    {
		// Raise up to avoid Z fighting
		position.y += pushOut;

		// Calculate four carpet corners
		bottomLeft = vec3Add(position, vec3Add( vec3Multiply(spriteRight, -width/2), vec3Multiply(spriteForward, -width/2)));
		bottomRight = vec3Add(position, vec3Add( vec3Multiply(spriteRight, width/2), vec3Multiply(spriteForward, -width/2)));
		topLeft = vec3Add(position, vec3Add( vec3Multiply(spriteRight, -width/2), vec3Multiply(spriteForward, width/2)));
		topRight = vec3Add(position, vec3Add( vec3Multiply(spriteRight, width/2), vec3Multiply(spriteForward, width/2)));
    }
    else
    {
        if (alignment == Sprite_WALL)
        {
            // Push out of wall to avoid Z fight
            position = vec3Add(position, vec3Multiply(spriteForward, pushOut));
        }
        if (pivot == Sprite_PivotCenter)
        {
            bottomRight = vec3Subtract(position, toRight);
            bottomRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, -height / 2));
            bottomLeft = vec3Add(position, toRight);
            bottomLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, -height / 2));
            topLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, height));
            topRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, height));
        }
        else
        {
            bottomRight = vec3Subtract(position, toRight);
            bottomLeft = vec3Add(position, toRight);
            topLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, height));
            topRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, height));
        }
    }

    ActivateVertexBuffer();

	BeginVertexBufferPolygon(spriteForward, SetPicnum_GetUVoffset(picnum), BrightnessOffsetToColor(brightnessOffset));

	BufferVertex(bottomLeft.x, bottomLeft.y, bottomLeft.z, 0.0f, 0.0f);
	BufferVertex(bottomRight.x, bottomRight.y, bottomRight.z, 1.0f, 0.0f);
	BufferVertex(topRight.x, topRight.y, topRight.z, 1.0f, 1.0f);
	BufferVertex(topLeft.x, topLeft.y, topLeft.z, 0.0f, 1.0f);

    EndVertexBufferPolygon();
}

void OpenGLRender_AnimateSprites()
{
    animationTimer += mgdl_GetDeltaTime();
    if (animationTimer > animationRate)
    {
        animationFrame++;
        animationTimer = 0.0f;
    }
}

void OpenGLRender_DrawDot(vec2 point, float size, DefaultColor color)
{
    OpenGLRender_SetColor(color);

    glVertex2i(point.x,point.y - size);
    glVertex2i(point.x + size,point.y);

    glVertex2i(point.x + size,point.y);
    glVertex2i(point.x ,point.y + size);

    glVertex2i(point.x ,point.y + size);
    glVertex2i(point.x - size,point.y);

    glVertex2i(point.x - size,point.y);
    glVertex2i(point.x,point.y - size);
}

void SetDark(bool newDark)
{
    dark = newDark;
}


void OpenGLRender_StartCountingFloorBufferSize(s16 sectorAmount, s16 wallAmount)
{
    // Reserve all memory
    if (floorStartIndices == nullptr)
    {
        floorStartIndices = (Tesselator_BufferIndices*)mgdl_AllocateGraphicsMemory(sectorAmount * sizeof(Tesselator_BufferIndices));
    }
    else if (sectorAmount > bufferedSectorAmount)
    {
        floorStartIndices = (Tesselator_BufferIndices*)realloc(floorStartIndices, sectorAmount * sizeof(Tesselator_BufferIndices));
    }
    bufferedSectorAmount = sectorAmount;

    if (floorBuffer == nullptr)
    {
        floorBuffer = (GLfloat*)mgdl_AllocateGraphicsMemory(wallAmount * FLOOR_BUFFER_VERTEX_SIZE * sizeof(GLfloat));
    }
    else if (wallAmount > floorBufferSizeVertices)
    {
        floorBuffer = (GLfloat*)realloc(floorBuffer, wallAmount * FLOOR_BUFFER_VERTEX_SIZE * sizeof(GLfloat));
    }

    static const int wallsToIndicesMultiplier = 8;
    if (floorIndexBuffer == nullptr)
    {
        // DANGER Try to allocate enough : multiply wall amount by some number
        floorIndexBuffer = (GLushort*)mgdl_AllocateGraphicsMemory(wallAmount * wallsToIndicesMultiplier * sizeof(GLushort));
    }
    else if (wallAmount > floorBufferSizeVertices)
    {
        floorIndexBuffer = (GLushort*)realloc(floorIndexBuffer, wallAmount * wallsToIndicesMultiplier * sizeof(GLushort));
    }
    floorIndexBufferSize = wallAmount * wallsToIndicesMultiplier;
    floorBufferSizeVertices = wallAmount;

    // Start tesselator and send buffer adresses

    Tesselator_Init();
    Tesselator_SetBuffers(floorBuffer, floorBufferSizeVertices, floorIndexBuffer, floorIndexBufferSize);
}

void OpenGLRender_TesselateFloor(DukeMap* map, u16 sectorIndex)
{
    Sector* sector = Map_GetSector(map, sectorIndex);
    // Set translation offset, normal and color for the whole polygon
    RectF uvOffset;
    Tesselator_BufferIndices indicesBefore;
    uvOffset = SetPicnum_GetUVoffset(sector->floorpicnum);
    indicesBefore = Tesselator_BeginPolygon(floorNormal, uvOffset);

        Tesselator_BeginContour();
        // This is where the current contour started
        int contourStartPoint = sector->wallptr + sector->wallnum -1;

        Wall* startWall = Map_GetWall(map, contourStartPoint);
        int contourEndPoint = startWall->point2;
        /* Because Mapster saves points in clockwise order, but we render
        in counter-clockwise, we need to save the point2 of this vertex
        that is the last point of this contour

        Square sector:
        0 > 1 > 2 > 3 > 0

        Square sector with a square island:
        0 > 1 > 2 > 3 > 0   : Outer wall
        4 > 5 > 6 > 7 > 4   : Island

        Our rendering order is
        7, 6, 5, 4, 3, 2, 1, 0

        When starting from point 7, the value of point2 is 4
        Store that to contourEndPoint
        When we come to point 4, we know that the contour is complete
        and a new one should begin.
        If the first point of new contour is > sector's wallptr there is still more
        islands or the outside wall.
        If the contourEndPoint is greater than sector's wallptr, we know that the sector is complete and
        this was the last contour.
        */
        //Log_InfoF("Tesselating sector %d start %d/%d\n", sector->lotag, startingWall, sector->wallnum);

        // Keep track of global wall index
        int pointIndex = contourStartPoint;
        GLfloat vertex[3];
        vec2 calculatedUV;
        GLfloat uv[2];
        for (s16 wi = sector->wallnum-1; wi >= 0; wi--)
        {
            Wall* w = Map_GetWallInSectorPtr(map, sector, wi);

            vertex[0] = w->x;
            vertex[1] = 0.0f;
            vertex[2] = w->z;
            calculatedUV = CalculateFloorOrCeilingUV(sector, w->x, w->z);
            uv[0] = calculatedUV.x;
            uv[1] = calculatedUV.y;

            Tesselator_AddVertexToPoly(vertex, uv);

            if (pointIndex == contourEndPoint)
            {
                Tesselator_EndContour();
                if (wi > 0 && contourEndPoint > sector->wallptr)
                {
                    Tesselator_BeginContour();
                    Wall* nextWall = Map_GetWallInSectorPtr(map, sector, (wi - 1));
                    contourEndPoint = nextWall->point2;
                }

            }
            pointIndex--;
        }
        Tesselator_BufferIndices indicesAfter = Tesselator_EndPolygon();

    floorStartIndices[sectorIndex].indexIndex = indicesBefore.indexIndex;
    u16 count = (indicesAfter.indexIndex - indicesBefore.indexIndex);
    floorStartIndices[sectorIndex].indexCount = count;
    //Log_InfoF("Sector %d: before %d After %d Count: %d\n", sectorIndex, indicesBefore.indexIndex, indicesAfter.indexIndex, count);
    // Set indices in our buffers
    floorStartIndices[sectorIndex].vertexIndex = indicesBefore.vertexIndex;
    u16 vertexCount = (indicesAfter.vertexIndex - indicesBefore.vertexIndex);
    floorStartIndices[sectorIndex].vertexCount = vertexCount;
}

void OpenGLRender_StopCountingFloorBufferSize()
{
    Tesselator_BufferIndices lastIndices = floorStartIndices[bufferedSectorAmount-1];
    // Allocate the needed amount of memory
    u16 lastIndex = lastIndices.indexIndex + lastIndices.indexCount;
    if (lastIndex < floorIndexBufferSize)
    {
        floorIndexBuffer = (GLushort*)realloc(floorIndexBuffer, lastIndex * sizeof(GLushort));
        floorIndexBufferSize = lastIndex;
    }
    //Log_InfoF("Tesselator created %d indices in total\n", floorIndexBufferSize);
    Tesselator_Deinit();

    /*
    Log_Info("Floor Vertex buffer:\n");
    for (u32 v = 0; v < floorBufferSizeVertices; v++)
    {
        int i = v * FLOOR_BUFFER_VERTEX_SIZE;
        Log_InfoF("V %d: (%.1f, %.1f, %.1f)\n", v, floorBuffer[i+0], floorBuffer[i+1], floorBuffer[i+2]);
    }
    Log_Info("Floor Index buffer to triangles:\n");
    for (u32 v = 0; v < floorIndexBufferSize; v += 3)
    {
        Log_InfoF("F %d: (%d, %d, %d)\n", v/3, floorIndexBuffer[v+0], floorIndexBuffer[v+1], floorIndexBuffer[v+2]);
        Log_InfoF("       %d, %d, %d )\n",v+0, v+1, v+2);
    }
    */
}
void OpenGLRender_StartDrawingFloorsFromBuffer()
{
    mgdl_CacheFlushRange(floorBuffer, floorBufferSizeVertices * FLOOR_BUFFER_VERTEX_SIZE * sizeof(GLfloat));
    mgdl_CacheFlushRange(floorIndexBuffer, floorIndexBufferSize * sizeof(GLushort));
}

// //////////////////////////////
// OBJ EXPORT FUNCTIONS NOTE TODO DANGER TEST WARNING BUG
// //////////////////////////////

void OpenGLRender_StartObjExport(DukeMap* map, const char* filename, RenderSettingsOpenGL* settings)
{
    ObjExport_Start(filename, map->mapfile, map->sectorAmount, settings->scale);
}
void OpenGLRender_StartFillingWallBuffer(DukeMap* map)
{
    u32 drawnWalls = 0;
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Sector* sector = Map_GetSector(map, si);
        for(int wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
            if (w->nextsector >= 0)
            {
                // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
                Sector* neighbor = Map_GetSector(map, w->nextsector);
                int n_floorY = neighbor->floory;
                int n_ceilingY = neighbor->ceilingy;

                // if this floor height is less than adjacent: Greate wall in between: goes up
                if (sector->floory < n_floorY)
                {
                    drawnWalls += 1;
                }

                // Ceiling:
                // If this ceiling is higher than adjacent: Greate wall in between: goes down
                if (sector->ceilingy > n_ceilingY)
                {
                    drawnWalls += 1;
                }
            }
            else
            {
                drawnWalls += 1;
            }
        }
    }
    // These are not drawn, just written to obj and newer on Wii
    // Each wall has at least 4 vertices,
    // some have 12 : floor bit, wall, ceiling bit
    wallBufferSizeVertices = drawnWalls * 4;
    wallBuffer = (GLfloat*)mgdl_AllocateGraphicsMemory(wallBufferSizeVertices * WALL_BUFFER_VERTEX_SIZE * sizeof(GLfloat));

    wallIndexBufferSize = drawnWalls * 6;
    wallIndexBuffer = (GLushort*)mgdl_AllocateGraphicsMemory(wallIndexBufferSize * sizeof(GLushort));

    wallIndexBufferIndex = 0;
    wallBufferVertexIndex = 0;
}

void BufferWallVertex(float x, float y, float z)
{
    int v = wallBufferVertexIndex * WALL_BUFFER_VERTEX_SIZE;
    wallBuffer[v + 0] = x;
    wallBuffer[v + 1] = y;
    wallBuffer[v + 2] = z;
    wallBufferVertexIndex += 1;

}
void BufferQuad(vec2 start, vec2 end, const vec2 normalXZ, float floorY, float ceilingY, u16 quad)
{

    BufferWallVertex(start.x, floorY, start.y);
    BufferWallVertex(end.x, floorY, end.y);
    BufferWallVertex(end.x, ceilingY, end.y);
    BufferWallVertex(start.x, ceilingY, start.y);

    wallIndexBuffer[wallIndexBufferIndex+ 0] = quad * 4 + 0;
    wallIndexBuffer[wallIndexBufferIndex+ 1] = quad * 4 + 1;
    wallIndexBuffer[wallIndexBufferIndex+ 2] = quad * 4 + 2;

    wallIndexBuffer[wallIndexBufferIndex+ 3] = quad * 4 + 2;
    wallIndexBuffer[wallIndexBufferIndex+ 4] = quad * 4 + 3;
    wallIndexBuffer[wallIndexBufferIndex+ 5] = quad * 4 + 0;
    wallIndexBufferIndex += 6;
}
void OpenGLRender_BufferWalls(DukeMap* map)
{
    u16 quadCounter = 0;
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Sector* sector = Map_GetSector(map, si);
        for(int wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
            vec2 start = vec2New(w->x, w->z);
            Wall* wend = Map_GetWallEnd(map, w);
            vec2 end =  vec2New(wend->x, wend->z);
            vec2 normalXZ = Map_GetWallNormal(map, w);
            if (w->nextsector >= 0)
            {
                // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
                Sector* neighbor = Map_GetSector(map, w->nextsector);
                int n_floorY = neighbor->floory;
                int n_ceilingY = neighbor->ceilingy;

                // if this floor height is less than adjacent: Greate wall in between: goes up
                if (sector->floory < n_floorY)
                {
                    BufferQuad(start, end, normalXZ, sector->floory, n_floorY, quadCounter);
                    quadCounter += 1;
                }

                // Ceiling:
                // If this ceiling is higher than adjacent: Greate wall in between: goes down
                if (sector->ceilingy > n_ceilingY)
                {
                    BufferQuad(start, end, normalXZ, n_ceilingY, sector->ceilingy, quadCounter);
                    quadCounter += 1;
                }
            }
            else
            {
                // TODO Masked walls
                // Draw the wall
                BufferQuad(start, end, normalXZ, sector->floory, sector->ceilingy, quadCounter);
                quadCounter += 1;
            }
        }
    }
}

void OpenGLRender_WriteToObj(DukeMap* map, const char* filename, RenderSettingsOpenGL* settings)
{
    OpenGLRender_StartObjExport(map, filename, settings);
    OpenGLRender_StartFillingWallBuffer(map);
    OpenGLRender_BufferWalls(map);

    // VERTICES
    // This will be buffer 0
    u16 wallBufferIndex = 0;
    s32 noYOffset = 0;
    ObjExport_WriteVertices(wallBuffer, wallBufferSizeVertices, WALL_BUFFER_VERTEX_SIZE,
                            noYOffset,
                            wallBufferIndex, mgdl_BufferPrintf("%s", "Wall buffer"));

    // Buffers 1 2, 3 4, 5 6 : Two buffers per sector. First is floor, second is ceiling
    // Write all floors in one big buffer
    u16 firstFloorBuffer = 1;
    u16 firstCeilingBuffer = 2;
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Sector* sector = Map_GetSector(map, si);
        Tesselator_BufferIndices indices = floorStartIndices[si];

        ObjExport_WriteVertices(&floorBuffer[indices.vertexIndex * FLOOR_BUFFER_VERTEX_SIZE], indices.vertexCount, FLOOR_BUFFER_VERTEX_SIZE,
                                sector->floory, firstFloorBuffer,
                                mgdl_BufferPrintf("%s", "Floor buffer"));
    }
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Sector* sector = Map_GetSector(map, si);
        Tesselator_BufferIndices indices = floorStartIndices[si];

        ObjExport_WriteVertices(&floorBuffer[indices.vertexIndex * FLOOR_BUFFER_VERTEX_SIZE], indices.vertexCount, FLOOR_BUFFER_VERTEX_SIZE,
                                sector->ceilingy, firstCeilingBuffer,
                                mgdl_BufferPrintf("%s", "Ceiling buffer"));
    }

//        ObjExport_WriteVertices(&floorBuffer[indices.vertexIndex * FLOOR_BUFFER_VERTEX_SIZE], indices.vertexCount, FLOOR_BUFFER_VERTEX_SIZE, sector->ceilingy, mgdl_BufferPrintf("Sector %d ceiling", si));
    // FACES
    u16 wallVertexBuffer = 0;
    ObjExport_WriteFaces(wallIndexBuffer, wallIndexBufferSize, wallVertexBuffer, Wind_CCW, mgdl_BufferPrintf("%s", "Wall faces"));
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Tesselator_BufferIndices indices = floorStartIndices[si];
        // These faces refer to earlier written floor and ceiling buffers
        ObjExport_WriteFaces(&floorIndexBuffer[indices.indexIndex], indices.indexCount,
                             firstFloorBuffer, Wind_CCW,
                             mgdl_BufferPrintf("Sector %d floor : refers to vb %d", si, firstFloorBuffer));
    }

    for (int si = 0; si < map->sectorAmount; si++)
    {
        Tesselator_BufferIndices indices = floorStartIndices[si];
        ObjExport_WriteFaces(&floorIndexBuffer[indices.indexIndex], indices.indexCount,
                             firstCeilingBuffer, Wind_CW,
                             mgdl_BufferPrintf("Sector %d ceiling : refers to vb %d", si, firstCeilingBuffer));
    }

    ObjExport_Stop();
}

