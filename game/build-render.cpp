// Renders a duke map in 2D
#include <mgdl.h>
#include <mgdl/mgdl-vector.h>
#include <mgdl/mgdl-color.h>
#include <mgdl/mgdl-vectorfunctions.h>

#include <mgdl/ccVector/ccVector.h>

#include "build-render.h"
#include "dukemap.h"
#include "dukemath.h"
#include "player.h"
#include "opengl-render.h"

// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
// IntersectBox: Determine whether two 2D-boxes intersect.
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))

// How many portals can be waiting for drawing
#define MAX_PORTAL_QUEUE 32
static SectorRender* renderQueue = nullptr; // Circular buffer of render requests
static s16 renderQueueInserts = 0;
// These point to renderQueue
static SectorRender* head;
static SectorRender* tail;

#define MAX_SECTOR_DRAW_TIMES 4
static int* sectorDrawTimes = nullptr; // NOTE How many times each should be drawn. Additional times come from requests

static int lastSectorAmount = 0; // How many sectors were in the last map loaded

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
    return sectorDrawTimes[sectornumber] > 0;
}

// TODO give renderer inteface so can use other render than OpenGL
void BuildRender_Init(DukeMap* map, RenderSettingsOpenGL* settings3D)
{
    if (renderQueue == nullptr)
    {
        renderQueue = (SectorRender*)mgdl_AllocateGeneralMemory(sizeof(SectorRender) * MAX_PORTAL_QUEUE);
    }

    // init again if more is needed than last time
    if (sectorDrawTimes != nullptr)
    {
        if (lastSectorAmount < map->sectorAmount)
        {
           mgdl_FreeGeneralMemory(sectorDrawTimes);
           sectorDrawTimes= nullptr;
        }
    }
    if (sectorDrawTimes == nullptr)
    {
        sectorDrawTimes = (int*)mgdl_AllocateGeneralMemory(sizeof(int) * map->sectorAmount);
    }

    lastSectorAmount = map->sectorAmount;
    renderQueueInserts = 0;

    // Build other data needed by game
    for (int si = 0; si < map->sectorAmount; si++)
    {
        vec2 minp = vec2New(32000, 32000);
        vec2 maxp = vec2New(-32000, -32000);
        Sector* sector = &map->sectors[si];
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = &map->walls[sector->wallptr + wi];
            minp.x = minF(w->x, minp.x);
            minp.y = minF(w->z, minp.y);
            maxp.x = maxF(w->x, maxp.x);
            maxp.y = maxF(w->z, maxp.y);
        }
        // Found points : calculate tex coords
        float width = (maxp.x - minp.x) * settings3D->scale;
        float height = (maxp.y - minp.y) * settings3D->scale;
        float aspect = width/height;
        sector->minXZPoint = minp;
        sector->sizeXZ = vec2Subtract(maxp, minp);
        sector->maxTexCoord.x = aspect * height * settings3D->textureScale;
        sector->maxTexCoord.y = 1.0 * height * settings3D->textureScale;

    }

    // Buffer the floor and ceiling vertices: The uvs need to be calculated first
    OpenGLRender_StartCountingFloorBufferSize(map->sectorAmount, map->wallAmount);
    for (int si = 0; si < map->sectorAmount; si++)
    {
        OpenGLRender_TesselateFloor(map, si);
    }
    OpenGLRender_StopCountingFloorBufferSize();
}

void BuildRender_ExportCurrentMapToObj(DukeMap* map, const char* filename, RenderSettingsOpenGL* settings)
{
    OpenGLRender_WriteToObj(map, filename, settings);
}

void BuildRender_DrawTempSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings)
{
    glColor3f(1.0f, 1.0f, 1.0f);

    // Draw all the sprites from renderer sectors
    for (int i = 0; i < TEMP_SPRITE_AMOUNT; i++)
    {
        DSprite* sprite = &tempSprites[i];
        if (BuildRender_WasSectorDrawn(sprite->sectnum)
            && !Flag_IsSet(sprite->cstat, 1 << CSTAT_SPRITE_INVISIBLE)
            && (player->playerNumber != sprite->owner || sprite->owner > 3))
        {
            // The size comes from the size of the texture somehow
            float scaleAspect = (float)sprite->xrepeat / (float)sprite->yrepeat;
            float spriteSize = sprite->extra;
            float spriteHeight = spriteSize * scaleAspect;

            OpenGLRender_DrawSprite(sprite->position, spriteSize, spriteHeight,
                Math_DukeAngleToRad(sprite->ang), player->angleRad,
                Sprite_GetAlignment(sprite), Sprite_GetPivot(sprite),
                sprite->picnum, sprite->shade);
        }
    }

}

