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


// TODO move to mgdl util
#define min(a,b)             (((a) < (b)) ? (a) : (b)) // min: Choose smaller of two scalars.
#define max(a,b)             (((a) > (b)) ? (a) : (b)) // max: Choose greater of two scalars.
#define vxs(x0,y0, x1,y1)    ((x0)*(y1) - (x1)*(y0))   // vxs: Vector cross product
// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
// IntersectBox: Determine whether two 2D-boxes intersect.
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))

// TODO move to util
/*
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
*/



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

void BuildRender_Init(DukeMap* map, RenderSettingsOpenGL* settings3D)
{
    H = mgdl_GetScreenHeight();
    W = mgdl_GetScreenWidth();
    if (renderQueue == nullptr)
    {
        renderQueue = (SectorRender*)malloc(sizeof(SectorRender) * MAX_PORTAL_QUEUE);
    }

    // init again if more is needed
    // TODO Just init to 4096 ?
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



    // Build other data needed by game
    for (int si = 0; si < map->sectorAmount; si++)
    {
        vec2 minp = vec2New(32000, 32000);
        vec2 maxp = vec2New(-32000, -32000);
        Sector* sector = &map->sectors[si];
        for (s16 wi = 0; wi < sector->wallnum; wi++)
        {
            Wall* w = &map->walls[sector->wallptr + wi];
            minp.x = min(w->x, minp.x);
            minp.y = min(w->z, minp.y);
            maxp.x = max(w->x, maxp.x);
            maxp.y = max(w->z, maxp.y);
        }
        // Found points : calculate tex coords
        float width = (maxp.x - minp.x) * settings3D->scaleXZ;
        float height = (maxp.y - minp.y) * settings3D->scaleXZ;
        float aspect = width/height;
        sector->minXZPoint = minp;
        sector->sizeXZ = vec2Subtract(maxp, minp);
        sector->maxTexCoord.x = aspect * height * settings3D->textureScale;
        sector->maxTexCoord.y = 1.0 * height * settings3D->textureScale;
    }
    OpenGLRender_Init();
}

void BuildRender_DrawTempSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings)
{
    glColor3f(1.0f, 1.0f, 1.0f);

    // Draw all the sprites from renderer sectors
    for (int i = 0; i < TEMP_SPRITE_AMOUNT; i++)
    {
        DSprite* sprite = &tempSprites[i];
        if (renderedSectorNames[sprite->sectnum] > 0
            && !Flag_IsSet(sprite->cstat, 1 << CSTAT_SPRITE_INVISIBLE)
            && (player->playerNumber != sprite->owner || sprite->owner > 3))
        {
            // Keep the texture aspect correct
            float scaleAspect = settings->scaleXZ / settings->scaleY;

            // TODO xrepeat and yrepeat adjust the aspect
            // The size comes from the size of the texture somehow
            float spriteSize = sprite->extra;
            float spriteHeight = spriteSize * scaleAspect;

            OpenGLRender_DrawSprite(sprite->position, spriteSize, spriteHeight,
                Math_DukeAngleToRad(sprite->ang), player->angleRad,
                Sprite_GetAlignment(sprite), Sprite_GetPivot(sprite),
                sprite->picnum);
        }
    }

    OpenGLRender_AnimateSprites();
}

void BuildRender_DrawSprites(DukeMap* map, Player* player, RenderSettingsOpenGL* settings)
{
    // Draw all the sprites from renderer sectors
    for (int si = 0; si < map->spriteAmount; si++)
    {
        DSprite* sprite = &map->sprites[si];
        if (renderedSectorNames[sprite->sectnum] > 0
            && !Flag_IsSet(sprite->cstat, 1 << CSTAT_SPRITE_INVISIBLE))
        {

            // Keep the texture aspect correct
            float scaleAspect = settings->scaleXZ / settings->scaleY;

            // TODO xrepeat and yrepeat adjust the aspect
            // The size comes from the size of the texture somehow
            float spriteSize = settings->spriteDefaultWidth;
            float spriteHeight = settings->spriteDefaultWidth * scaleAspect;

            SpriteAlignment al = Sprite_GetAlignment(sprite);
            /*
            //float pushOut = 0;
            switch(al)
            {
                case Sprite_FACE: // nop
                    break;
                case Sprite_WALL:
                    pushOut = settings->scaleXZ;
                    break;
                case Sprite_FLOOR:
                    pushOut = settings->scaleY;
                    break;

            }
            */

            OpenGLRender_DrawSprite(sprite->position, spriteSize, spriteHeight,
                                    Math_DukeAngleToRad(sprite->ang), player->angleRad,
                                    al , Sprite_GetPivot(sprite),
                                    sprite->picnum);
        }
    }
}

