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
// These point to renderQueue
SectorRender* head;
SectorRender* tail;

int* renderedSectorNames; // NOTE this is related to all sectors in map

static int lastSectorAmount = 0;
static int W;
static int H;

static const int FLOOR_COLOR = 99;
static const int CEIL_COLOR = 100;
static const int PORTAL_COLOR = 101;
static const int MAP_COLOR = 102;
static const int PLAYER_COLOR = 103;
static const int WALL_COLOR = 104;

static const int playerSize = 256;

// OpenGL
Texture* checkers;

static void SetColor(int color)
{
    if (color == WALL_COLOR)
    {
        Palette* p = Palette_GetDefault();
        color = 4;
        Color4f c = Palette_GetColor4f(p, color);
        glColor3f(c.red, c.green, c.blue);
    }
    else
    {
        Color4f* c;
        switch(color)
        {
            case FLOOR_COLOR:
                c = Color_GetDefaultColor(Color_Red);
                glColor4fv(&c->red);
                break;
            case CEIL_COLOR:
                c = Color_GetDefaultColor(Color_Blue);
                glColor4fv(&c->red);
                break;
            case PORTAL_COLOR:
                c = Color_GetDefaultColor(Color_Green);
                glColor4fv(&c->red);
                break;
            case MAP_COLOR:
                c = Color_GetDefaultColor(Color_White);
                glColor4fv(&c->red);
                break;
            case PLAYER_COLOR:
                c = Color_GetDefaultColor(Color_Black);
                glColor4fv(&c->red);
                break;
        }
    }
}

static void Line2(int x1, int z1, int x2, int z2)
{
    glVertex2i(x1, z1);
    glVertex2i(x2, z2);
}

void BuildRender_Init(DukeMap* map)
{
    H = mgdl_GetScreenHeight();
    W = mgdl_GetScreenWidth();
    renderQueue = (SectorRender*)malloc(sizeof(SectorRender) * MAX_PORTAL_QUEUE);

    // init again if more is needed
    renderedSectorNames = (int*)malloc(sizeof(int) * map->sectorAmount);
    lastSectorAmount = map->sectorAmount;

    checkers = Texture_GenerateCheckerBoard();
}

void InitArrays()
{
    memset(renderedSectorNames, 0, lastSectorAmount);

    // No items in buffer
    head = renderQueue;
    tail = renderQueue;
}
/*
// find intersection of line A-B with near and far limits
static vec2 Intersect(vec3 A, vec3 B, float nearSide, float nearZ, float farSide, float farZ)
{

}
*/
void BuildRender_DrawOpenGL(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    InitArrays();

    vec2 playerPos2 = vec2New(player->positionOpenGL.x, player->positionOpenGL.y);
    //playerPos2.x *= settings->scaleXZ;
    //playerPos2.y *= settings->scaleXZ;
    // Put the pla
    // Draw whole screen: ytop and ybottom are at initial values
    *head = {player->sectorNumber, 0, W-1};
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

        // TODO Use gluTesselateion to draw floor and ceiling
        // Render all walls of the current sector
        // Discard those that do not face player
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSector(map, request.number, wi);
            vec2 start = w->start;
            vec2 end =  w->end;
            /*

            start = vec2Subtract(start, playerPos2);
            end = vec2Subtract(end, playerPos2);

            // The Y is how far ahead of player the point is
            vec2 startZ;
            vec2 endZ;
            // NOTE negative around player
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
                //continue;
            }
            */

            // Wall is drawn
            // if it was a portal Add neighbor to queue
            // if there is neighbor AND AND there is room in QUEUE
            if (w->nextsector >= 0 && (head + MAX_PORTAL_QUEUE+1-tail)%MAX_PORTAL_QUEUE)
            {
                // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
                Sector* neighbor = Map_GetSector(map, w->nextsector);
                int n_floorY = neighbor->floorz;
                int n_ceilingY = neighbor->ceilingz;

                // if this floor height is less than adjacent: Greate wall in between: goes up
                if (floorY < n_floorY)
                {
                    // Build Triangles for stair
                glBegin(GL_TRIANGLES);
                SetColor(WALL_COLOR);
                    glVertex3f(start.x, floorY, start.y);
                    glVertex3f(end.x, floorY, end.y);
                    glVertex3f(end.x, n_floorY, end.y);

                    glVertex3f(end.x, n_floorY, end.y);
                    glVertex3f(start.x, n_floorY, start.y);
                    glVertex3f(start.x, floorY, start.y);
                    glEnd();

                }

                // Ceiling:
                // If this ceiling is higher than adjacent: Greate wall in between: goes down
                if (ceilingY > n_ceilingY)
                {
                glBegin(GL_TRIANGLES);
                SetColor(WALL_COLOR);
                    // Build Triangles for arc
                    glVertex3f(start.x, n_ceilingY, start.y);
                    glVertex3f(end.x, n_ceilingY, end.y);
                    glVertex3f(end.x, ceilingY, end.y);

                    glVertex3f(end.x, ceilingY, end.y);
                    glVertex3f(start.x, ceilingY, start.y);
                    glVertex3f(start.x, n_ceilingY, start.y);
                glEnd();
                }

                (*head) = {w->nextsector, 0, W};
                // Move head and loop around buffer
                if ( (head++) == renderQueue + MAX_PORTAL_QUEUE)
                {
                    head = renderQueue;
                }
            }
            else
            {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, checkers->textureId);
                glBegin(GL_TRIANGLES);
                SetColor(MAP_COLOR);
                    // Build Triangles for wall
                    glTexCoord2f(0.0f, 0.0f); // 0
                    glVertex3f(start.x, floorY, start.y); // 0

                    glTexCoord2f(1.0f, 0.0f); // 1
                    glVertex3f(end.x, floorY, end.y);   // 1

                    glTexCoord2f(1.0f, 1.0f); // 2
                    glVertex3f(end.x, ceilingY, end.y); // 2

                    glTexCoord2f(1.0f, 1.0f); // 2
                    glVertex3f(end.x, ceilingY, end.y);  // 2

                    glTexCoord2f(0.0f, 1.0f); // 3
                    glVertex3f(start.x, ceilingY, start.y); // 3

                    glTexCoord2f(0.0f, 0.0f); // 0
                    glVertex3f(start.x, floorY, start.y); // 0

                glEnd();
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_TEXTURE_2D);

                glBegin(GL_LINE_LOOP);
                    SetColor(FLOOR_COLOR);
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

