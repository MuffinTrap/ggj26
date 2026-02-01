#include "map-play.h"
#include "dukemap.h"
#include "player.h"
#include "dukemapreader.h"
#include "main.h"
#include "build-render.h"
#include "dukemath.h"
#include "mgdl/mgdl-alloc.h"

#define MAX_MAP_AMOUNT 4
static int loadedMapAmount =0;
static DukeMap** allMapsArray;
static DukeMap* activeMap;
static Player* players;
static int activePlayerAmount = 1;

static Camera* glCamera;

// For settings and debug
static RenderSettings2D render2D;
static RenderSettingsOpenGL renderGL;
static bool showMenu;
static Menu* debugMenu;
static float mapZoom;
static bool ZoomOut;
static bool drawTopdown;
static bool drawOpenGL;

static float playerAmountSlider = 1;

// For drawing the map in 3D
static float dukeUnitsPerMetreXZ;
static float dukeUnitsPerMetreY;
static float texCoordPerMetre;

Texture* maskTexture;
Texture* crosshairTexture;

static void InitRenderSettings3D()
{
    dukeUnitsPerMetreXZ = 7.3f;// NOTE CHECKED
    dukeUnitsPerMetreY = 109.6;// NOTE CHECKED
    texCoordPerMetre = 100.0f; // NOTE CHECKED
    renderGL.scaleXZ = 1.0f/dukeUnitsPerMetreXZ;
    renderGL.scaleY = 1.0f/dukeUnitsPerMetreY;
    renderGL.textureScale = 1.0/texCoordPerMetre;
    renderGL.spriteDefaultWidth = 1024;
    renderGL.spriteDefaultHeight = 8024;

    glCamera = Camera_CreateDefault();
    glCamera->nearZ = 0.0001f;
    glCamera->fovY = 77.7f;
    glCamera->projection = CameraNone;
    Camera_SetMode(glCamera, CameraDirection);
}

static void InitRenderSettings2D()
{
    debugMenu = Menu_CreateWindowed(DefaultFont_GetDefaultFont(), 1.0f, 1.5f, 256,mgdl_GetScreenHeight(), "Map menu");
    drawTopdown = true;
    drawOpenGL = true;
    showMenu = true;

    render2D.mapOffset = vec2New(0,0);
    render2D.mapZoom = 1.0f;
    render2D.scaleXZ = 1.0f;
    render2D.collisionPoint = vec2New(0, 0);
    render2D.collisionLength = 100.0f;
    render2D.collisionAngleDeg = 180.0f;
    render2D.movePlayer = true;
    render2D.drawOneWall = -1;
    render2D.drawOneSector = -1;
    render2D.rotateMap= true;
    render2D.centerMapToPlayer= true;
    mapZoom = 0.9f;
    ZoomOut = false;
}

