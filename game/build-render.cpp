// Renders a duke map in 2D
#include <mgdl.h>
#include <mgdl/mgdl-vector.h>
#include <mgdl/mgdl-color.h>
#include <mgdl/mgdl-vectorfunctions.h>

#include <mgdl/ccVector/ccVector.h>

#include "build-render.h"
#include "dukemap.h"
#include "player.h"


// TODO move to mgdl util
#define min(a,b)             (((a) < (b)) ? (a) : (b)) // min: Choose smaller of two scalars.
#define max(a,b)             (((a) > (b)) ? (a) : (b)) // max: Choose greater of two scalars.
#define vxs(x0,y0, x1,y1)    ((x0)*(y1) - (x1)*(y0))   // vxs: Vector cross product
// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
// IntersectBox: Determine whether two 2D-boxes intersect.
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))
// PointSide: Determine which side of a line the point is on. Return value: <0, =0 or >0.
#define PointSide(px,py, x0,y0, x1,y1) vxs((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0))
// Intersect: Calculate the point of intersection between two lines.
#define Intersect(x1,y1, x2,y2, x3,y3, x4,y4) (vec2New( \
    vxs(vxs(x1,y1, x2,y2), (x1)-(x2), vxs(x3,y3, x4,y4), (x3)-(x4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)), \
    vxs(vxs(x1,y1, x2,y2), (y1)-(y2), vxs(x3,y3, x4,y4), (y3)-(y4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)) ))

// TODO move to util
static int clampInt(int v, int min, int max)
{
    if (v<min)
    {
        v = min;
    }
    else if (v>max)
    {
        v = max;
    }
    return v;
}
static vec2 RotateZ(vec2 p, float angle)
{
	float xt = p.x*cos(angle) - p.y*sin(angle);
	float yt = p.x*sin(angle) + p.y*cos(angle);
    return vec2New(xt, yt);
}


// How many portals can be waiting for drawing
#define MAX_PORTAL_QUEUE 32
SectorRender* renderQueue = nullptr; // Circular buffer of render requests
s16 renderQueueInserts = 0;
// These point to renderQueue
SectorRender* head;
SectorRender* tail;

int* renderedSectorNames = nullptr; // NOTE this is related to all sectors in map

static int lastSectorAmount = 0;
static int W;
static int H;

SectorRender* BuildRender_GetDrawnSectorNumbers()
{
    return renderQueue;
}
s16 BuildRender_GetDrawnSectorAmount()
{
    return renderQueueInserts;
}
bool BuildRender_WasSectorDrawn(s16 sectornumber)
{
    return renderedSectorNames[sectornumber] > 0;
}

// OpenGL
Texture* checkers;
GLUtesselator* tesselator = nullptr;
bool tesselationActive = true;

static void SetColor(DefaultColor oc)
{
    Color4f* c = Color_GetDefaultColor(oc);
    glColor4fv(&c->red);
}

static void Line2(int x1, int z1, int x2, int z2)
{
    glVertex2i(x1, z1);
    glVertex2i(x2, z2);
}

// TESSELATION CALLBACKS
// /////////////////////

// Ring buffer for combine to use
static GLdouble* combineBuffer = nullptr;
#define COMBINE_BUFFER_SIZE (64*6)
int combineBufferIndex = 0;

#ifndef CALLBACK
#define CALLBACK
#endif

void CALLBACK tessBegin(GLenum which)
{
    //Log_InfoF("Tesselation start mode: %s \n", which == GL_TRIANGLES ? "Triangles" : "Not triangles");
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, checkers->textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBegin(which);
}
void CALLBACK tessVertex(GLvoid* vertex, void* SectorPtr)
{
    const Sector* s = (Sector*)SectorPtr;
    const GLdouble* pointer;
    pointer = (GLdouble*)vertex;
    //Log_InfoF("Tesselation vertex %.2f, %.2f\n", pointer[0], pointer[2]);
    // TODO texture coordinates and colors
    float xrange = s->sizeXY.x;
    float yrange = s->sizeXY.y;
    float xdiff = pointer[0] - s->minXYPoint.x;
    float ydiff = pointer[2] - s->minXYPoint.y;
    float tx = xdiff/xrange * s->maxTexCoord.x;
    float ty = ydiff/yrange * s->maxTexCoord.y;
    //Log_InfoF("Tesselation tex coord %.2f, %.2f\n", tx, ty);
    glTexCoord2f(tx,ty);
    glVertex3dv(pointer);

}
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
    /* Weighing other data that we dont have yet
     *
    for (int i = 3; i < 6; i++)
    {
        vertex[i] = weight[0] * vertex_data[0][i] +
                    weight[1] * vertex_data[1][i] +
                    weight[2] * vertex_data[2][i] +
                    weight[3] * vertex_data[3][i];
    }
    */
    *dataOut = vertex;
    combineBufferIndex = (combineBufferIndex + 6) % COMBINE_BUFFER_SIZE;
}

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
   glEdgeFlag(flag);
}



