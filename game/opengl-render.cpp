
#include "opengl-render.h"
#include <mgdl.h>
#include <mgdl/mgdl-cache.h>
#include <mgdl/mgdl-draw2d.h>
#include "dukemap.h"
#include "build-render.h"
#include "dukemath.h"
#ifdef MGDL_PLATFORM_WINDOWS
#define _GLUfuncptr void(*)()
#endif

// OpenGL

// Arrays for storing sprite pointers and matching picnums to Sprites
#define RENDERER_SPRITE_ARRAY_SIZE 128
#define RENDERER_PICNUM_TO_SPRITE_ARRAY_SIZE 2048
static sizetype nextFreeSpriteSlot = 0;
static Sprite** spritePtrArray = nullptr;
static u16* picnumToSpriteArray = nullptr;

Texture* checkers;

GLUtesselator* tesselator = nullptr;
bool tesselationActive = true;
static RectF uvs;
int animationFrame = 0;
float animationRate = 0.05f;
float animationTimer = 0;

GLfloat floorNormal[3];
GLfloat ceilingNormal[3];

// TODO Alternative textures? Multiple Textures per sprite
#define RENDERER_PICNUM_TO_SPRITE_ARRAY_DARK_SIZE 2048
static u16* picnumToSpriteArray_DARK;
static bool dark = false;

// Buffer for drawing vertices of the walls and sprites
// 3 position + 2 texture coordinates
#define FULL_VERTEX_SIZE_FLOATS (3 + 2)
#define VERTEX_BUFFER_SIZE_VERTICES 64 * 3 // This will always contain triangles
#define VERTEX_BUFFER_SIZE_BYTES (FULL_VERTEX_SIZE_FLOATS * sizeof(float) * VERTEX_BUFFER_SIZE_VERTICES)
static GLfloat* vertexBuffer = nullptr;
static int vertexBufferIndexVertices = 0;

static RectF polygonUVLimits;
static RectF zeroOffset;

static bool UseVertexBufferForTesselation = false;

void OpenGLRender_StartDrawingPolygons()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 5, &vertexBuffer[0]);
    glTexCoordPointer(2, GL_FLOAT, sizeof(float) * 5, &vertexBuffer[3]);

    glEnable(GL_TEXTURE_2D);
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

static void BeginPolygon(const vec3 normal, const RectF uvLimits, const float brightness)
{
    polygonUVLimits = uvLimits;
    glNormal3f(normal.x, normal.y, normal.z);
    glColor3f(brightness, brightness, brightness);
    vertexBufferIndexVertices = 0;
}

static void BufferVertex(const float x, const float y, const float z, const float u, const float v)
{
    GLfloat* vertex = &vertexBuffer[vertexBufferIndexVertices * FULL_VERTEX_SIZE_FLOATS];
    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = z;

    vertex[3] = polygonUVLimits.x + u * polygonUVLimits.w;
    vertex[4] = polygonUVLimits.y + v * polygonUVLimits.h;

    vertexBufferIndexVertices += 1;

    // DANGER what if buffer becomes full?
    if (vertexBufferIndexVertices >= VERTEX_BUFFER_SIZE_VERTICES)
    {
        // Flush all written vertices
        mgdl_CacheFlushRange(vertexBuffer, vertexBufferIndexVertices * FULL_VERTEX_SIZE_FLOATS * sizeof(float));
        glDrawArrays(GL_TRIANGLES, 0, vertexBufferIndexVertices);
        vertexBufferIndexVertices = 0;
    }
}
static void BufferVertexV(const vec3 position, const vec2 textureCoord)
{
    BufferVertex(position.x, position.y, position.z, textureCoord.x, textureCoord.y);
}