static void DrawMinimap()
{
    // Reset viewPort
    Rect viewPort = MapPlay_GetPlayerScreenRect(0, 1);
    glViewport(viewPort.x, viewPort.y, viewPort.w, viewPort.h);


    // example.Draw();
    ///////////////////
    glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        /////////// 2D drawing mode
        // Y increases down :
        gluOrtho2D(0.0, (double)mgdl_GetScreenWidth(), (double)mgdl_GetScreenHeight(), 0.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // NOTE: This is from the OpenGL red book. The purpose is to have the vertices
        // in the middle of the screen pixels
        if (drawTopdown)
        {
            glTranslatef(0.375f, 0.375f, 0.0f);
            BuildRender_DrawTopDown(&players[0], activeMap, &renderGL, &render2D);
        }
    glPopMatrix();
}

static void DrawDebugMenu()
{
    mgdl_InitOrthoProjection();
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    if (showMenu) { debugMenu->windowHeight = mgdl_GetScreenHeight()-8;} else { debugMenu->windowHeight = 64;}

    Menu_Start(debugMenu, 8, showMenu ?mgdl_GetScreenHeight()-8 : 64, 256);
        Menu_Toggle(debugMenu, showMenu ? "Hide" : "Show", &showMenu);
    /*if (showMenu)
    {

        if (render2D.movePlayer)
        {
            Menu_TextF(debugMenu, "Player3D: (%.1f %.1f %.1f) Dir: %.0f",
                       players[0].position.x,
                       players[0].position.y,
                       players[0].position.z,
                       Rad2Deg(players[0].angleRad));

            Menu_TextF(debugMenu, "Player Sector: %s %d",
                       players[0].sectorNumber >=0 ? "Inside" : "Outside", players[0].sectorNumber);
        }
        else
        {
            Menu_TextF(debugMenu, "Collision point: (%.1f %.1f) Dir: %.0f", render2D.collisionPoint.x, render2D.collisionPoint.y, render2D.collisionAngleDeg);
            Menu_TextF(debugMenu, "Sector: %s %d", render2D.collisionInsideSector >= 0 ? "Inside" : "Outside", render2D.collisionInsideSector);
        }
    //Menu_Slider(debugMenu, "Speed", 1, 2048.0f, &player.moveSpeed);
    //Menu_Slider(debugMenu, "V Speed", 1, 512.0f, &player.verticalSpeed);
    //Menu_Slider(debugMenu, "R Speed", 45, 720.0f, &player.turnSpeedDegrees);

    Menu_Slider(debugMenu, "Zoom", 0.1f, 6.0f, &mapZoom);
    Menu_Toggle(debugMenu, "Zoom Out", &ZoomOut);
    if (ZoomOut)
    {
        render2D.mapZoom = 1.0f/mapZoom;
    }
    else
    {
        render2D.mapZoom = mapZoom;
    }

    Menu_Toggle(debugMenu, "Rotate on Player", &render2D.rotateMap);
    Menu_Toggle(debugMenu, "Center on Player", &render2D.centerMapToPlayer);

    if (Menu_Button(debugMenu, "Draw Wall -"))
    {
       render2D.drawOneWall--;
    }
    if (Menu_Button(debugMenu, "Draw Wall +"))
    {
       render2D.drawOneWall++;
    }
    Menu_TextF(debugMenu, "Draw wall: %d", render2D.drawOneWall);
    if (Menu_Button(debugMenu, "Draw Sector -"))
    {
       render2D.drawOneSector--;
    }
    if (Menu_Button(debugMenu, "Draw Sector +"))
    {
       render2D.drawOneSector++;
    }
    Menu_TextF(debugMenu, "Draw sect: %d", render2D.drawOneSector);
    Menu_Toggle(debugMenu, "Move player", &render2D.movePlayer);
    Menu_Slider(debugMenu, "Collision L", 1.0f, 1024.0f, &render2D.collisionLength);
    Menu_Slider(debugMenu, "Collision A", 0, 360.0f, &render2D.collisionAngleDeg);

    //Menu_Slider(debugMenu, "Scale XZ", 0.1f, 16.0f, &dukeUnitsPerMetreXZ2D);
    //render2D.scaleXZ =dukeUnitsPerMetreXZ2D;
    //Menu_TextF(debugMenu, "Scale XZ: %.4f", render2D.scaleXZ);
    Menu_Slider(debugMenu, "X", -100.f, 400.0f, &render2D.mapOffset.x);
    Menu_Slider(debugMenu, "Y", -100.f, 400.0f, &render2D.mapOffset.y);
    Menu_Toggle(debugMenu, "Map", &drawTopdown);
    //Menu_Toggle(debugMenu, "Map Y Down", &render2D.mapYDown);

    Menu_Toggle(debugMenu, "OpenGL", &drawOpenGL);

    //Menu_Slider(debugMenu, "Player Height", 1024, 16 * 1024, &players[0].standingHeight);

    //Menu_Slider(debugMenu, "Sprite width", 64, 1024, &renderGL.spriteDefaultWidth);
    //Menu_Slider(debugMenu, "Sprite height", 64, 8024, &renderGL.spriteDefaultHeight);
    //Menu_Slider(debugMenu, "GL Width scaling", 1, 16, &dukeUnitsPerMetreXZ);
    //Menu_Slider(debugMenu, "GL Height scaling", 16, 128, &dukeUnitsPerMetreY);
    // Menu_Slider(debugMenu, "GL Texture scale", 16, 128, &texCoordPerMetre);
    //Menu_TextF(debugMenu, "Scale XZ: %.2f Y: %.2f", renderGL.scaleXZ, renderGL.scaleY);
    renderGL.scaleXZ = 1.0f/dukeUnitsPerMetreXZ;
    renderGL.scaleY = 1.0f/dukeUnitsPerMetreY;
    renderGL.textureScale = 1.0/texCoordPerMetre;

    //Menu_Text(debugMenu, "Camera");
    //Menu_Slider(debugMenu, "FOV ", 45, 90, &glCamera->fovY);
    Menu_Slider(debugMenu, "Far Z ", 100, 1000, &glCamera->farZ);
    if (Menu_Button(debugMenu, "ResetPlayer"))
    {
        Map_InitPlayer(activeMap, &players[0]);
        players[0].position.y += players[0].standingHeight;
    }
    if (Menu_Button(debugMenu, "Reload map"))
    {
        //MapPlay_ReloadActiveMap();
    }

    Menu_Slider(debugMenu,"Player count", 0.1f, 4.0f, &playerAmountSlider);
    activePlayerAmount = ceilf(playerAmountSlider);
    if (activePlayerAmount < 1)
    {
        activePlayerAmount = 1;
    }
    // Show controller status
    if (Menu_Button(debugMenu, "Init controllers"))
    {
        Platform_InitControllers();
        Platform_MapJoystickToController(0, 1);
        Platform_MapJoystickToController(1, 2);
        Platform_MapJoystickToController(2, 3);
    }
    for (int i = 0; i <MGDL_MAX_CONTROLLERS; i++)
    {
        bool isConnected = Platform_IsControllerConnected(i);
        if (isConnected)
        {
            Menu_TextF(debugMenu, "Controller %d", i);
        }
    }
    */
    /*
    for(int i = 0; i< 10; i++)
    {
        Menu_TextF(debugMenu, "Wall %d x: %d -> %d", i, wallXPoints[i*2], wallXPoints[i*2+1]);
    }
    */
    //}
    Menu_DrawCursor(debugMenu);
}

void MapPlay_Init()
{
    InitRenderSettings2D();
    InitRenderSettings3D();

    players = (Player*)mgdl_AllocateGeneralMemory(sizeof(Player) * MAX_PLAYERS);
    MapPlay_ResetPlayers();

    // Change controller mapping : First gamepad to second controller etc...
    Platform_MapJoystickToController(0, 1);
    Platform_MapJoystickToController(1, 2);
    Platform_MapJoystickToController(2, 3);

    // Load Maps
    allMapsArray = (DukeMap**)mgdl_AllocateGeneralMemory(sizeof(DukeMap**) * MAX_MAP_AMOUNT);
    allMapsArray[0] = MapPlay_LoadMap("assets/Maps/samulitesti.map");
    allMapsArray[1] = MapPlay_LoadMap("assets/Maps/GGJ26Test1.map");
    allMapsArray[2] = MapPlay_LoadMap("assets/Maps/GGJ26Test2.map");
    allMapsArray[3] = MapPlay_LoadMap("assets/Maps/tonnitesti.map");
    loadedMapAmount = 4;

    // Load ui textures
    maskTexture = mgdl_LoadTexture("assets/screen_mask_texture.png", Linear);
    crosshairTexture = mgdl_LoadTexture("assets/crosshair.png", Linear);
}

void MapPlay_ResetPlayers()
{
    for (int pi = 0; pi < MAX_PLAYERS; pi++)
    {
        players[pi].playerNumber = pi;
        players[pi].moveSpeed = 2048.0f; // NOTE Set
        players[pi].verticalSpeed = 1400.0f;
        players[pi].fallingSpeed = 32000.0f;
        players[pi].standingHeight = 6 * 1024.0f; // NOTE Set
        players[pi].kneelingHeight = 4000.0f;
        players[pi].turnSpeedDegrees = 150.0f; // NOTE set
        players[pi].position.y += players[pi].standingHeight;
        players[pi].radius = 340.0f;
        players[pi].pitchRad = 0.0f;
        players[pi].shootTimer = 0.0f;
        players[pi].shootRate = 0.33f;
        players[pi].shotThisFrame = false;
        players[pi].shotOrigin = vec3();
        players[pi].shotDirection = vec3();
        players[pi].hasTreasure = false;
        players[pi].stunTimer = 0.0f;
    }
}

DukeMap* MapPlay_LoadMap(const char* mapfile)
{
    DukeMap* map = ReadMapFromFile(mapfile);
    Map_PrintInfo(map);
    return map;
}

void MapPlay_StartMap(int mapIndex, int playerAmount)
{
    activePlayerAmount = playerAmount;
    if (mapIndex >= 0 && mapIndex < loadedMapAmount)
    {
        activeMap = allMapsArray[mapIndex];
    }
    else
    {
        return;
    }
    BuildRender_Init(activeMap, &renderGL);
    MapPlay_ResetPlayers();
    Map_InitPlayers(activeMap, players, activePlayerAmount);
    Gameplay_StartMap(activeMap);
}

Rect MapPlay_GetPlayerScreenRect(int playerIndex, int amountPlayers)
{
    int W = mgdl_GetScreenWidth();
    int H = mgdl_GetScreenHeight();
    Rect r = Rect_Create(0, 0, W, H);

    if (amountPlayers == 1)
    {
        // Full screen
        // [ 1 ]
        // [   ]
        // NOP
    }
    if (amountPlayers == 2)
    {
        // Half screen vertically, multiply y with index
        // [ 1 ]
        // [ 2 ]
        // XY is bottom left
        r = Rect_Create(0,  H/2 - (H/2)*playerIndex, W, H/2);

    }
    else if (amountPlayers >= 3)
    {
        // Quarter of screen;
        // [1] [2]
        // [3] [4]
        // X = index % 2
        // Y = index/2 % 2
        int x = playerIndex % 2;  // 0-> 0 1-> 1  2-0 3 ->1
        int y = (playerIndex/2) % 2; // 0 -> 0, 1->0  2-> 1 3-> 1
        r = Rect_Create(W/2 * x, H/2 - (H/2)*y, W/2, H/2);
    }
    return r;
}

static void DebugCollisions()
{

    vec2 jdir = WiiController_GetNunchukJoystickDirection(mgdl_GetController(0));
    render2D.collisionPoint =  vec2Add(render2D.collisionPoint, vec2Multiply(jdir, 128 * mgdl_GetDeltaTime()));
}

MapPlayResult MapPlay_Frame()
{
    // Update
    if (render2D.movePlayer)
    {
        for (int pi = 0; pi < activePlayerAmount; pi++)
        {
            Gameplay_Update(&players[pi], activeMap);
            Gameplay_UpdateBullets(players, activePlayerAmount, activeMap);
            Player_UpdateMove(&players[pi], mgdl_GetController(pi), &render2D, &renderGL, activeMap, activePlayerAmount);
        }

        if (Gameplay_GetWinner() >= 0)
        {
            return MapPlayEndMap;
        }
    }

    // Draw
        glPushMatrix();
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE); //  is this needed?

            // This is the other way around on Wii, but
            // hopefully OpenGX handles it
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glShadeModel(GL_FLAT);

            for (int pi = 0; pi < activePlayerAmount; pi++)
            {
                Player* player = &players[pi];
                // Move camera
                vec3 playerposGL = Vec3DukePosToOpenGL(player->position, &renderGL);

                // NOTE this eventuall calls gluLookAt: which wants the eye position

                Rect viewPort = MapPlay_GetPlayerScreenRect(pi, activePlayerAmount);


                // NOTE Must set GL_PROJECTION first then GL_MODELVIEW
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                gluPerspective(glCamera->fovY,
                                (float)viewPort.w/(float)viewPort.h,
                                glCamera->nearZ,
                                glCamera->farZ);
                    // Draw the game for multiple players
                glViewport(viewPort.x, viewPort.y, viewPort.w, viewPort.h);

                Camera_SetPosition(glCamera,
                                playerposGL.x,
                                playerposGL.y,
                                playerposGL.z);

                Camera_SetRotations(glCamera, player->pitchRad, Rad2Deg(-player->angleRad), 0.0f);

                // Sets GL_MODELVIEW
                Camera_Apply(glCamera);

                BuildRender_Draw3D(player, activeMap, &renderGL);
            }
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        glPopMatrix();

        for (int pi = 0; pi < activePlayerAmount; pi++)
        {
            Rect viewPort = MapPlay_GetPlayerScreenRect(pi, activePlayerAmount);
            glViewport(viewPort.x, viewPort.y, viewPort.w, viewPort.h);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            // Y increases up : OpenGL default
            gluOrtho2D(0.0, (double)viewPort.w, 0.0, (double)viewPort.h);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // NOTE: This is from the OpenGL red book. The purpose is to have the vertices
            // in the middle of the screen pixels
            glTranslatef(0.375f, 0.375f, 0.0f);

            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.3f);

            // Draw crosshair
#ifdef MGDL_PLATFORM_WII
            vec2 cursorPosition = WiiController_GetCursorPosition(mgdl_GetController(pi));
            vec2 relativeScreenPosition = vec2New((cursorPosition.x - viewPort.x), (cursorPosition.y - viewPort.y));
            if (IsPointInsideRect(viewPort, cursorPosition))
            {
                Texture_Draw2DAbsolute(crosshairTexture, relativeScreenPosition.x - 32, relativeScreenPosition.y - 32, relativeScreenPosition.x + 32, relativeScreenPosition.y + 32);
            }
#endif
            // Draw mask
            if (players[pi].hasTreasure)
            {
                Texture_Draw2DAbsolute(maskTexture, 0, 0, viewPort.w, viewPort.h);
            }
        }
        glDisable(GL_ALPHA_TEST);
        

        DrawDebugMenu();

    return MapPlayLoop;
}

