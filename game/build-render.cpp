// Renders a duke map in 2D
#include <mgdl.h>
#include <mgdl/mgdl-vector.h>
#include <mgdl/mgdl-vectorfunctions.h>

#include <mgdl/ccVector/ccVector.h>

#include "build-render.h"
#include "dukemap.h"


// TODO move to mgdl util
#define min(a,b)             (((a) < (b)) ? (a) : (b)) // min: Choose smaller of two scalars.
#define max(a,b)             (((a) > (b)) ? (a) : (b)) // max: Choose greater of two scalars.
#define clamp(a, mi,ma)      min(max(a,mi),ma)         // clamp: Clamp value into set range.


// Array the size of screen Width to
// store the up and bottom borders
// of draw area
int* ytop;
int* ybottom;

// How many portals can be waiting for drawing
#define MAX_PORTAL_QUEUE 32
SectorRender* renderQueue; // Circular buffer of render requests
// These point to renderQueue
SectorRender* head;
SectorRender* tail;

int* renderedSectorNames; // NOTE this is related to all sectors in map

static int lastSectorAmount = 0;

void Init(DukeMap* map)
{
    renderQueue = (SectorRender*)malloc(sizeof(SectorRender) * MAX_PORTAL_QUEUE);

    // init again if more is needed
    renderedSectorNames = (int*)malloc(sizeof(int) * map->sectorAmount);
    lastSectorAmount = map->sectorAmount;

    ytop = (int*)malloc(sizeof(int) * mgdl_GetScreenWidth());
    ybottom = (int*)malloc(sizeof(int) * mgdl_GetScreenWidth());
}

void InitArrays()
{
    memset(renderedSectorNames, 0, lastSectorAmount);
    memset(ytop, 0, mgdl_GetScreenWidth());
    memset(ybottom, mgdl_GetScreenHeight()-1, mgdl_GetScreenWidth());

    // No items in buffer
    head = renderQueue;
    tail = renderQueue;
}

// find intersection of line A-B with near and far limits
static vec2 Intersect(vec3 A, vec3 B, float nearSide, float nearZ, float farSide, float farZ)
{

}

