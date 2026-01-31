
#include "opengl-render.h"
#include <mgdl.h>
#include "dukemap.h"
#include "build-render.h"
#include "dukemath.h"
#ifdef MGDL_PLATFORM_WINDOWS
#define _GLUfuncptr void(*)()
#endif

// OpenGL
Texture* checkers;
Texture* bulletTexture;
Texture* treasureTexture;
Texture* playerTexture;
GLUtesselator* tesselator = nullptr;
bool tesselationActive = true;

Texture* OpenGLRender_GetTexture(s16 picnum)
{
    if (picnum == PICNUM_BULLET)
    {
        return bulletTexture;
    }
    else if (picnum == PICNUM_TREASURE)
    {
        return treasureTexture;
    }
    else if (picnum == PICNUM_PLAYER)
    {
        return playerTexture;
    }
    return checkers;
}

void OpenGLRender_SetColor(DefaultColor oc)
{
    Color4f* c = Color_GetDefaultColor(oc);
    glColor4fv(&c->red);
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

// TESSELATION CALLBACKS
// /////////////////////

// Ring buffer for tesselation
#define VERTEX_BUFFER_SIZE_DOUBLES (3*64) // Divisible by three for the ring buffering to work; 3 doubles per vertex
#define VERTEX_BUFFER_SIZE_BYTES (VERTEX_BUFFER_SIZE_DOUBLES * sizeof(double))
static GLdouble* vertexRingBuffer = nullptr;
int VertexBufferIndexDoubles = 0;

#ifndef CALLBACK
#define CALLBACK
#endif

void CALLBACK tessBegin(GLenum which)
{
    //Log_InfoF("Tesselation start mode: %s \n", which == GL_TRIANGLES ? "Triangles" : "Not triangles");
    glBegin(which);
}
void CALLBACK tessVertex(GLvoid* vertex, void* SectorPtr)
{
    const Sector* s = (Sector*)SectorPtr;
    const GLdouble* pointer;
    pointer = (GLdouble*)vertex;
    //Log_InfoF("Tesselation vertex %.2f, %.2f\n", pointer[0], pointer[2]);
    // TODO texture coordinates and colors
    float xrange = s->sizeXZ.x;
    float zrange = s->sizeXZ.y;
    float xdiff = pointer[0] - s->minXZPoint.x;
    float zdiff = pointer[2] - s->minXZPoint.y;
    float tx = xdiff/xrange * s->maxTexCoord.x;
    float tz = zdiff/zrange * s->maxTexCoord.y;
    //Log_InfoF("Tesselation tex coord %.2f, %.2f\n", tx, ty);
    glTexCoord2f(tx,tz);
    glVertex3dv(pointer);

}
/* NOTE This is not used, points are far apart
void CALLBACK tessCombine(GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4], GLdouble **dataOut)
{
    Log_InfoF("Tesselation combine vertex: %.2f, %.2f\n", coords[0], coords[2]);
    if (combineBufferIndex + 6 >= COMBINE_BUFFER_SIZE)
    {
        combineBufferIndex = 0;
    }
    // Reads 6 doubles
    GLdouble* vertex = &combineBuffer[combineBufferIndex];

    // Coordinates of the combined vertex
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    for (int i = 3; i < 6; i++)
    {
        vertex[i] = weight[0] * vertex_data[0][i] +
                    weight[1] * vertex_data[1][i] +
                    weight[2] * vertex_data[2][i] +
                    weight[3] * vertex_data[3][i];
    }
    *dataOut = vertex;
    combineBufferIndex = (combineBufferIndex + 6) % COMBINE_BUFFER_SIZE;
}
*/

void CALLBACK tessEnd(void)
{
    //Log_Info("Tesselation end\n");
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
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


void OpenGLRender_Init()
{

    // TODO move to graphics system
    checkers = Texture_GenerateCheckerBoard();
    if (tesselator == nullptr)
    {
        tesselator = gluNewTess();
        mgdl_assert_print(tesselator != nullptr, "No Glut tesselator!");

        gluTessCallback(tesselator, GLU_TESS_BEGIN_DATA, (void(*)())tessBegin);
        gluTessCallback(tesselator, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)tessVertex);
        gluTessCallback(tesselator, GLU_TESS_END, (_GLUfuncptr)tessEnd);
        gluTessCallback(tesselator, GLU_TESS_ERROR, (_GLUfuncptr)tessError);
        gluTessCallback(tesselator, GLU_TESS_EDGE_FLAG, (_GLUfuncptr)tessEdgeFlag); // this makes tess only submit triangles
    }

    bulletTexture = mgdl_LoadTexture("assets/tempBullet.png", Linear);
    treasureTexture = mgdl_LoadTexture("assets/tempTreasure.png", Linear);
    playerTexture = mgdl_LoadTexture("assets/tempPlayer.png", Linear);

    if (vertexRingBuffer == nullptr)
    {
#ifdef GEKKO
        vertexRingBuffer = (GLdouble*)malloc(VERTEX_BUFFER_SIZE_BYTES);
#else
        vertexRingBuffer = (GLdouble*)mgdl_AllocateGraphicsMemory(VERTEX_BUFFER_SIZE_BYTES);
#endif
    }
}

void DrawQuad(vec2 start, vec2 end, float floorY, float ceilingY, s16 picnum, RenderSettingsOpenGL* settings3D)
{
    // Keep texture aspect 1:1 unless told otherwise
    float width = vec2Length( vec2Subtract(end, start)) * settings3D->scaleXZ;
    float height = (ceilingY - floorY) * settings3D->scaleY;

    // TODO move to graphics
    float aspect = width/height;
    float tex_x1 = 0.0f;
    float tex_x2 = aspect * height * settings3D->textureScale;
    float tex_bottom = 0.0f;
    float tex_top = 1.0 * height * settings3D->textureScale;

    // TODO enable and disable just once
    glEnable(GL_TEXTURE_2D);

    // TODO Get texture that corresponds to picnum
    glBindTexture(GL_TEXTURE_2D, OpenGLRender_GetTexture(picnum)->textureId);

    // TODO Do this just once when loading textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBegin(GL_TRIANGLES);
        // Build Triangles for wall
        glTexCoord2f(tex_x1, tex_bottom); // 0
        glVertex3f(start.x, floorY, start.y); // 0

        glTexCoord2f(tex_x2, tex_bottom); // 1
        glVertex3f(end.x, floorY, end.y);   // 1

        glTexCoord2f(tex_x2, tex_top); // 2
        glVertex3f(end.x, ceilingY, end.y); // 2

        glTexCoord2f(tex_x2, tex_top); // 2
        glVertex3f(end.x, ceilingY, end.y);  // 2

        glTexCoord2f(tex_x1, tex_top); // 3
        glVertex3f(start.x, ceilingY, start.y); // 3

        glTexCoord2f(tex_x1, tex_bottom); // 0
        glVertex3f(start.x, floorY, start.y); // 0

    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void OpenGLRender_DrawWall(DukeMap* map, Wall* w, float floorY, float ceilingY, RenderSettingsOpenGL* settings)
{

    vec2 start = vec2New(w->x, w->z);
    Wall* wend = Map_GetWallEnd(map, w);
    vec2 end =  vec2New(wend->x, wend->z);
    if (w->nextsector >= 0)
    {
        // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
        Sector* neighbor = Map_GetSector(map, w->nextsector);
        int n_floorY = neighbor->floory;
        int n_ceilingY = neighbor->ceilingy;

        // if this floor height is less than adjacent: Greate wall in between: goes up
        if (floorY < n_floorY)
        {
            DrawQuad(start, end, floorY, n_floorY, w->picnum, settings);
        }

        // Ceiling:
        // If this ceiling is higher than adjacent: Greate wall in between: goes down
        if (ceilingY > n_ceilingY)
        {
            Wall* otherWall = Map_GetWall(map, w->nextwall);
            DrawQuad(start, end, n_ceilingY, ceilingY, otherWall->picnum, settings);
        }
    }
    else
    {
        // Draw the wall
        glColor3f(0.5f, 0.5f, 0.5f);
        DrawQuad(start, end, floorY, ceilingY, w->picnum, settings);
    }
}

void OpenGLRender_DrawFloorOrCeiling(DukeMap* map, Sector* sector, bool floor)
{
    float ceilingY = sector->ceilingy;
    float floorY = sector->floory;
    if (floor)
    {
        glColor3f(0.0f, 0.0f, 0.5f);
    }
    else
    {
        glColor3f(0.0f, 0.5f, 0.0f);
    }

    // TODO Get texture from wall or ceiling
    // TODO Are we drawing floor or ceiling???

    glEnable(GL_TEXTURE_2D);
    glPushMatrix();
        // Set floor level to 0.0f
        if (floor)
        {
            glTranslatef(0.0f, floorY, 0.0f);
            gluTessNormal(tesselator, 0, 1, 0); // All points on XZ plane
            Texture* floorTexture = OpenGLRender_GetTexture(sector->floorpicnum);
            glBindTexture(GL_TEXTURE_2D, floorTexture->textureId);
        }
        else
        {
            glTranslatef(0.0f, ceilingY, 0.0f);
            gluTessNormal(tesselator, 0, -1, 0); // All points on XZ plane

            Texture* ceilingTexture = OpenGLRender_GetTexture(sector->ceilingpicnum);
            glBindTexture(GL_TEXTURE_2D, ceilingTexture->textureId);

        }
        // TODO do this only once
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // TODO If there are sectors inside sectors we need a much more complex
        // tesselation
        gluTessBeginPolygon(tesselator, sector);

        // If the end of last wall is not the sectors wallptr, then there are islands
        Wall* last_wall = Map_GetWallInSectorPtr(map, sector, sector->wallnum-1);
        if (last_wall->nextwall != sector->wallptr)
        {
            // This sector contains islands
            // TODO do something about it
        }

        gluTessBeginContour(tesselator);
        //Log_InfoF("Tesselating sector %d\n", request.number);
        for (s16 wi = sector->wallnum-1; wi >= 0; wi--)
        {
            Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
            // Tesselation
            // NOTE DANGER Must be counter clockwise
            //Log_InfoF("Tesselation vertex sent %d:: %.2f, %.2f\n", wi, w->glutVertices[0], w->glutVertices[2]);
            // TODO Send normal too, but maybe not with every vertex?
            vertexRingBuffer[VertexBufferIndexDoubles + 0] = w->x;
            vertexRingBuffer[VertexBufferIndexDoubles + 1] = 0;
            vertexRingBuffer[VertexBufferIndexDoubles + 2] = w->z;
            gluTessVertex(tesselator, &vertexRingBuffer[VertexBufferIndexDoubles], &vertexRingBuffer[VertexBufferIndexDoubles]);
            VertexBufferIndexDoubles = (VertexBufferIndexDoubles + 3) % VERTEX_BUFFER_SIZE_DOUBLES;
        }
        gluTessEndContour(tesselator);
        gluTessEndPolygon(tesselator);
    glPopMatrix();
    // First round: floor is false
    // Second round: floor is true

}


void OpenGLRender_DrawSprite(vec3 position, float width, float height, float spriteAngle, float playerAngle, SpriteAlignment alignment, SpritePivot pivot, s16 picnum)
{
    glEnable(GL_TEXTURE_2D);
	Texture* spriteTexture = OpenGLRender_GetTexture(picnum);
	glBindTexture(GL_TEXTURE_2D, spriteTexture->textureId);

	if (alignment == Sprite_FACE)
	{
		spriteAngle = playerAngle - M_PI_2;
	}

	vec3 spriteDir = Vec3XYZRotateY(WORLD_FORWARD, spriteAngle);

	vec3 toRight = vec3Multiply(spriteDir, width/2);

    vec3 bottomRight, bottomLeft, topLeft, topRight;
    if (pivot == Sprite_PivotCenter)
    {
        bottomRight = vec3Add(position, toRight);
        bottomRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, -height / 2));
        bottomLeft = vec3Subtract(position, toRight);
        bottomLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, -height / 2));
        topLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, height));
        topRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, height));
    }
    else
    {
        bottomRight = vec3Add(position, toRight);
        bottomLeft = vec3Subtract(position, toRight);
        topLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, height));
        topRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, height));
    }

	if (alignment == Sprite_FACE)
	{
		// These always face the player
		//OpenGLRender_SetColor(Color_Red);
	}
	else if (alignment == Sprite_FLOOR)
	{
		//OpenGLRender_SetColor(Color_Green);

		// Raise up to avoid Z fighting
		// TODO position.y += pushOut;

		// Calculate four carpet corners
		vec3 toLeft = Vec3XYZRotateY(spriteDir, M_PI_2);
		// bl = pos + left + forward
		bottomLeft = vec3Add(position, vec3Add( vec3Multiply(toLeft, width/2), vec3Multiply(spriteDir, width/2)));
		// br = pos - left + forward
		bottomRight = vec3Add(position, vec3Add( vec3Multiply(toLeft, -width/2), vec3Multiply(spriteDir, width/2)));
		// tl = pos + left - forward
		topLeft = vec3Add(position, vec3Add( vec3Multiply(toLeft, width/2), vec3Multiply(spriteDir, -width/2)));
		// tr = pos - left - forward
		topRight = vec3Add(position, vec3Add( vec3Multiply(toLeft, -width/2), vec3Multiply(spriteDir, -width/2)));
	}
	else if (alignment == Sprite_WALL)
	{
		//OpenGLRender_SetColor(Color_Blue);

		// Push out of wall to avoid Z fight
		vec3 spriteDir = Vec3XYZRotateY(WORLD_FORWARD, spriteAngle);

		vec3 toRight = vec3Multiply(spriteDir, width/2);
		vec3 normal = Vec3XYZRotateY(WORLD_FORWARD, spriteAngle);
		// TODO position = vec3Add(position, vec3Multiply(normal, pushOut));

		// calculate corners // TODO just add to existing ones

		bottomRight = vec3Add(position, toRight);
		bottomLeft = vec3Subtract(position, toRight);
		topLeft = vec3Add(bottomLeft, vec3Multiply(WORLD_UP, height));
		topRight = vec3Add(bottomRight, vec3Multiply(WORLD_UP, height));
	}

    glBegin(GL_QUADS);
	//glColor3f(0.5f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(topRight.x, topRight.y, topRight.z);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(topLeft.x, topLeft.y, topLeft.z);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
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