void BuildRender_DrawSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings)
{
    // Draw all the sprites from renderer sectors
    for (int si = 0; si < map->spriteAmount; si++)
    {
        DSprite* sprite = &map->sprites[si];
        if (BuildRender_WasSectorDrawn(sprite->sectnum)
            && !Flag_IsSet(sprite->cstat, 1 << CSTAT_SPRITE_INVISIBLE))
        {
            float scaleAspect = (float)sprite->xrepeat / (float)sprite->yrepeat;
            // The size comes from the size of the texture somehow
            float spriteSize = settings->spriteDefaultWidth;
            float spriteHeight = settings->spriteDefaultWidth * scaleAspect;

            OpenGLRender_DrawSprite(sprite->position, spriteSize, spriteHeight,
                                    Math_DukeAngleToRad(sprite->ang), player->angleRad,
                                    Sprite_GetAlignment(sprite), Sprite_GetPivot(sprite),
                                    sprite->picnum, sprite->shade);
        }
    }
}

void BuildRender_Draw3D(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    glPushMatrix();
    glScalef(settings->scale, settings->scale, settings->scale);
        OpenGLRender_StartDrawingPolygons();
            mgdl_glSetAlphaTest(true);
                BuildRender_DrawSectorWalls(player, map, settings);
                BuildRender_DrawSectorFloorsAndCeilings(player, map, settings);
                BuildRender_DrawSprites(map, player, settings);
                BuildRender_DrawTempSprites(map, player, settings);
            mgdl_glSetAlphaTest(false);
        OpenGLRender_EndDrawingPolygons();
    glPopMatrix();


    OpenGLRender_AnimateSprites();
}



