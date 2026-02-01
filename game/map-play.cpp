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

static Camera* editorCamera;
static vec3 editorCameraPos;
static bool useEditorCamera = false;

// For settings and debug
static RenderSettings2D render2D;
static RenderSettingsOpenGL renderGL;
static bool showMenu;
static Menu* debugMenu;
static float mapZoom;
static bool ZoomOut;
static bool drawTopdown;
static bool drawOpenGL;

// FOG test
static bool useFog = true;
static GLenum fogMode = GL_LINEAR;
static float fogNear = 84.0f;
static float fogFar = 406.0f;
static GLfloat fogCOlor[4];
static float fogColorValue = 0.0f;
static float fogDensity = 0.9f;

static Color4f clearColor; // set this to fog color

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

    editorCamera = Camera_CreateDefault();
    editorCamera->nearZ = 0.01f;
    editorCamera->farZ = 1000.0f;
    editorCamera->fovY = 77.7f;
    editorCamera->projection = CameraNone;
    Camera_SetMode(editorCamera, CameraDirection);
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

    if (showMenu)
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
        if (useEditorCamera)
        {
            Menu_TextF(debugMenu, "Editor Cam: (%.1f %.1f %.1f) Dirs: (%.1f, %.1f, %.1f)",
                       editorCameraPos.x,
                       editorCameraPos.y,
                       editorCameraPos.z,
                       editorCamera->rotations.x,
                       editorCamera->rotations.y,
                       editorCamera->rotations.z);
        }

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

        if (render2D.movePlayer == false)
        {
            Menu_TextF(debugMenu, "Draw sect: %d", render2D.drawOneSector);
            Menu_Toggle(debugMenu, "Move player", &render2D.movePlayer);
            Menu_Slider(debugMenu, "Collision L", 1.0f, 1024.0f, &render2D.collisionLength);
            Menu_Slider(debugMenu, "Collision A", 0, 360.0f, &render2D.collisionAngleDeg);

            Menu_TextF(debugMenu, "Collision point: (%.1f %.1f) Dir: %.0f", render2D.collisionPoint.x, render2D.collisionPoint.y, render2D.collisionAngleDeg);
            Menu_TextF(debugMenu, "Sector: %s %d", render2D.collisionInsideSector >= 0 ? "Inside" : "Outside", render2D.collisionInsideSector);
            /*
             *    if (Menu_Button(debugMenu, "Draw Wall -"))
             *    {
             *    render2D.drawOneWall--;
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
        */
        }

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

        Menu_Slider(debugMenu, "Far Z ", 100, 1000, &glCamera->farZ);
        Menu_Toggle(debugMenu, "Editor camera", &useEditorCamera);
        //Menu_Text(debugMenu, "Camera");
        //Menu_Slider(debugMenu, "FOV ", 45, 90, &glCamera->fovY);
        //Menu_Slider(debugMenu, "Speed", 1, 2048.0f, &player.moveSpeed);
        //Menu_Slider(debugMenu, "V Speed", 1, 512.0f, &player.verticalSpeed);
        //Menu_Slider(debugMenu, "R Speed", 45, 720.0f, &player.turnSpeedDegrees);
        Menu_Toggle(debugMenu, "FOG", &useFog);
        Menu_Slider(debugMenu, "fogNear", 5.0f, 100.0f, &fogNear);
        Menu_Slider(debugMenu, "fogFar", 200.0f, 1000.0f, &fogFar);
        Menu_Slider(debugMenu, "fogDensity", 0.1f, 1.0f, &fogDensity);
        Menu_Slider(debugMenu, "fogColor", 0.1f, 1.0f, &fogColorValue);
        fogCOlor[0] = fogColorValue;
        fogCOlor[1] = fogColorValue;
        fogCOlor[2] = fogColorValue;
        fogCOlor[3] = 1.0f;
        clearColor.red = fogCOlor[0];
        clearColor.green = fogCOlor[1];
        clearColor.blue = fogCOlor[2];
        clearColor.alpha = 1;
    }

    Menu_DrawCursor(debugMenu);
}