void DrawFirstPerson(Player* player, DukeMap* map)
{
    const int W = mgdl_GetScreenWidth();
    const int H = mgdl_GetScreenHeight();

    // TODO Calculate these to match window size and OpenGL
    const float FovH = 0.75f * H; // Horizontal
    const float FovV = 0.2f * H; // Vertical
    const float nearZ = 1e-4f;
    const float farZ = 5.0f;
    const float nearSide = 1e-4f;
    const float farSide = 20.0f; // From screen widh and FOV
    InitArrays();

    // Put the player's sector to head of buffer
    // Draw whole screen: ytop and ybottom are at initial values
    *head = (SectorRender){player->sectorNumber, 0, W-1};
    // Circular buffer pointer arithmetics
    // Next request is put towards the tail
    if ( ( head += 1) == renderQueue + MAX_PORTAL_QUEUE)
    {
        head = renderQueue;
    }

    const float player_cos = cos(player->angleRad);
    const float player_sin = sin(player->angleRad);

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
        for (int wi = 0; wi < sector->wallAmount; wi++)
        {
            Wall* w = Sector_GetWall(sector, wi);
            vec2 start = w->start;
            vec2 end = w->end;
            // Rotate around player
            vec2 playerPos2 = (vec2){{player->position.x, player->position.y}};
            start = vec2Subtract(start, playerPos2);
            end = vec2Subtract(end, playerPos2);

            // The Z is how far ahead of player the point is
            vec3 start3;
            vec3 end3;
            start3.x = start.x * player_sin - start.y * player_cos;
            start3.z = start.x * player_cos - start.y * player_sin;
            end3.x = end.x * player_sin - end.y * player_cos;
            end3.z = end.x * player_cos - end.y * player_sin;
            // Is the wall behind player?
            if(start3.z <= 0 && end3.z <= 0)
            {
                // Draw next wall
                continue;
            }

            // Partially behind ?
            if (start3.z <= 0 || end3.z <= 0)
            {
                // Clip to view  left
                vec2 leftClip = Intersect(start3, end3, -nearSide, nearZ, -farSide, farZ);
                vec2 rightClip = Intersect(start3, end3, -nearSide, nearZ, farSide, farZ);
                if (start3.z < nearZ)
                {
                    // Start was behind and was clipped to left
                    if (leftClip.y > 0)
                    {
                       start3.x = leftClip.x;
                       start3.z = leftClip.y;
                    }
                    else // was clipped to right
                    {

                       start3.x = rightClip.x;
                       start3.z = rightClip.y;
                    }
                }
                if (end3.z < nearZ)
                {
                    if (leftClip.y > 0)
                    {
                       end3.x = leftClip.x;
                       end3.z = leftClip.y;
                    }
                    else
                    {
                       end3.x = rightClip.x;
                       end3.z = rightClip.y;
                    }
                }
            } // Clipping done

            // persective transformation
            float xScaleStart = FovH / start3.z;
            float yScaleStart = FovV / start3.z;
            float xScaleEnd = FovH / end3.z;
            float yScaleEnd = FovV / end3.z;

            // These are in 3D
            vec3 start3D;
            vec3 end3D;
            start3D.x  = (W/2)- (int)(start3.x * xScaleStart);
            end3D.x = (W/2)- (int)(end3.x * xScaleEnd);
            // Render if visible
            // Back side || left scissors || right scissors
            if (start3D.x >= end3D.x || end3D.x < request.leftX || start3D.x > request.rightX)
            {
                continue;
            }
            // Floor and ceiling heights relative to player Z coordinate
            float ceilingY = sector->ceilingZ - player->position.z;
            float floorY = sector->floorZ - player->position.z;

            // Check if this wall is a portal
            float n_ceilingY = 0;
            float n_floorY = 0;
            if (w->neighborSector >= 0)
            {
                n_ceilingY = Map_GetSector(map, w->neighborSector)->ceilingZ - player->position.z;
                n_floorY = Map_GetSector(map, w->neighborSector)->floorZ - player->position.z;
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

            int ceilDiff =  (endCeilingY-startCeilingY);
            int floorDiff = (endFloorY-startFloorY);

            int n_ceilDiff =  (n_endCeilingY-n_startCeilingY);
            int n_floorDiff = (n_endFloorY-n_startFloorY);

            // TODO get the rendering rectangle
            // for debug drawing from beginX and endX

            for (int x = beginX; x < endX; x++)
            {
                // Calculate ceiling and floor heights at this point
                int ceil_y = (x-start3D.x) * ceilDiff / xDiff + ceilingY; // ya
                int floor_y = (x-start3D.x) * floorDiff / xDiff + floorY; // yb

                // Clamp inside scissors
                int clampCeilY = clamp(ceil_y, ytop[x], ybottom[x]);  // cya
                int clampFloorY = clamp(floor_y, ytop[x], ybottom[x]); // cyb

                // Render visible ceiling
                // Line (x, ytop[x], clampCeilY-1)
                // Render visible floor
                // Line (x, clampFloorY+1, ybottom[x] )

                // Is this a portal?
                if (w->neighborSector >= 0)
                {
                    int n_ceil_y = (x-start3D.x) * n_ceilDiff / xDiff + n_ceilingY;
                    int n_floor_y = (x-start3D.x) * n_floorDiff / xDiff + n_floorY;

                    // Clamp inside scissors
                    int n_clampCeilY = clamp(n_ceil_y, ytop[x], ybottom[x]);
                    int n_clampFloorY = clamp(n_floor_y, ytop[x], ybottom[x]);

                    // If our ceiling is higher than their ceiling, draw upper wall
                    // Line (x, clampCeilY, n_clampCeilY-1)
                    // Shrink scissors
                    ytop[x] = clamp( max(clampCeilY, n_clampCeilY), ytop[x], H-1);
                    // If our floor is lower, draw bottom wall
                    // Line( x, clampFloorY, n_clampFloorY+1)

                    // Shrink scissors
                    ybottom[x] = clamp( min(clampFloorY, n_clampFloorY), 0, ybottom[x]);
                }
                else
                {
                    // No neighbor: draw wall
                    // Line (x, clampCeilY, clampFloorY);
                }
            } // Top down lines drawn

            // Wall is drawn
            // Add neighbor to queue
            // if there is neighbor AND scissors window has width left AND there is room in QUEUE
            if (w->neighborSector >= 0 && endX >= beginX && (head + MAX_PORTAL_QUEUE+1-tail)%MAX_PORTAL_QUEUE)
            {
                (*head) = (SectorRender){w->neighborSector, beginX, endX};
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
}