void BuildRender_DrawSectorWalls(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    for (int i = 0; i < map->sectorAmount ; i++)
    {
        sectorDrawTimes[i] = 0;
    }
    for (int i = 0; i < MAX_PORTAL_QUEUE; i++)
    {
        renderQueue[i].number = -8012; // Max sector number is 4096
    }

    // Perspective projection values to cull walls that player
    // does not see
    float top = settings->near * tan( Deg2Rad(settings->FOVyDegrees/2.0f));
    float right = top * settings->aspectRatio;
    float left = -right;

    // No items in buffer
    head = renderQueue;
    tail = renderQueue;

    vec2 playerPos2 = vec2New(player->position.x, player->position.z);

    // Put player sector draw request at tail
    *head = (SectorRender){player->sectorNumber, left, right};
    renderQueueInserts++;

    // Circular buffer pointer arithmetics
    // Move the head forward or loop around
    if ( ( head += 1) == renderQueue + MAX_PORTAL_QUEUE)
    {
        head = renderQueue;
    }

    // Draw a sector and put more sectors to queue for drawing
    do {
        // Take next request from buffer:
        SectorRender request = (*tail);
        // Mark as done in queue

        // Move tail to next one
        if ( ( tail += 1) == renderQueue + MAX_PORTAL_QUEUE)
        {
            tail = renderQueue;
        }
        // If this is drawn for maximum amount of times, skip it
        if (sectorDrawTimes[request.number] >= MAX_SECTOR_DRAW_TIMES)
        {
            continue;
        }

        // Get the sector info from map
        Sector* sector = Map_GetSector(map, request.number);
        //Log_InfoF("Draw sector %d\n", request.number);

        const float ceilingY = sector->ceilingy;
        const float floorY = sector->floory;

        // Render all walls of the current sector
        // Discard those that do not face player
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = Map_GetWallInSectorPtr(map, sector, wi);
            vec2 start = vec2New(w->x, w->z);
            Wall* wend = Map_GetWallEnd(map, w);
            vec2 end =  vec2New(wend->x, wend->z);

            vec2 startZ = vec2Subtract(start, playerPos2);
            vec2 endZ = vec2Subtract(end, playerPos2);

            // The Y is how far ahead of player the point is
            // NOTE negative around player
            startZ = Vec2XZRotateY(startZ, -player->angleRad);
            endZ = Vec2XZRotateY(endZ, -player->angleRad);

            // Is the wall behind player?
            // Behind is positive Z
            if(startZ.y >= 0 && endZ.y >= 0)
            {
                // Wall is behind
                // Draw next wall
                continue;
            }
            // Clip to view frustum and check if
            // inside it
            bool startVisible = false;
            bool endVisible = false;
            if (startZ.y < 0)
            {
                startZ.x = ( startZ.x * settings->near)/-startZ.y;
                if (startZ.x <= request.limitRight)
                {
                    // start point is visible: on the left side of right frustum wall
                    startVisible = true;
                }

            }
            if (endZ.y < 0)
            {
                endZ.x = (endZ.x * settings->near)/-endZ.y;
                if (endZ.x >= request.limitLeft)
                {
                    // end point is visible
                    endVisible = true;
                }
            }

            // End of the wall is too much to left
            // or start of the wall is too much to right
            // or end is more left than start
            // This works because walls are always going clockwise around player
            if ( (endVisible || startVisible) == false)
            {
                // Neither point is visible
                continue;
            }

            // If the player sees the whole wall, but it faces away
            // OR player is very close to long wall so that start is more right
            // than end.
            if (Map_IsPointInsideWall(map, playerPos2, w) == false)
            {
                continue;

            }
            // Calculate new limits
            float newLimitLeft = maxF(request.limitLeft, startZ.x);
            float newLimitRight = minF(endZ.x, request.limitRight);


            if (newLimitLeft > newLimitRight)
            {
                // Special case where wall is long and other point is behind player
                // Line based renderer would clip the wall to player vision edge
                // This is only done if drawing player's sector
                if (request.number == player->sectorNumber)
                {
                    if (endVisible == false)
                    {
                        // Clip to right side of view
                        newLimitRight = right;
                    }
                    else if (startVisible == false)
                    {
                        newLimitLeft = left;
                    }
                }
                else
                {
                    continue;
                }
            }


            //if it was a portal Add neighbor to queue
            // if there is neighbor AND there is room in QUEUE
            OpenGLRender_DrawWall(map, w, floorY, ceilingY, settings);
            if (w->nextsector >= 0)
            {
                // When drawing walls seen from this portal,
                // limit the view cone to the wall start and end points

                // Check that there is space left to draw
                // TODO how much is one pixel? The difference must be at least that
                if (newLimitLeft < newLimitRight)
                {
                    // If there is already a request for w->nextsector
                    // combine the limits: otherwise it will be drawn only
                    // partially and the other requests are skipped
                    if  ((head + MAX_PORTAL_QUEUE+1-tail)%MAX_PORTAL_QUEUE)
                    {
                        bool addRequest = false;
                        bool passedHead = false;
                        int steps = 0; // Safety measure

                        // Start from first request. Current request is tail-1
                        SectorRender* lookAhead = (tail);

                        // Look through the buffer until at tail-1
                        while(lookAhead != (tail-1) && steps < MAX_PORTAL_QUEUE)
                        {
                            // Check when going past requests and start to wrap around
                            if (passedHead)
                            {
                                if (sectorDrawTimes[w->nextsector] == 0)
                                {
                                    // There was no request for it and
                                    // The nextsector has newer been drawn, do it now
                                    addRequest = true;
                                    break;
                                }
                            }
                            else if (lookAhead == head)
                            {
                                passedHead = true;
                            }

                            if (lookAhead->number == w->nextsector)
                            {
                                if (!passedHead)
                                {
                                    // This request is waiting, increase it's limits if
                                    // they were smaller than new ones
                                    lookAhead->limitLeft = minF(lookAhead->limitLeft, newLimitLeft);
                                    lookAhead->limitRight = maxF(lookAhead->limitRight, newLimitRight);
                                    // No need to add, since it is already waiting
                                    addRequest = false;
                                }
                                else
                                {
                                    // This is a request that has been processed.
                                    // Resubmit if new limits are bigger
                                    if (lookAhead->limitLeft > newLimitLeft || lookAhead->limitRight < newLimitRight)
                                    {
                                        // If they were, add the request again
                                        lookAhead->limitLeft = minF(lookAhead->limitLeft, newLimitLeft);
                                        lookAhead->limitRight = maxF(lookAhead->limitRight, newLimitRight);
                                        // If has passed draw limit, decrease times by one
                                        if (sectorDrawTimes[w->nextsector] >= MAX_SECTOR_DRAW_TIMES)
                                        {
                                            sectorDrawTimes -= 1;
                                        }
                                        addRequest = true;
                                    }

                                }
                                //Log_InfoF("Combined sector request: S %d |%.2f - %.2f|\n", w->nextsector, newLimitLeft, newLimitRight);

                                // Stop looking
                                break;
                            }
                            steps += 1;
                            lookAhead += 1;

                            if (lookAhead  == renderQueue + MAX_PORTAL_QUEUE)
                            {
                                // At the end, loop to start
                                lookAhead = renderQueue;
                            }
                        }

                        if (addRequest == true)
                        {
                            // The w->nextsector needs to be drawn: for first time or again
                            (*head) = {w->nextsector, newLimitLeft, newLimitRight};
                            // Move head and loop around buffer
                            if ( (++head) == renderQueue + MAX_PORTAL_QUEUE)
                            {
                                head = renderQueue;
                                renderQueueInserts++;
                            }
                            //Log_InfoF("Sector request: S %d |%.2f , %.2f|\n", w->nextsector, newLimitLeft, newLimitRight);
                        }
                    }
                }
            }
        } // All walls of the sector have been drawn; head has moved forward

        // Mark the sector as drawn
        sectorDrawTimes[request.number] += 1;

    } while(head != tail); // Render until buffer is empty: if nothing was added, they are the same
}