void MapPlay_Init()
{
    InitRenderSettings2D();
    InitRenderSettings3D();

    clearColor.red = 0;
    clearColor.green = 0;
    clearColor.blue = 0;
    clearColor.alpha = 1;

    players = (Player*)mgdl_AllocateGeneralMemory(sizeof(Player) * MAX_PLAYERS);
    MapPlay_ResetPlayers();

    // Change controller mapping : First gamepad to second controller etc...
    Platform_MapJoystickToController(0, 1);
    Platform_MapJoystickToController(1, 2);
    Platform_MapJoystickToController(2, 3);


    // Load Maps
    allMapsArray = (DukeMap**)mgdl_AllocateGeneralMemory(sizeof(DukeMap**) * MAX_MAP_AMOUNT);
    allMapsArray[0] = MapPlay_LoadMap("assets/Maps/samulitesti.map");
    allMapsArray[1] = MapPlay_LoadMap("assets/Maps/GGJ26SmallTest1.map");
    loadedMapAmount = 2;

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

    editorCameraPos= players[0].position;
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

static void MoveEditorCamera(WiiController* controller)
{
    float turnSpeed = players[0].turnSpeedDegrees;
    float moveSpeed = players[0].moveSpeed;
    float moveSpeed3D = moveSpeed;
    float verticalSpeed = players[0].verticalSpeed * 8;
    float verticalSpeed3D = verticalSpeed;
    float dt = mgdl_GetDeltaTime();

    // Use right joystick for turning on Windows
    #if defined (MGDL_PLATFORM_WINDOWS) || defined (MGDL_PLATFORM_LINUX)
    float turn = WiiController_GetRoll(controller);
    if (abs(turn) < CONTROLLER_DEADZONE) turn = 0.0f;

    vec2 js = WiiController_GetNunchukJoystickDirection(controller);

    // Use dpad for turning on Wii
    #else
        float turn = 0.0f;
        if (WiiController_ButtonHeld(controller, ButtonRight)) turn = 1.0f;
        else if (WiiController_ButtonHeld(controller, ButtonLeft)) turn = -1.0f;
    #endif

    // NOTE turning left around Y is positive
    // Forward is 0
    // Left is 90
    // right is -90
    // So when joystick is turned right: it is positive : decrease rotation
    editorCamera->rotations.y -= turn * turnSpeed * dt;
    float pitchInput = WiiController_GetPitch(controller);
    if ( abs(pitchInput) > CONTROLLER_DEADZONE)
    {
        editorCamera->rotations.x += pitchInput;
    }

    vec2 jdir = WiiController_GetNunchukJoystickDirection(controller);
    if (abs(jdir.x) < CONTROLLER_DEADZONE) jdir.x = 0.0f;
    if (abs(jdir.y) < CONTROLLER_DEADZONE) jdir.y = 0.0f;

    vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
    vec2 direction = Vec2XZRotateY(forward, Deg2Rad(editorCamera->rotations.y));

    // NOTE  Joystick dir -Y is forward, +Y is backwards
    // But -Z is forward so...
    vec2 moveXZ = vec2Multiply(direction, jdir.y * moveSpeed3D * dt);

    // Store old

    // Apply move
    editorCameraPos.x += moveXZ.x;
    editorCameraPos.z += moveXZ.y;

    // Jetback controls?
    if (WiiController_ButtonHeld(controller, Button1))
    {
        editorCameraPos.y += verticalSpeed3D * dt;
    }
    if (WiiController_ButtonHeld(controller, Button2))
    {
        editorCameraPos.y -= verticalSpeed3D* dt;
    }
}


MapPlayResult MapPlay_Frame()
{
    mgdl_glClearColor4f(&clearColor);
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
    // NOTE Debugging rendering
    if (useEditorCamera)
    {
        WiiController* cameraC = mgdl_GetController(1);
        MoveEditorCamera(cameraC);
    }
    if (useFog)
    {
        glEnable(GL_FOG);

        glFogi(GL_FOG_MODE, fogMode);
        glFogfv(GL_FOG_COLOR, fogCOlor);
        glFogf(GL_FOG_DENSITY, fogDensity);
        glHint(GL_FOG_HINT, GL_DONT_CARE);
        glFogf(GL_FOG_START, fogNear);
        glFogf(GL_FOG_END, fogFar);
        glEnable(GL_BLEND);
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

                if (useEditorCamera)
                {
                    gluPerspective(editorCamera->fovY,
                                    (float)viewPort.w/(float)viewPort.h,
                                    editorCamera->nearZ,
                                    editorCamera->farZ);
                        // Draw the game for multiple players
                    glViewport(viewPort.x, viewPort.y, viewPort.w, viewPort.h);

                    vec3 cameraPos = Vec3DukePosToOpenGL(editorCameraPos, &renderGL);
                    Camera_SetPosition(editorCamera,
                                    cameraPos.x,
                                    cameraPos.y,
                                    cameraPos.z);

                    //Camera_SetRotations(editorCamera, player->pitchRad, Rad2Deg(-player->angleRad), 0.0f);

                    // Sets GL_MODELVIEW
                    Camera_Apply(editorCamera);
                }
                else
                {

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
                }

                BuildRender_Draw3D(player, activeMap, &renderGL);
            }
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        glPopMatrix();

        if(useFog)
        {
            glDisable(GL_FOG);
        }
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