static void EndPolygon()
{
    // Flush all written vertices
    mgdl_CacheFlushRange(vertexBuffer, vertexBufferIndexVertices * FULL_VERTEX_SIZE_FLOATS * sizeof(float));
    glDrawArrays(GL_TRIANGLES, 0, vertexBufferIndexVertices);
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


// TESSELATION CALLBACKS
// /////////////////////

// Ring buffer for tesselation input
// This buffer needs to hold all the vertices of a sector floor or ceiling
#define TESSELATION_BUFFER_SIZE_DOUBLES (3*128) // Divisible by three for the ring buffering to work; 3 doubles per vertex
#define TESSELATION_BUFFER_SIZE_BYTES (TESSELATION_BUFFER_SIZE_DOUBLES * sizeof(double))
static GLdouble* tesselationBuffer = nullptr;
int tesselationBufferIndexDoubles = 0;

// Ring buffer for tesselation combine
// This is needed if two vertices are identical, but hopefully it is not needed
#define COMBINE_BUFFER_SIZE_DOUBLES (3*9) // Divisible by three for the ring buffering to work; 3 doubles per vertex
#define COMBINE_BUFFER_SIZE_BYTES (COMBINE_BUFFER_SIZE_DOUBLES * sizeof(double))
static GLdouble* combineRingBuffer = nullptr;
int CombineBufferIndexDoubles = 0;


#ifndef CALLBACK
#define CALLBACK
#endif

// NOTE not used: the code calls StartPolygon and EndPolygon
void CALLBACK tessBegin(GLenum which)
{
    if (UseVertexBufferForTesselation == false)
    {
        glBegin(which);
    }
    //Log_InfoF("Tesselation start mode: %s \n", which == GL_TRIANGLES ? "Triangles" : "Not triangles");
}

// This puts a new vertex into the buffer
void CALLBACK tessVertex(GLvoid* vertex, void* SectorPtr)
{
    const Sector* s = (Sector*)SectorPtr;
    const GLdouble* coordinates = (GLdouble*)vertex;

    //Log_InfoF("Tesselation vertex %.2f, %.2f\n", pointer[0], pointer[2]);
    // TODO texture coordinates and colors
    float xrange = s->sizeXZ.x;
    float zrange = s->sizeXZ.y;
    float xdiff = coordinates[0] - s->minXZPoint.x;
    float zdiff = coordinates[2] - s->minXZPoint.y;
    float tx = xdiff/xrange * s->maxTexCoord.x;
    float tz = zdiff/zrange * s->maxTexCoord.y;
    //Log_InfoF("Tesselation tex coord %.2f, %.2f\n", tx, ty);
    if (UseVertexBufferForTesselation)
    {
        BufferVertex(
            (GLfloat)coordinates[0], (GLfloat)coordinates[1], (GLfloat)coordinates[2],
            tx, tz);
    }
    else
    {
        glTexCoord2f(tx,tz);
        glVertex3f((GLfloat)coordinates[0], (GLfloat)coordinates[1], (GLfloat)coordinates[2]);
    }
}
void CALLBACK tessCombine(GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4], GLdouble **dataOut)
{
    //Log_InfoF("Tesselation combine vertex: %.2f, %.2f\n", coords[0], coords[2]);
    if (CombineBufferIndexDoubles + 6 >= COMBINE_BUFFER_SIZE_DOUBLES)
    {
        CombineBufferIndexDoubles = 0;
    }
    // Reads 6 doubles
    GLdouble* vertex = &combineRingBuffer[CombineBufferIndexDoubles];

    // Coordinates of the combined vertex
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    vertex[3] = 0.0f;
    vertex[4] = 0.0f;
    vertex[5] = 0.0f;
    /*  This causes crashes so don't do it
    for (int i = 3; i < 6; i++)
    {
        vertex[i] = weight[0] * vertex_data[0][i] +
                    weight[1] * vertex_data[1][i] +
                    weight[2] * vertex_data[2][i] +
                    weight[3] * vertex_data[3][i];
    }
    */
    *dataOut = vertex;
    CombineBufferIndexDoubles = (CombineBufferIndexDoubles + 6) % COMBINE_BUFFER_SIZE_DOUBLES;
}

// NOTE not used
void CALLBACK tessEnd(void)
{
    //Log_Info("Tesselation end\n");
    if (UseVertexBufferForTesselation == false)
    {
        glEnd();
    }
}
void CALLBACK tessError(GLenum errorCode)
{
    const GLubyte* str;
    str = gluErrorString(errorCode);
    Log_ErrorF("Tesselation error: %s\n", str);
    tesselationActive = false;
}

void CALLBACK tessEdgeFlag(GLboolean flag)
{
#ifndef GEKKO
   glEdgeFlag(flag);
#endif
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

    checkers = Texture_GenerateCheckerBoard();
    if (tesselator == nullptr)
    {
        tesselator = gluNewTess();
        mgdl_assert_print(tesselator != nullptr, "No Glut tesselator!");

        gluTessCallback(tesselator, GLU_TESS_BEGIN, (_GLUfuncptr)tessBegin);
        gluTessCallback(tesselator, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)tessVertex);
        gluTessCallback(tesselator, GLU_TESS_END, (_GLUfuncptr)tessEnd);
        gluTessCallback(tesselator, GLU_TESS_ERROR, (_GLUfuncptr)tessError);
        gluTessCallback(tesselator, GLU_TESS_EDGE_FLAG, (_GLUfuncptr)tessEdgeFlag); // this makes tess only submit triangles
        gluTessCallback(tesselator, GLU_TESS_COMBINE, (_GLUfuncptr)tessCombine);

        floorNormal[0] = 0;
        floorNormal[1] = 1;
        floorNormal[2] = 0;
        ceilingNormal[0] = 0;
        ceilingNormal[1] = -1;
        ceilingNormal[2] = 0;
    }

    if (tesselationBuffer == nullptr)
    {
        tesselationBuffer = (GLdouble*)mgdl_AllocateGraphicsMemory(TESSELATION_BUFFER_SIZE_BYTES);
    }
    if (combineRingBuffer == nullptr)
    {
        combineRingBuffer = (GLdouble*)mgdl_AllocateGraphicsMemory(COMBINE_BUFFER_SIZE_BYTES);
    }
    if (vertexBuffer == nullptr)
    {
        vertexBuffer = (GLfloat*)mgdl_AllocateGraphicsMemory(VERTEX_BUFFER_SIZE_BYTES);
    }
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



    BeginPolygon(normal, SetPicnum_GetUVoffset(picnum), BrightnessOffsetToColor(brightnessOffset));


        // Build Triangles for wall
        BufferVertex(start.x, floorY, start.y, tex_x1, tex_bottom); // 0
        BufferVertex(end.x, floorY, end.y, tex_x2, tex_bottom);
        BufferVertex(end.x, ceilingY, end.y, tex_x2, tex_top);

        BufferVertex(end.x, ceilingY, end.y, tex_x2, tex_top);
        BufferVertex(start.x, ceilingY, start.y, tex_x1, tex_top);
        BufferVertex(start.x, floorY, start.y, tex_x1, tex_bottom);

    EndPolygon();

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