void BuildRender_DrawSectorFloorsAndCeilings(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    // Go through all sectors
    // Draw walls and ceilings of those
    // that had any walls drawn
    OpenGLRender_StartDrawingFloorsFromBuffer();
    for(int i = 0; i < map->sectorAmount; i++)
    {
        if (sectorDrawTimes[i] > 0)
        {
            // Get the sector info from map
            Sector* sector = Map_GetSector(map, i);
            //Log_InfoF("Draw sector %d\n", request.number);

            float ceilingY = sector->ceilingy;
            float floorY = sector->floory;
            // Draw the floor and ceiling with tesselation
            bool floor = true;
            do {
                // Draw only floors and ceilings the player can see
                if ((floor && player->position.y >= floorY) ||
                    (!floor && player->position.y <= ceilingY))
                {
                    OpenGLRender_DrawFloorOrCeiling(map, i, floor);
                }
                floor = !floor;
                // First round: floor is false
                // Second round: floor is true
            } while(floor == false);
        }
    }
}


void BuildRender_DrawSectorRequests(RenderSettingsOpenGL* settings3D)
{
    Color4f blueColor = Color_Create4f(0.1f, 0.1f, 0.5f, 1.0f);
    Color4f yellowColor = Color_Create4f(0.1f, 0.5f, 0.5f, 1.0f);
    Font* df = DefaultFont_GetDefaultFont();
    int H = mgdl_GetScreenHeight();
    int W = mgdl_GetScreenWidth();

    for (int i = 0; i < MAX_PORTAL_QUEUE; i++)
    {
        SectorRender* r = &renderQueue[i];
        if (r->number > -4096)
        {
            int number = r->number;
            if (i % 2 == 0) {
                OpenGLRender_SetColor4f(blueColor);
            }
            else
            {
                OpenGLRender_SetColor4f(yellowColor);
            }
            glBegin(GL_LINES);
            int lineleft = W/2 + (r->limitLeft/settings3D->near) * W/2;
            int lineright = W/2 + (r->limitRight/settings3D->near) * W/2;
            int lineY = 16 + (i * 18);
            OpenGLRender_Line2(lineleft, H, lineleft, 0);
            OpenGLRender_Line2(lineright, H, lineright, 0);
            OpenGLRender_Line2(lineleft, lineY, lineright, lineY);
            glEnd();

            Font_Printf(df, (i%2==0) ? &blueColor : &yellowColor, lineleft + (lineright-lineleft)/2, lineY, 16, "%d", number);
        }
        if (i>=renderQueueInserts)
        {
            break;
        }
    }
}


