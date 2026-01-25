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
SectorRender* renderQueue; // Circular buffer of render requests
s16 renderQueueInserts = 0;
// These point to renderQueue
SectorRender* head;
SectorRender* tail;

int* renderedSectorNames; // NOTE this is related to all sectors in map

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
GLUtesselator* tesselator;
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
static GLdouble* combineBuffer;
#define COMBINE_BUFFER_SIZE (64*6)
int combineBufferIndex = 0;

#ifndef CALLBACK
#define CALLBACK
#endif

void CALLBACK tessBegin(GLenum which)
{
    //Log_InfoF("Tesselation start mode: %s \n", which == GL_TRIANGLES ? "Triangles" : "Not triangles");
    glBegin(which);
}
void CALLBACK tessVertex(GLvoid* vertex)
{
    const GLdouble* pointer;
    pointer = (GLdouble*)vertex;
    // TODO texture coordinates and colors
    glVertex3dv(pointer);
    //Log_InfoF("Tesselation vertex %.2f, %.2f\n", pointer[0], pointer[2]);

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



void BuildRender_Init(DukeMap* map)
{
    H = mgdl_GetScreenHeight();
    W = mgdl_GetScreenWidth();
    renderQueue = (SectorRender*)malloc(sizeof(SectorRender) * MAX_PORTAL_QUEUE);

    // init again if more is needed
    renderedSectorNames = (int*)malloc(sizeof(int) * map->sectorAmount);
    lastSectorAmount = map->sectorAmount;
    renderQueueInserts = 0;

    checkers = Texture_GenerateCheckerBoard();
    tesselator = gluNewTess();
    mgdl_assert_print(tesselator != nullptr, "No Glut tesselator!");

    gluTessCallback(tesselator, GLU_TESS_BEGIN, (void(*)())tessBegin);
    gluTessCallback(tesselator, GLU_TESS_VERTEX, (_GLUfuncptr)tessVertex);
    gluTessCallback(tesselator, GLU_TESS_END, (_GLUfuncptr)tessEnd);
    gluTessCallback(tesselator, GLU_TESS_ERROR, (_GLUfuncptr)tessError);
    gluTessCallback(tesselator, GLU_TESS_COMBINE, (_GLUfuncptr)tessCombine);
    gluTessCallback(tesselator, GLU_TESS_EDGE_FLAG, (_GLUfuncptr)tessEdgeFlag); // this makes tess only submit triangles

    combineBuffer = (GLdouble*)malloc(sizeof(GLdouble)*COMBINE_BUFFER_SIZE);

    // Build other data needed by game
    for (int si = 0; si < map->sectorAmount; si++)
    {
        Sector* sector = &map->sectors[si];
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = &map->walls[sector->wallptr + wi];
            w->start = vec2New(w->x, w->y);
            w->glutVertices[0] = w->x;
            w->glutVertices[1] = 0; // Left to zero, glTranslate handles height
            w->glutVertices[2] = w->y;
        }
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
                    gluTessBeginPolygon(tesselator, NULL);
                    gluTessBeginContour(tesselator);
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

                glBegin(GL_LINE_LOOP);
                    SetColor(Color_Red);
                    // Draw outline
                    glVertex3f(start.x, floorY, start.y); // 0
                    glVertex3f(end.x, floorY, end.y);   // 1
                    glVertex3f(end.x, ceilingY, end.y); // 2
                    glVertex3f(start.x, ceilingY, start.y); // 3

                glEnd();
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
    glPushMatrix();
        glTranslatef(W/2 + settings2D->mapOffset.x, H/2 + settings2D->mapOffset.y, 0.0f);
        //glRotatef(180, 0.0f, 0.0f, 1.0f); // Mapster32 Y is down. OpenGL Y is up
        glScalef(settings2D->mapZoom, settings2D->mapZoom, 1);


            // Draw origo
            SetColor(Color_White);
            Line2(0, -10, 0 ,10);
            Line2(-10, 0, 10, 0);
        // The walls zoom
        glPushMatrix();
            glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);

            // DRAW WALLS
            ////////////////////////////
            glBegin(GL_LINES);
            // Keep player at center

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
            DrawDot(playerPos2, dotSize,pc );

            // PLAYER ARROW
            // //////////////////
            if (settings2D->movePlayer)
            {
                SetColor(Color_Red);
                vec2 forward = vec2New(player->direction.x, player->direction.y);
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
        /*
    for(int si = 0; si < map->sectorAmount; si++)
    {
        // Get the sector info from map
        Sector* sector = Map_GetSector(map, si);
        // Render all walls of the current sector
        // Discard those that do not face player
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, si, wi);
            vec2 start = w->start;
            vec2 end =  w->end;
            // Rotate around player
            start = vec2Subtract(start, playerPos2);
            end = vec2Subtract(end, playerPos2);

            Font_Printf(df, c, start.x-8, start.y-8, 16, "%d", sector->wallptr+wi);
        }
    }
    */
}


/** OLD LINE BASED 3D RENDERING
void BuildRender_DrawFirstPerson(Player* player, DukeMap* map, RenderSettings2D* settings)
{
    W = mgdl_GetScreenWidth();
    H = mgdl_GetScreenHeight();

    // TODO Calculate these to match window size and OpenGL
    const float FovH = 0.75f * H; // Horizontal
    const float FovV = 0.2f * H; // Vertical
    const float nearZ = 1e-4f;
    const float farZ = 5.0f;
    const float nearSide = 1e-4f;
    const float farSide = 20.0f; // From screen widh and FOV

    vec2 playerPos2 = vec2New(player->position2D.x, player->position2D.y);
    playerPos2 = ConvertXY(playerPos2, settings);
    InitArrays();

    // Put the player's sector to head of buffer
    // Draw whole screen: ytop and ybottom are at initial values
    *head = {player->sectorNumber, 0, W-1};
    // Circular buffer pointer arithmetics
    // Next request is put towards the tail
    if ( ( head += 1) == renderQueue + MAX_PORTAL_QUEUE)
    {
        head = renderQueue;
    }

    // Start drawing OpenGL lines
    glBegin(GL_LINES);

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
        // Render all walls of the current sector
        // Discard those that do not face player
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, request.number, wi);
            vec2 start = ConvertXY(w->start, settings);
            vec2 end = ConvertXY( w->end, settings);
            // Rotate around player
            start = vec2Subtract(start, playerPos2);
            end = vec2Subtract(end, playerPos2);

            // The Y is how far ahead of player the point is
            vec2 startZ;
            vec2 endZ;
            // NOTE NEGATIVE AROUND PLAYER
            startZ = RotateZ(start, -player->angleRad);
            endZ = RotateZ(end, -player->angleRad);
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

            // Partially behind ?
            if (startZ.y <= 0 || endZ.y <= 0)
            {
                // Clip to view  left
                vec2 leftClip = Intersect(startZ.x, startZ.y, endZ.x, endZ.y, -nearSide, nearZ, -farSide, farZ);
                vec2 rightClip = Intersect(startZ.x, startZ.y, endZ.x, endZ.y, -nearSide, nearZ, farSide, farZ);
                if (startZ.y < nearZ)
                {
                    // Start was behind and was clipped to left
                    if (leftClip.y > 0)
                    {
                       startZ.x = leftClip.x;
                       startZ.y = leftClip.y;
                    }
                    else // was clipped to right
                    {

                       startZ.x = rightClip.x;
                       startZ.y = rightClip.y;
                    }
                }
                if (endZ.y < nearZ)
                {
                    if (leftClip.y > 0)
                    {
                       endZ.x = leftClip.x;
                       endZ.y = leftClip.y;
                    }
                    else
                    {
                       endZ.x = rightClip.x;
                       endZ.y = rightClip.y;
                    }
                }
            } // Clipping done

            // persective transformation
            float xScaleStart = FovH / startZ.y;
            float yScaleStart = FovV / startZ.y;
            float xScaleEnd = FovH / endZ.y;
            float yScaleEnd = FovV / endZ.y;

            // These are in 3D
            vec3 start3D;
            vec3 end3D;
            start3D.x  = (W/2)- (int)(startZ.x * xScaleStart);
            end3D.x = (W/2)- (int)(endZ.x * xScaleEnd);
            start3D.z = startZ.y;
            end3D.z = endZ.y;

            // Render if visible
            // Back side || left scissors || right scissors
            if (start3D.x >= end3D.x || end3D.x < request.leftX || start3D.x > request.rightX)
            {
                continue;
            }
            // Floor and ceiling heights relative to player Z coordinate
            // NOTE In Duke the negative is up. But we have positive up
            // Bisqwit numbers start floor from 0 and go to 36
            // so need to scale
            float ceilingY = ConvertCeilingHeight(sector->ceilingz, settings);
            float floorY = ConvertFloorHeight(sector->floorz, settings);

            float playerZ = ConvertFloorHeight(player->position2D.z, settings);

            ceilingY -= playerZ;
            floorY -= playerZ;

            // Check if this wall is a portal
            float n_ceilingY = 0;
            float n_floorY = 0;
            if (w->nextsector >= 0)
            {
                n_ceilingY = ConvertCeilingHeight(Map_GetSector(map, w->nextsector)->ceilingz, settings) - playerZ;
                n_floorY = ConvertFloorHeight(Map_GetSector(map, w->nextsector)->floorz, settings) - playerZ;
            }
            // Project ceiling and floor heights to screen coordinates
#           define Pitch(y,z) (y + z*player->Pitch)
            int startCeilingY = (H/2) - (int)Pitch(ceilingY, start3D.z) * yScaleStart; // y1a
            int startFloorY = (H/2) - (int)Pitch(floorY, start3D.z) * yScaleStart;     // y1b
            int endCeilingY = (H/2) - (int)Pitch(ceilingY, end3D.z) * yScaleEnd;       // y2a
            int endFloorY = (H/2) - (int)Pitch(floorY, end3D.z) * yScaleEnd;           // y2b

            // Project neighbor ceiling and floor
            int n_startCeilingY = (H/2) - (int)Pitch(n_ceilingY, start3D.z) * yScaleStart;
            int n_startFloorY = (H/2) - (int)Pitch(n_floorY, start3D.z) * yScaleStart;
            int n_endCeilingY = (H/2) - (int)Pitch(n_ceilingY, end3D.z) * yScaleEnd;
            int n_endFloorY = (H/2) - (int)Pitch(n_floorY, end3D.z) * yScaleEnd;

            // Finally render the wall
            // Keep inside scissors area
            int beginX = max(start3D.x, request.leftX);
            int endX = min(end3D.x, request.rightX);
            int xDiff = (end3D.x - start3D.x);
            if (xDiff == 0)
            {
                xDiff = 1;
            }

            int ceilDiff =  (endCeilingY-startCeilingY);
            int floorDiff = (endFloorY-startFloorY);

            int n_ceilDiff =  (n_endCeilingY-n_startCeilingY);
            int n_floorDiff = (n_endFloorY-n_startFloorY);

            // TODO get the rendering rectangle
            // for debug drawing from beginX and endX

            wallXPoints[wi*2] = beginX;
            wallXPoints[wi*2+1] = endX;

            for (int x = beginX; x < endX; x++)
            {
                // Calculate ceiling and floor heights at this point
                // FIXME This needs to use floating point, otherwise the step becomes 0
                int ceil_y = (x-start3D.x) * ceilDiff / xDiff + startCeilingY; // ya
                int floor_y = (x-start3D.x) * floorDiff / xDiff + startFloorY; // yb

                // Clamp inside scissors
                // FIXME This had some bug that corrupted the values
                int clampCeilY = ceil_y; //clampInt(ceil_y, ytop[x], ybottom[x]);  // cya
                int clampFloorY = floor_y;//clampInt(floor_y, ytop[x], ybottom[x]); // cyb

                // Render visible ceiling
                SetColor(CEIL_COLOR);
                //Line (x, ytop[x], clampCeilY-1);
                // Render visible floor
                SetColor(FLOOR_COLOR);
                //Line (x, clampFloorY+1, ybottom[x] );

                // Is this a portal?
                if (w->nextsector >= 0)
                {
                    int n_ceil_y = (x-start3D.x) * n_ceilDiff / xDiff + n_ceilingY;
                    int n_floor_y = (x-start3D.x) * n_floorDiff / xDiff + n_floorY;

                    // Clamp inside scissors
                    int n_clampCeilY = clampInt(n_ceil_y, ytop[x], ybottom[x]);
                    int n_clampFloorY = clampInt(n_floor_y, ytop[x], ybottom[x]);

                    // If our ceiling is higher than their ceiling, draw upper wall
                    SetColor(CEIL_COLOR);
                    Line (x, clampCeilY, n_clampCeilY-1);
                    // Shrink scissorClamp
                    ytop[x] = clampInt( max(clampCeilY, n_clampCeilY), ytop[x], H-1);
                    // If our floor is lower, draw bottom wall
                    SetColor(FLOOR_COLOR);
                    Line( x, clampFloorY, n_clampFloorY+1);

                    ybottom[x] = clampInt( min(clampFloorY, n_clampFloorY), 0, ybottom[x]);
                }
                else
                {
                    // No neighbor: draw wall
                    SetColor(WALL_COLOR);
                    Line (x, clampCeilY, clampFloorY);
                }
            } // Top down lines drawn

            // DEBUG Draw RECTANGLE
                // Calculate ceiling and floor heights at this point
                int begin_ceil_y = startCeilingY; // ya
                int begin_floor_y = startFloorY; // yb

                // Calculate ceiling and floor heights at this point
                int end_ceil_y = endCeilingY;
                int end_floor_y = endFloorY;
                SetColor(MAP_COLOR);
            Line2(beginX, begin_ceil_y, endX, end_ceil_y);
            Line2(endX, end_ceil_y, endX, end_floor_y);
            Line2(endX, end_floor_y, beginX, begin_floor_y);
            Line2(beginX, begin_floor_y, beginX, begin_ceil_y);

            // Wall is drawn
            // Add neighbor to queue
            // if there is neighbor AND scissors window has width left AND there is room in QUEUE
            if (w->nextsector >= 0 && endX >= beginX && (head + MAX_PORTAL_QUEUE+1-tail)%MAX_PORTAL_QUEUE)
            {
                (*head) = {w->nextsector, beginX, endX};
                // Move head and loop around buffer
                if ( (head++) == renderQueue + MAX_PORTAL_QUEUE)
                {
                    head = renderQueue;
                }
            }
        } // All walls of the sector have been drawn; head has moved forward

        // Make the count even??
        renderedSectorNames[request.number] += 1;

    } while(head != tail); // Render until buffer is empty: if nothing was added, they are the same
    glEnd();
}
*/