void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D)
{
    Font* df = DefaultFont_GetDefaultFont();
    vec2 playerPos2 = vec2New(player->positionOpenGL.x, player->positionOpenGL.y);

    // WALL NUMBERS
    ////////////////////

    Color4f* c = Color_GetDefaultColor(Color_Red);

    // The whole map zoom
    glPushMatrix();
        glTranslatef(W/2 + settings2D->mapOffset.x, H/2 + settings2D->mapOffset.y, 0.0f);
        //glRotatef(180, 0.0f, 0.0f, 1.0f); // Mapster32 Y is down. OpenGL Y is up
        glScalef(settings2D->mapZoom, settings2D->mapZoom, 1);


            // Draw origo
            SetColor(MAP_COLOR);
            Line2(0, -10, 0 ,10);
            Line2(-10, 0, 10, 0);
        // The walls zoom
        glPushMatrix();
            glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);

            // DRAW WALLS
            ////////////////////////////
            glBegin(GL_LINES);
            SetColor(FLOOR_COLOR);
            // Keep player at center

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
                    //start = vec2Multiply(start, settings2D->scaleXZ);
                    //end = vec2Multiply(end, settings2D->scaleXZ);
                    // Rotate around player
                    //start = vec2Subtract(start, playerPos2);
                    //end = vec2Subtract(end, playerPos2);

                    Line2(start.x, start.y, end.x, end.y);
                }
            }
            glEnd(); // end walls

            // DRAW WALL NUMBERS
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

                    Font_Printf(df, c, start.x-8, start.y-8, 32, "%d", sector->wallptr+wi);
                }
            }
        glPopMatrix(); // WALLS

        // DRAW PLAYER
        // ////////////

    glPushMatrix();

        glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);
        glBegin(GL_LINES);
            if (mgdl_GetElapsedFrames() % 30 == 0)
            {
                SetColor(MAP_COLOR);
            }
            else
            {
                SetColor(PLAYER_COLOR);
            }
            float dotSize = playerSize/3;
            glVertex2i(playerPos2.x,playerPos2.y - dotSize);
            glVertex2i(playerPos2.x + dotSize,playerPos2.y);

            glVertex2i(playerPos2.x + dotSize,playerPos2.y);
            glVertex2i(playerPos2.x ,playerPos2.y + dotSize);

            glVertex2i(playerPos2.x ,playerPos2.y + dotSize);
            glVertex2i(playerPos2.x - dotSize,playerPos2.y);

            glVertex2i(playerPos2.x - dotSize,playerPos2.y);
            glVertex2i(playerPos2.x,playerPos2.y - dotSize);

            // PLAYER ARROW
            // //////////////////
            SetColor(FLOOR_COLOR);
            vec2 forward = vec2New(player->direction.x, player->direction.y);
            forward = vec2Multiply(forward, playerSize);

            if (settings2D->mapYDown)
            {
//               forward = vec2Negate(forward);
            }
            vec2 end = forward;

            end = vec2Add(playerPos2, end);

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

        glEnd();
        glPopMatrix(); // Player

    glPopMatrix(); // Whole map view
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