void BuildRender_DrawTopDown(Player* players, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D)
{
    Font* df = DefaultFont_GetDefaultFont();
    int H = mgdl_GetScreenHeight();
    int W = mgdl_GetScreenWidth();

    vec2 firstPlayerPos2 = vec2New(players[0].position.x, players[0].position.z);

    vec2 collision_forward = vec2New(0.0, WORLD_FORWARD.z * settings2D->collisionLength);

    collision_forward  = Vec2XZRotateY(collision_forward,  Deg2Rad(settings2D->collisionAngleDeg));
    vec2 collisionEnd = vec2Add(settings2D->collisionPoint, collision_forward);

    bool collisionMiss = true;
    vec2 collisionOut;

        Color4f* whiteColor = Color_GetDefaultColor(Color_White);
        Color4f* greenColor = Color_GetDefaultColor(Color_Green);
        Color4f portalColor = Color_Create4f(0.5f, 0.1f, 0.1f, 1.0f);
        Color4f wallColor = Color_Create4f(0.75f, 0.75f, 0.75f, 1.0f);

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
            // Turn the world around player
            glScalef(settings3D->scale, settings3D->scale, 1);

            if (settings2D->rotateMap)
            {
                glRotatef(Rad2Deg( (-players[0].angleRad )), 0, 0, WORLD_FORWARD.z);
            }

            if (settings2D->centerMapToPlayer)
            {
                // Keep player at center of screen
                glTranslatef(-firstPlayerPos2.x, -firstPlayerPos2.y, 0);
            }

            glBegin(GL_LINES);

            // Draw Grid in grey under everything else
            glColor3f(0.2f, 0.2f, 0.2f);
            if (settings2D->gridSize > 0)
            {
                float antiscale = 1.0f / settings3D->scale;
                float gz = floorf(settings2D->gridSize) * antiscale;
                float dx = (-10 * gz);
                float dy = (-10 * gz);
                for(int x = 0; x < 20; x++)
                {
                    OpenGLRender_Line2(dx + gz * x, dy,
                                       dx + gz * x, dy + gz * 20);
                }
                for (int y = 0; y < 20; y++)
                {
                    OpenGLRender_Line2(dx, dy + gz * y,
                                        dx + gz * 20, dy + gz * y);

                }
            }
            glLineWidth(4.0f);

            // Draw origo
            OpenGLRender_SetColor(Color_White);
            OpenGLRender_Line2(0, -10, 0 ,10);
            OpenGLRender_Line2(-10, 0, 10, 0);

            // Draw WORLD_FORWARD and WORLD_RIGHT
            int axisLength = 1024;
            OpenGLRender_SetColor(Color_Red);
            OpenGLRender_Line2(0, 0, WORLD_RIGHT.x * axisLength, WORLD_RIGHT.z * axisLength);
            OpenGLRender_SetColor(Color_Blue);
            OpenGLRender_Line2(0, 0, WORLD_FORWARD.x * axisLength, WORLD_FORWARD.z * axisLength);

            // DRAW WALLS
            ////////////////////////////
            settings2D->collisionInsideSector = -1;
            for(int si = 0; si < map->sectorAmount; si++)
            {
                if (settings2D->drawOneSector >=0 && settings2D->drawOneSector != si)
                { continue; }

                // Get the sector info from map
                Sector* sector = Map_GetSector(map, si);
                for (s16 wi = 0; wi < sector->wallnum; wi++)
                {
                    if (settings2D->drawOneWall >=0 && settings2D->drawOneWall != wi)
                    { continue; }

                    Wall* w = Map_GetWallInSector(map, si, wi);
                    vec2 start = vec2New(w->x, w->z);
                    Wall* wend = Map_GetWallEnd(map, w);
                    vec2 end =  vec2New(wend->x, wend->z);

                    if (settings2D->movePlayer == false)
                    {
                        if (Map_IsPointInsideSectorOG(map, settings2D->collisionPoint, si))
                        {
                            settings2D->collisionInsideSector = si;
                        }
                        if (Map_FindIntersectionWithWall(map,  settings2D->collisionPoint, collisionEnd, w, &collisionOut))
                        {
                            OpenGLRender_DrawDot(collisionOut, 48, DefaultColor::Color_White);
                        }
                        else
                        {
                            collisionMiss = true;
                        }
                        if (Map_IsPointInsideWall(map, settings2D->collisionPoint, w))
                        {
                            OpenGLRender_SetColor(Color_Green);
                        }
                        else
                        {
                            OpenGLRender_SetColor(Color_Black);
                        }
                    }
                    else
                    {
                        if (w->nextsector < 0)
                        {
                            OpenGLRender_SetColor4f(wallColor);
                        }
                        else
                        {
                            OpenGLRender_SetColor4f(portalColor);
                        }
                    }

                    OpenGLRender_Line2(start.x, start.y, end.x, end.y);
                    if (settings2D->drawNormals)
                    {
                        vec2 m = Map_GetWallMiddle(map, w);
                        vec2 N = vec2Multiply( Map_GetWallNormal(map, w), 32 );
                        OpenGLRender_Line2(m.x, m.y, m.x + N.x, m.y + N.y);
                    }
                }
            }
            glEnd(); // end walls

            // DRAW SPRITES
            // //////////////////////

            if (settings2D->drawSprites)
            {
                glBegin(GL_LINES);

                float spriteSize = 64;
                DefaultColor spriteColor = Color_Red;
                OpenGLRender_SetColor(spriteColor);
                vec2 spriteForward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
                float spriteWidth = settings3D->spriteDefaultWidth/2;
                for(int spi =  0; spi < map->spriteAmount; spi++)
                {
                    DSprite* sprite = &map->sprites[spi];
                    vec2 spos2 = vec2New(sprite->position.x, sprite->position.z);
                    SpriteAlignment al = Sprite_GetAlignment(sprite);

                    float angle = Math_DukeAngleToRad(sprite->ang);
                    if (al == Sprite_FACE)
                    {
                        angle = players[0].angleRad + Deg2Rad(180);
                    }
                    vec2 spriteDir = Vec2XZRotateY(spriteForward, angle);

                    if (al == Sprite_FLOOR)
                    {
                        OpenGLRender_DrawDot(spos2, spriteSize, spriteColor);
                    }
                    else
                    {
                        OpenGLRender_DrawDot(spos2, spriteSize, spriteColor);
                        vec2 spriteEnd = vec2Add(spos2, vec2Multiply(spriteDir, spriteWidth));
                        OpenGLRender_Line2(spos2.x, spos2.y, spriteEnd.x, spriteEnd.y);
                    }
                }
                glEnd();
            }


            // DRAW WALL NUMBERS and SECTOR NUMBERS
            glPushMatrix();
                glScalef(1, -1, 1);

            int numberSize = 128;
            for(int si = 0; si < map->sectorAmount; si++)
            {
                if (settings2D->drawOneSector >=0 && settings2D->drawOneSector != si) { continue; }

                // Get the sector info from map
                Sector* sector = Map_GetSector(map, si);

                if (settings2D->drawSectorNumbers)
                {
                    int sx = sector->minXZPoint.x + sector->sizeXZ.x/2;
                    int sy = -(sector->minXZPoint.y + sector->sizeXZ.y/2);
                    int numbers = 1;
                    if (si >= 10)
                    {
                        numbers += 1;
                        if (si >= 100)
                        {
                            numbers += 1;
                        }
                    }
                    Draw2D_RectWH(sx, sy, numberSize * numbers, numberSize, Color_GetDefaultColor(Color_Black));

                    if (sectorDrawTimes[si] > 0)
                    {
                        // Draw in green if rendered at least once
                        Font_Printf(df, greenColor, sx, sy, numberSize, "%d", si);
                    }
                    else
                    {
                        // Draw in white if not rendered
                        Font_Printf(df, whiteColor, sx, sy, numberSize, "%d", si);

                    }
                }

                if (settings2D->drawWallNumbers)
                {
                    // Render all walls of the current sector
                    // Discard those that do not face player
                    for (s16 wi = 0; wi < sector->wallnum; wi++)
                    {
                        if (settings2D->drawOneWall >=0 && settings2D->drawOneWall != wi) { continue; }

                        Wall* w = Map_GetWallInSector(map, si, wi);
                        vec2 start = vec2New(w->x, w->z);
                        vec2 middle = Map_GetWallMiddle(map, w);

                        int mapWi =sector->wallptr+wi;

                        int tx = middle.x - numberSize/2;
                        int ty = -middle.y + numberSize/2;
                        int numbers = 1;
                        if (mapWi >= 10)
                        {
                            numbers += 1;
                            if (mapWi >= 100)
                            {
                                numbers += 1;
                            }
                        }
                        ty -= (numberSize * (numbers-1));

                        Draw2D_RectWH(tx, ty, numberSize*numbers, numberSize, Color_GetDefaultColor(Color_Black));

                        if (w->nextsector < 0)
                        {
                            Font_Printf(df, &wallColor, tx, ty, numberSize, "%d", mapWi);
                        }
                        else
                        {
                            Font_Printf(df, &portalColor, tx, ty, numberSize, "%d", mapWi);
                        }
                        if (settings2D->drawOneWall == wi && settings2D->drawOneSector == si)
                        {

                            if (collisionMiss)
                            {
                                Font_Printf(df, whiteColor, start.x, start.y + 64+32, 32, "U: %.2f", collisionOut.y);
                                Font_Printf(df, whiteColor, start.x, start.y + 64, 32, "T: %.2f", collisionOut.x);
                            }
                            else
                            {
                                Font_Printf(df, whiteColor, start.x, start.y + 64+32, 32, "Y: %.2f", collisionOut.y);
                                Font_Printf(df, whiteColor, start.x, start.y + 64, 32, "X: %.2f", collisionOut.x);
                            }
                        }
                    }
                }
            }
            glPopMatrix(); // NUMBERS
        glPopMatrix(); // WALLS


        // DRAW PLAYERS
        // ////////////

    glPushMatrix();
        for (int pi = 0; pi < settings2D->drawPlayersAmount; pi++)
        {
            Player* player = &players[pi];
            vec2 playerPos2 = vec2New(player->position.x, player->position.z);

            glScalef(settings3D->scale, settings3D->scale, 1);
            glBegin(GL_LINES);

            vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
            if (settings2D->rotateMap == false)
            {
                forward = Vec2XZRotateY(forward, player->angleRad);
            }
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
                if (settings2D->centerMapToPlayer && pi == 0)
                {
                    playerPos2 = vec2Zero();
                }
                OpenGLRender_DrawDot(playerPos2, dotSize,pc );
                OpenGLRender_SetColor(Color_Red);
                forward = vec2Multiply(forward, player->radius * 2);
                vec2 end = vec2Add(playerPos2, forward);

                glVertex2i(playerPos2.x,playerPos2.y);
                glVertex2f(end.x, end.y);

                vec2 sideLeft = Vec2XZRotateY(forward,  M_PI * 3.0f/4.0f);
                vec2 sideRight = Vec2XZRotateY(forward, -M_PI * 3.0f/4.0f);
                sideLeft = vec2Add(sideLeft, end);
                sideRight = vec2Add(sideRight, end);

                glVertex2f(end.x, end.y);
                glVertex2f(sideLeft.x, sideLeft.y);

                glVertex2f(end.x, end.y);
                glVertex2f(sideRight.x, sideRight.y);
            }
            else
            {
                OpenGLRender_DrawDot(playerPos2, dotSize,pc );
                OpenGLRender_SetColor(Color_Black);

                glVertex2i(playerPos2.x,playerPos2.y);
                glVertex2f(collisionEnd.x, collisionEnd.y);
            }
        }

        glEnd();
        glPopMatrix(); // Player
    glPopMatrix(); // Whole map view
    glLineWidth(1.0f);
}