void BuildRender_Init(DukeMap* map, RenderSettingsOpenGL* settings3D)
{
    H = mgdl_GetScreenHeight();
    W = mgdl_GetScreenWidth();
    if (renderQueue == nullptr)
    {
        renderQueue = (SectorRender*)malloc(sizeof(SectorRender) * MAX_PORTAL_QUEUE);
    }

    // init again if more is needed
    if (renderedSectorNames != nullptr)
    {
        if (lastSectorAmount < map->sectorAmount)
        {
           free(renderedSectorNames);
           renderedSectorNames= nullptr;
        }
    }
    if (renderedSectorNames == nullptr)
    {
        renderedSectorNames = (int*)malloc(sizeof(int) * map->sectorAmount);
    }

    lastSectorAmount = map->sectorAmount;
    renderQueueInserts = 0;

    checkers = Texture_GenerateCheckerBoard();
    if (tesselator == nullptr)
    {
        tesselator = gluNewTess();
        mgdl_assert_print(tesselator != nullptr, "No Glut tesselator!");

        gluTessCallback(tesselator, GLU_TESS_BEGIN, (void(*)())tessBegin);
        gluTessCallback(tesselator, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)tessVertex);
        gluTessCallback(tesselator, GLU_TESS_END, (_GLUfuncptr)tessEnd);
        gluTessCallback(tesselator, GLU_TESS_ERROR, (_GLUfuncptr)tessError);
        gluTessCallback(tesselator, GLU_TESS_COMBINE, (_GLUfuncptr)tessCombine);
        gluTessCallback(tesselator, GLU_TESS_EDGE_FLAG, (_GLUfuncptr)tessEdgeFlag); // this makes tess only submit triangles
    }

    if (combineBuffer == nullptr)
    {
        combineBuffer = (GLdouble*)malloc(sizeof(GLdouble)*COMBINE_BUFFER_SIZE);
    }

    // Build other data needed by game
    for (int si = 0; si < map->sectorAmount; si++)
    {
        vec2 minp = vec2New(32000, 32000);
        vec2 maxp = vec2New(-32000, -32000);
        Sector* sector = &map->sectors[si];
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = &map->walls[sector->wallptr + wi];
            w->start = vec2New(w->x, w->y);
            w->glutVertices[0] = w->x;
            w->glutVertices[1] = 0; // Left to zero, glTranslate handles height
            w->glutVertices[2] = w->y;
            minp.x = min(w->start.x, minp.x);
            minp.y = min(w->start.y, minp.y);
            maxp.x = max(w->start.x, maxp.x);
            maxp.y = max(w->start.y, maxp.y);
        }
        // Found points : calculate tex coords
        float width = (maxp.x - minp.x) * settings3D->scaleXZ;
        float height = (maxp.y - minp.y) * settings3D->scaleXZ;
        float aspect = width/height;
        sector->minXYPoint = minp;
        sector->sizeXY = vec2Subtract(maxp, minp);
        sector->maxTexCoord.x = aspect * height * settings3D->textureScale;
        sector->maxTexCoord.y = 1.0 * height * settings3D->textureScale;
    }
}

void DrawQuad(vec2 start, vec2 end, float floorY, float ceilingY, RenderSettingsOpenGL* settings3D)
{
    // Keep texture aspect 1:1 unless told otherwise
    float width = vec2Length( vec2Subtract(end, start)) * settings3D->scaleXZ;
    float height = (ceilingY - floorY) * settings3D->scaleY * -1.0f;
    float aspect = width/height;
    float tex_x1 = 0.0f;
    float tex_x2 = aspect * height * settings3D->textureScale;
    float tex_bottom = 0.0f;
    float tex_top = 1.0 * height * settings3D->textureScale;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, checkers->textureId);
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