void BuildRender_Draw3D(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    glPushMatrix();
    glScalef(settings->scaleXZ, settings->scaleY, settings->scaleXZ);
        BuildRender_DrawSectors(player, map, settings);
        mgdl_glSetTransparency(true);
        BuildRender_DrawSprites(map, player, settings);
        BuildRender_DrawTempSprites(map, player, settings);
        mgdl_glSetTransparency(false);
    glPopMatrix();
}


void BuildRender_DrawSectors(Player* player, DukeMap* map, RenderSettingsOpenGL* settings)
{
    for (int i = 0; i < map->sectorAmount ; i++)
    {
        renderedSectorNames[i] = 0;
    }

    // No items in buffer
    head = renderQueue;
    tail = renderQueue;

    vec2 playerPos2 = vec2New(player->position.x, player->position.z);
    *head = {player->sectorNumber};
    renderQueueInserts++;
    // Circular buffer pointer arithmetics
    // Next request is put towards the tail
    if ( ( head += 1) == renderQueue + MAX_PORTAL_QUEUE)
    {
        head = renderQueue;
    }


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
        if (renderedSectorNames[request.number] > 0) // DANGER not really sure what this was doing before with 0x21
        {
            continue;
        }

        // Get the sector info from map
        Sector* sector = Map_GetSector(map, request.number);

        float ceilingY = sector->ceilingy;
        float floorY = sector->floory;

        bool drawTesselation = true;
        // Draw the floor and ceiling with tesselation
        // TODO Use gluTesselateion to draw floor and ceiling
        if (drawTesselation)
        {
            bool floor = true;
            do {
                OpenGLRender_DrawFloorOrCeiling(map, sector, floor);
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
            vec2 start = vec2New(w->x, w->z);
            Wall* wend = Map_GetWallEnd(map, w);
            vec2 end =  vec2New(wend->x, wend->z);

            vec2 startZ = vec2Subtract(start, playerPos2);
            vec2 endZ = vec2Subtract(end, playerPos2);

            // The Y is how far ahead of player the point is
            // NOTE negative around player
            startZ = Vec2XZRotateY(startZ, -player->angleRad);
            endZ = Vec2XZRotateY(endZ, -player->angleRad);
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
            // TODO Clip tp view frustum and check if
            // inside it

            // Wall is drawn
            // if it was a portal Add neighbor to queue
            // if there is neighbor AND AND there is room in QUEUE
            OpenGLRender_DrawWall(map, w, floorY, ceilingY, settings);
            if (w->nextsector >= 0)
            {
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
        } // All walls of the sector have been drawn; head has moved forward

        // Mark the sector as drawn
        renderedSectorNames[request.number] += 1;

    } while(head != tail); // Render until buffer is empty: if nothing was added, they are the same

}


void BuildRender_DrawTopDown(Player* player, DukeMap* map, RenderSettingsOpenGL* settings3D, RenderSettings2D* settings2D)
{
    Font* df = DefaultFont_GetDefaultFont();

    vec2 playerPos2;
    if (settings2D->movePlayer)
    {
        playerPos2 = vec2New(player->position.x, player->position.z);
    }
    else
    {
        playerPos2 = settings2D->collisionPoint;
    }

    vec2 collision_forward = vec2New(0.0, WORLD_FORWARD.z * settings2D->collisionLength);

    collision_forward  = Vec2XZRotateY(collision_forward,  Deg2Rad(settings2D->collisionAngleDeg));
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
            // Turn the world around player
            glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);

            if (settings2D->rotateMap)
            {
                glRotatef(Rad2Deg( (player->angleRad )), 0, 0, WORLD_FORWARD.z);
            }

            if (settings2D->centerMapToPlayer)
            {
                // Keep player at center of screen
                glTranslatef(-playerPos2.x, -playerPos2.y, 0);
            }


            // DRAW WALLS
            ////////////////////////////
            glBegin(GL_LINES);
            // Draw origo
            OpenGLRender_SetColor(Color_White);
            OpenGLRender_Line2(0, -10, 0 ,10);
            OpenGLRender_Line2(-10, 0, 10, 0);

            settings2D->collisionInsideSector = -1;
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
                            OpenGLRender_SetColor(Color_White);
                        }
                        else
                        {
                            OpenGLRender_SetColor(Color_Green);
                        }
                    }

                    OpenGLRender_Line2(start.x, start.y, end.x, end.y);
                    vec2 m = Map_GetWallMiddle(map, w);
                    vec2 N = vec2Multiply( Map_GetWallNormal(map, w), 32 );
                    OpenGLRender_Line2(m.x, m.y, m.x + N.x, m.y + N.y);
                }
            }
            glEnd(); // end walls

            // DRAW SPRITES
            // //////////////////////
            glBegin(GL_LINES);

            float spriteSize = 64;
            DefaultColor spriteColor = Color_Red;
            OpenGLRender_SetColor(spriteColor);
            vec2 spriteForward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z * -1);
            for(int spi =  0; spi < map->spriteAmount; spi++)
            {
                DSprite* sprite = &map->sprites[spi];
                vec2 spos2 = vec2New(sprite->position.x, sprite->position.z);
                OpenGLRender_DrawDot(spos2, spriteSize, spriteColor);
                vec2 spriteDir = Vec2XZRotateY(spriteForward, Math_DukeAngleToRad(sprite->ang)-M_PI_2);
                vec2 spriteEnd = vec2Add(spos2, vec2Multiply(spriteDir, settings3D->spriteDefaultWidth/2));
                OpenGLRender_Line2(spos2.x, spos2.y, spriteEnd.x, spriteEnd.y);
            }
            glEnd();

            // DRAW WALL NUMBERS and SECTOR NUMBERS
            glPushMatrix();
                glScalef(1, -1, 1);

            Color4f* wc = Color_GetDefaultColor(Color_White);
            Color4f* sc = Color_GetDefaultColor(Color_Green);
            int numberSize = 128;
            for(int si = 0; si < map->sectorAmount; si++)
            {
                if (settings2D->drawOneSector >=0 && settings2D->drawOneSector != si) { continue; }

                // Get the sector info from map
                Sector* sector = Map_GetSector(map, si);

                int sx = sector->minXZPoint.x + sector->sizeXZ.x/2;
                int sy = -(sector->minXZPoint.y + sector->sizeXZ.y/2);
                Draw2D_RectWH(sx, sy, numberSize, numberSize, Color_GetDefaultColor(Color_Black));

                Font_Printf(df, sc, sx, sy, numberSize, "%d", si);

                // Render all walls of the current sector
                // Discard those that do not face player
                for (s16 wi = 0; wi < sector->wallnum; wi++)
                {
                    if (settings2D->drawOneWall >=0 && settings2D->drawOneWall != wi) { continue; }

                    Wall* w = Map_GetWallInSector(map, si, wi);
                    vec2 start = vec2New(w->x, w->z);

                    int tx = start.x - numberSize/2;
                    int ty = -start.y + numberSize/2;

                    Draw2D_RectWH(tx, ty, numberSize, numberSize, Color_GetDefaultColor(Color_Black));

                    Font_Printf(df, wc, tx, ty, numberSize, "%d", sector->wallptr+wi);
                    if (settings2D->drawOneWall == wi && settings2D->drawOneSector == si)
                    {

                        if (collisionMiss)
                        {
                            Font_Printf(df, wc, start.x, start.y + 64+32, 32, "U: %.2f", collisionOut.y);
                            Font_Printf(df, wc, start.x, start.y + 64, 32, "T: %.2f", collisionOut.x);
                        }
                        else
                        {
                            Font_Printf(df, wc, start.x, start.y + 64+32, 32, "Y: %.2f", collisionOut.y);
                            Font_Printf(df, wc, start.x, start.y + 64, 32, "X: %.2f", collisionOut.x);
                        }
                    }
                }
            }
            glPopMatrix(); // NUMBERS
        glPopMatrix(); // WALLS


        // DRAW PLAYER
        // ////////////

    glPushMatrix();

        glScalef(settings3D->scaleXZ, settings3D->scaleXZ, 1);
        glBegin(GL_LINES);

            vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
            if (settings2D->rotateMap == false)
            {
                forward = Vec2XZRotateY(forward, player->angleRad + M_PI);
            }
            else
            {
                forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z * -1);

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
                if (settings2D->centerMapToPlayer)
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

        glEnd();
        glPopMatrix(); // Player
    glPopMatrix(); // Whole map view
    glLineWidth(1.0f);
}