void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, Sector* sector, bool floor)
{
    float ceilingY = sector->ceilingy;
    float floorY = sector->floory;

    glPushMatrix();
        // Set floor level to 0.0f
        if (floor)
        {
            glTranslatef(0.0f, floorY, 0.0f);
            gluTessNormal(tesselator, floorNormal[0], floorNormal[1], floorNormal[2]); // All points on XZ plane
            BeginPolygon( vec3New(floorNormal[0], floorNormal[1], floorNormal[2] ), SetPicnum_GetUVoffset(sector->floorpicnum), BrightnessOffsetToColor(sector->floorshade));
        }
        else
        {
            glTranslatef(0.0f, ceilingY, 0.0f);
            gluTessNormal(tesselator, ceilingNormal[0], ceilingNormal[1], ceilingNormal[2]); // All points on XZ plane

            BeginPolygon(vec3New(ceilingNormal[0], ceilingNormal[1], ceilingNormal[2]), SetPicnum_GetUVoffset(sector->ceilingpicnum), BrightnessOffsetToColor(sector->ceilingshade));
        }

        /*
        if (sector->wallnum <= 4)
        {
            // its just a quad
            if (floor)
            {
                for (s16 wi = sector->wallnum-1; wi >= 0; wi--)
                {
                    Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
                }
            }
            else
            {
                for (s16 wi = 0; wi < sector->wallnum; wi++)
                {
                    Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
                }

            }
        }
        else
        */
        {

        //Log_InfoF("Tesselating sector:  extra %d\n", sector->extra);

            // tesselation
            tesselationBufferIndexDoubles = 0; // Start from beginning
            gluTessBeginPolygon(tesselator, sector);

            gluTessBeginContour(tesselator);
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
            for (s16 wi = sector->wallnum-1; wi >= 0; wi--)
            {
                Wall* w = Map_GetWallInSectorPtr(map, sector, wi);

                // Tesselation
                // NOTE DANGER Must be counter clockwise
                //Log_InfoF("Tesselation vertex sent %d:: %.2f, %.2f\n", wi, w->glutVertices[0], w->glutVertices[2]);
                // TODO Send normal too, but maybe not with every vertex?
                tesselationBuffer[tesselationBufferIndexDoubles + 0] = w->x;
                tesselationBuffer[tesselationBufferIndexDoubles + 1] = 0;
                tesselationBuffer[tesselationBufferIndexDoubles + 2] = w->z;

                gluTessVertex(tesselator,
                              &tesselationBuffer[tesselationBufferIndexDoubles], &tesselationBuffer[tesselationBufferIndexDoubles]);

                tesselationBufferIndexDoubles = (tesselationBufferIndexDoubles + 3) % TESSELATION_BUFFER_SIZE_DOUBLES;

                if (pointIndex == contourEndPoint)
                {
                    gluTessEndContour(tesselator);
                    if (wi > 0 && contourEndPoint > sector->wallptr)
                    {
                        gluTessBeginContour(tesselator);
                        Wall* nextWall = Map_GetWallInSectorPtr(map, sector, (wi - 1));
                        contourEndPoint = nextWall->point2;
                    }

                }
                pointIndex--;
            }
            // Tesselation code starts drawing vertices after glutTessEndPolygon
            // flush the vertices before that
            mgdl_CacheFlushRange(tesselationBuffer, TESSELATION_BUFFER_SIZE_BYTES);
            gluTessEndPolygon(tesselator);
        }

        EndPolygon();
    glPopMatrix();
}


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

	BeginPolygon(spriteForward, SetPicnum_GetUVoffset(picnum), BrightnessOffsetToColor(brightnessOffset));

	BufferVertex(bottomLeft.x, bottomLeft.y, bottomLeft.z, 0.0f, 0.0f);
	BufferVertex(bottomRight.x, bottomRight.y, bottomRight.z, 1.0f, 0.0f);
	BufferVertex(topRight.x, topRight.y, topRight.z, 1.0f, 1.0f);

	BufferVertex(topRight.x, topRight.y, topRight.z, 1.0f, 1.0f);
	BufferVertex(topLeft.x, topLeft.y, topLeft.z, 0.0f, 1.0f);
	BufferVertex(bottomLeft.x, bottomLeft.y, bottomLeft.z, 0.0f, 0.0f);

    EndPolygon();
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