void BuildRender_DrawOpenGL(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    for (int i = 0; i < map->sectorAmount ; i++)
    {
        renderedSectorNames[i] = 0;
    }

    // No items in buffer
    head = renderQueue;
    tail = renderQueue;

    vec2 playerPos2 = vec2New(player->positionOpenGL.x, player->positionOpenGL.y);
    //playerPos2.x *= settings->scaleXZ;
    //playerPos2.y *= settings->scaleXZ;
    // Put the pla
    // Draw whole screen: ytop and ybottom are at initial values
    *head = {player->sectorNumber};
    renderQueueInserts++;
    // Circular buffer pointer arithmetics
    // Next request is put towards the tail
    if ( ( head += 1) == renderQueue + MAX_PORTAL_QUEUE)
    {
        head = renderQueue;
    }

    glPushMatrix();
    glScalef(settings->scaleXZ, -settings->scaleY, settings->scaleXZ);

    // Start drawing OpenGL lines

    // Draw a sector and put more sectors to queue for drawing
    do {
        // Take last request from buffer: the first one is the head
        SectorRender request = (*tail);
        // Move tail to next one
        if ( ( tail += 1) == renderQueue + MAX_PORTAL_QUEUE)
        {
            tail = renderQueue;
        }
        // If the number is odd, keep rendering. If number is 32 give up
        // This tests that the same sector is not drawn too many times?
        if (renderedSectorNames[request.number] & 0x21) // 0x21 is 32 + 1
        {
            continue;
        }
        // Add one to this sector: it is now being rendered
        renderedSectorNames[request.number] += 1;

        // Get the sector info from map
        Sector* sector = Map_GetSector(map, request.number);

        float ceilingY = sector->ceilingz;
        float floorY = sector->floorz;

        bool drawTesselation = true;
        // Draw the floor and ceiling with tesselation
        // TODO Use gluTesselateion to draw floor and ceiling
        if (drawTesselation)
        {
            bool floor = true;
            do {
                if (floor)
                {
                    glColor3f(0.0f, 0.0f, 0.5f);
                }
                else
                {
                    glColor3f(0.0f, 0.5f, 0.0f);
                }
                glPushMatrix();
                    // Set floor level to 0.0f
                    if (floor)
                    {
                        glTranslatef(0.0f, floorY, 0.0f);
                        gluTessNormal(tesselator, 0, 1, 0); // All points on XZ plane
                    }
                    else
                    {
                        glTranslatef(0.0f, ceilingY, 0.0f);
                        gluTessNormal(tesselator, 0, -1, 0); // All points on XZ plane
                    }
                    // TODO If there are sectors inside sectors we need a much more complex
                    // tesselation
                    gluTessBeginPolygon(tesselator, sector);
                    gluTessBeginContour(tesselator);
                    //Log_InfoF("Tesselating sector %d\n", request.number);
                    for (s16 wi = sector->wallnum-1; wi >= 0; wi--)
                    {
                        Wall* w = Map_GetWallInSector(map, request.number, wi);
                        // Tesselation
                        // NOTE DANGER Must be counter clockwise
                        //Log_InfoF("Tesselation vertex sent %d:: %.2f, %.2f\n", wi, w->glutVertices[0], w->glutVertices[2]);
                        gluTessVertex(tesselator, w->glutVertices, w->glutVertices);
                    }
                    gluTessEndContour(tesselator);
                    gluTessEndPolygon(tesselator);
                glPopMatrix();
                floor = !floor;
                // First round: floor is false
                // Second round: floor is true
            } while(floor == false);
        }


        // Render all walls of the current sector
        // Discard those that do not face player
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, request.number, wi);
            vec2 start = w->start;
            vec2 end =  w->end;


            vec2 startZ = vec2Subtract(start, playerPos2);
            vec2 endZ = vec2Subtract(end, playerPos2);

            // The Y is how far ahead of player the point is
            // NOTE negative around player
            startZ = RotateZ(startZ, -player->angleRad);
            endZ = RotateZ(endZ, -player->angleRad);
            //startZ.x = start.x * player_sin - start.y * player_cos;
            //startZ.z = start.x * player_cos + start.y * player_sin;
            // endZ.x = end.x * player_sin - end.y * player_cos;
            // endZ.z = end.x * player_cos + end.y * player_sin;
            // Is the wall behind player?
            if(startZ.y <= 0 && endZ.y <= 0)
            {
                // Draw next wall
                continue;
            }

            // Wall is drawn
            // if it was a portal Add neighbor to queue
            // if there is neighbor AND AND there is room in QUEUE
            if (w->nextsector >= 0)
            {
                // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
                Sector* neighbor = Map_GetSector(map, w->nextsector);
                int n_floorY = neighbor->floorz;
                int n_ceilingY = neighbor->ceilingz;

                // if this floor height is less than adjacent: Greate wall in between: goes up
                if (floorY < n_floorY)
                {
                    SetColor(Color_Blue);
                    DrawQuad(start, end, floorY, n_floorY, settings);
                }

                // Ceiling:
                // If this ceiling is higher than adjacent: Greate wall in between: goes down
                if (ceilingY > n_ceilingY)
                {
                    SetColor(Color_Green);
                    DrawQuad(start, end, n_ceilingY, ceilingY, settings);
                }
                if  ((head + MAX_PORTAL_QUEUE+1-tail)%MAX_PORTAL_QUEUE)
                {
                    (*head) = {w->nextsector};
                    // Move head and loop around buffer
                    if ( (++head) == renderQueue + MAX_PORTAL_QUEUE)
                    {
                        head = renderQueue;
                        renderQueueInserts++;
                    }
                }
            }
            else
            {
                glColor3f(0.5f, 0.5f, 0.5f);
                DrawQuad(start, end, floorY, ceilingY, settings);

                /*
                glBegin(GL_LINE_LOOP);
                    SetColor(Color_Red);
                    // Draw outline
                    glVertex3f(start.x, floorY, start.y); // 0
                    glVertex3f(end.x, floorY, end.y);   // 1
                    glVertex3f(end.x, ceilingY, end.y); // 2
                    glVertex3f(start.x, ceilingY, start.y); // 3
                glEnd();
                */
            }
        } // All walls of the sector have been drawn; head has moved forward


        // Make the count even??
        renderedSectorNames[request.number] += 1;

    } while(head != tail); // Render until buffer is empty: if nothing was added, they are the same
    glPopMatrix();

}

void DrawDot(vec2 point, float size, DefaultColor color)
{
    SetColor(color);

    glVertex2i(point.x,point.y - size);
    glVertex2i(point.x + size,point.y);

    glVertex2i(point.x + size,point.y);
    glVertex2i(point.x ,point.y + size);

    glVertex2i(point.x ,point.y + size);
    glVertex2i(point.x - size,point.y);

    glVertex2i(point.x - size,point.y);
    glVertex2i(point.x,point.y - size);
}

void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D)
{
    Font* df = DefaultFont_GetDefaultFont();

    vec2 playerPos2;
    if (settings2D->movePlayer)
    {
        playerPos2 = vec2New(player->positionOpenGL.x, player->positionOpenGL.y);
    }
    else
    {
        playerPos2 = settings2D->collisionPoint;
    }

    vec2 collision_forward = vec2New(0.0, settings2D->collisionLength);

    collision_forward  = RotateZ(collision_forward,  Deg2Rad(settings2D->collisionAngleDeg));
    vec2 collisionEnd = vec2Add(playerPos2, collision_forward);

    bool collisionMiss = true;
    vec2 collisionOut;



    // WALL NUMBERS
    ////////////////////
    glLineWidth(4.0f);
    // The whole map zoom
    // Put the origo on the center of the screen
    glPushMatrix();

        glTranslatef(
            W/2 + settings2D->mapOffset.x,
            H/2 + settings2D->mapOffset.y,
            0.0f);
        glScalef(settings2D->mapZoom, settings2D->mapZoom, 1);

        // The walls zoom
        glPushMatrix();
            // Keep player at center of screen
            glRotatef(Rad2Deg(-player->angleRad) + 180, 0, 0, 1);
            glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);
            glTranslatef(-playerPos2.x, -playerPos2.y, 0);


            // DRAW WALLS
            ////////////////////////////
            glBegin(GL_LINES);
            // Draw origo
            SetColor(Color_White);
            Line2(0, -10, 0 ,10);
            Line2(-10, 0, 10, 0);

            for(int si = 0; si < map->sectorAmount; si++)
            {
                if (settings2D->drawOneSector >=0 && settings2D->drawOneSector != si)
                { continue; }

                // Get the sector info from map
                Sector* sector = Map_GetSector(map, si);
                // Render all walls of the current sector
                // Discard those that do not face player
                for (s16 wi = 0; wi < sector->wallnum; wi++)
                {
                    if (settings2D->drawOneWall >=0 && settings2D->drawOneWall != wi)
                    { continue; }
                    Wall* w = Map_GetWallInSector(map, si, wi);
                    vec2 start = w->start;
                    vec2 end =  w->end;
                    //start = vec2Multiply(start, settings2D->scaleXZ);
                    //end = vec2Multiply(end, settings2D->scaleXZ);
                    // Rotate around player
                    //start = vec2Subtract(start, playerPos2);
                    //end = vec2Subtract(end, playerPos2);

                    // TODO Show collision detection info here
                    if (settings2D->movePlayer == false)
                    {
                        if (Map_FindIntersectionWithWall(settings2D->collisionPoint, collisionEnd, w, &collisionOut))
                        {
                            DrawDot(collisionOut, 48, DefaultColor::Color_White);
                        }
                        else
                        {
                            collisionMiss = true;
                            glPushMatrix();
                            glScalef(1, -1, 1);
                            glPopMatrix();
                        }
                        if (Map_IsPointInsideWall(settings2D->collisionPoint, w))
                        {
                            SetColor(Color_Green);
                        }
                        else
                        {
                            SetColor(Color_Black);
                        }
                    }
                    else
                    {
                        if (w->nextsector < 0)
                        {
                            SetColor(Color_White);
                        }
                        else
                        {
                            SetColor(Color_Green);
                        }
                    }

                    Line2(start.x, start.y, end.x, end.y);
                    vec2 m = Wall_GetMiddle(w);
                    vec2 N = vec2Multiply( Wall_GetNormal(w), 32 );
                    Line2(m.x, m.y, m.x + N.x, m.y + N.y);
                }
            }
            glEnd(); // end walls

            glPushMatrix();
            glScalef(1, -1, 1);

            Color4f* c = Color_GetDefaultColor(Color_White);
            int numberSize = 32;
            // DRAW WALL NUMBERS
            for(int si = 0; si < map->sectorAmount; si++)
            {
                if (settings2D->drawOneSector >=0 && settings2D->drawOneSector != si)
                { continue; }
                // Get the sector info from map
                Sector* sector = Map_GetSector(map, si);
                // Render all walls of the current sector
                // Discard those that do not face player
                for (s16 wi = 0; wi < sector->wallnum; wi++)
                {

                    if (settings2D->drawOneWall >=0 && settings2D->drawOneWall != wi)
                    { continue; }
                    Wall* w = Map_GetWallInSector(map, si, wi);
                    vec2 start = w->start;

                    int tx = start.x - numberSize/2;
                    int ty = -start.y + numberSize/2;

                    Draw2D_RectWH(tx, ty, numberSize, numberSize, Color_GetDefaultColor(Color_Black));

                    SetColor(Color_White);
                    Font_Printf(df, c, tx, ty, numberSize, "%d", sector->wallptr+wi);
                    if (settings2D->drawOneWall == wi && settings2D->drawOneSector == si)
                    {

                        if (collisionMiss)
                        {
                            Font_Printf(df, c, start.x, start.y + 64+32, 32, "U: %.2f", collisionOut.y);
                            Font_Printf(df, c, start.x, start.y + 64, 32, "T: %.2f", collisionOut.x);
                        }
                        else
                        {
                            Font_Printf(df, c, start.x, start.y + 64+32, 32, "Y: %.2f", collisionOut.y);
                            Font_Printf(df, c, start.x, start.y + 64, 32, "X: %.2f", collisionOut.x);
                        }
                    }
                }
            }
            glPopMatrix();
        glPopMatrix(); // WALLS

        // DRAW PLAYER
        // ////////////

    glPushMatrix();

        glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);
        glBegin(GL_LINES);
        DefaultColor pc =  Color_Black;
            if (mgdl_GetElapsedFrames() % 30 == 0)
            {
                pc = Color_White;
            }
            float dotSize = player->radius/2;

            // PLAYER ARROW
            // //////////////////
            if (settings2D->movePlayer)
            {
                playerPos2 = vec2Zero();
                DrawDot(playerPos2, dotSize,pc );
                SetColor(Color_Red);
                vec2 forward = vec2New(0, -1);//vec2New(player->direction.x, player->direction.y);
                forward = vec2Multiply(forward, player->radius * 2);
                vec2 end = vec2Add(playerPos2, forward);

                glVertex2i(playerPos2.x,playerPos2.y);
                glVertex2f(end.x, end.y);

                vec2 sideLeft = RotateZ(forward,  M_PI * 3.0f/4.0f);
                vec2 sideRight = RotateZ(forward, -M_PI * 3.0f/4.0f);
                sideLeft = vec2Add(sideLeft, end);
                sideRight = vec2Add(sideRight, end);

                glVertex2f(end.x, end.y);
                glVertex2f(sideLeft.x, sideLeft.y);

                glVertex2f(end.x, end.y);
                glVertex2f(sideRight.x, sideRight.y);
            }
            else
            {
                DrawDot(playerPos2, dotSize,pc );
                SetColor(Color_Black);
                vec2 forward = vec2New(0.0, settings2D->collisionLength);

                forward  = RotateZ(forward,  Deg2Rad(settings2D->collisionAngleDeg));
                vec2 end = vec2Add(playerPos2, forward);

                glVertex2i(playerPos2.x,playerPos2.y);
                glVertex2f(end.x, end.y);
            }

        glEnd();
        glPopMatrix(); // Player
    glPopMatrix(); // Whole map view
    glLineWidth(1.0f);
}



