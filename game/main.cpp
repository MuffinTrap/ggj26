#include <mgdl.h>
#include "example.h"
#include "dukemapreader.h"
#include "dukemath.h"
#include "build-render.h"
#include "player.h"
#include "gameplay.h"

static Example example;
static DukeMap* map;
static Player player;

// Map menu
static bool showMenu;
static Menu* mapMenu;
static float mapZoom;
static bool ZoomOut;
static bool drawTopdown;
static bool drawOpenGL;

static RenderSettings2D render2D;
static RenderSettingsOpenGL renderGL;
static float dukeUnitsPerMetreXZ;
static float dukeUnitsPerMetreY;
static float texCoordPerMetre;
static Camera* glCamera;

void LoadMap()
{
    map = ReadMapFromFile("assets/Maps/tonnitesti.map");
    Map_PrintInfo(map);
    Map_InitPlayer(map, &player);
    BuildRender_Init(map, &renderGL);
}

void init()
{
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //example.Init();

    player.moveSpeed = 2048.0f; // NOTE Set
    player.verticalSpeed = 1400.0f;
    player.fallingSpeed = 32000.0f;
    player.standingHeight = 10 * 1024.0f; // NOTE Set
    player.kneelingHeight = 4000.0f;
    player.turnSpeedDegrees = 150; // NOTE set
    player.position.y += player.standingHeight;
    player.radius = 128;
    player.shootTimer = 0.0f;
    player.shootRate = 0.33f;
    player.shotThisFrame = false;
    player.shotOrigin = vec3();
    player.shotDirection = vec3();

    Gameplay_Init();

    mapMenu = Menu_CreateWindowed(DefaultFont_GetDefaultFont(), 1.0f, 1.5f, 256,mgdl_GetScreenHeight(), "Map menu");
    drawTopdown = true;
    drawOpenGL = true;
    showMenu = true;

    render2D.mapOffset = vec2New(0,0);
    render2D.mapZoom = 1.0f;
    render2D.scaleXZ = 1.0f;
    render2D.collisionPoint = vec2New(player.position.x, player.position.z);
    render2D.collisionLength = 100.0f;
    render2D.collisionAngleDeg = 180.0f;
    render2D.movePlayer = true;
    render2D.drawOneWall = -1;
    render2D.drawOneSector = -1;
    render2D.rotateMap= true;
    render2D.centerMapToPlayer= true;
    mapZoom = 0.9f;
    ZoomOut = false;



    // 3D settings

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
    Camera_SetMode(glCamera, CameraDirection);

    LoadMap();
}

void frame()
{
    Gameplay_Update(mgdl_GetDeltaTime());

    //example.Update();
    if (render2D.movePlayer)
    {
        Player_UpdateMove(&player, mgdl_GetController(0), &render2D, &renderGL, map);
    }
    else
    {
        vec2 jdir = WiiController_GetNunchukJoystickDirection(mgdl_GetController(0));
        render2D.collisionPoint =  vec2Add(render2D.collisionPoint, vec2Multiply(jdir, 128 * mgdl_GetDeltaTime()));
    }
    // Check if player went outside perimeter wall and put them back


    // Move camera
    vec3 playerposGL = Vec3DukePosToOpenGL(player.position, &renderGL);

    // NOTE this eventuall calls gluLookAt: which wants the eye position
    Camera_SetPosition(glCamera,
                       playerposGL.x,
                       playerposGL.y,
                       playerposGL.z);

    Camera_SetRotations(glCamera, player.pitchRad, Rad2Deg(-player.angleRad), 0.0f);

    Color4f c = Palette_GetColor4f(Palette_GetDefault(), 0);
    mgdl_glClearColor4f(&c);

    // NOTE Use the mgdl_glClear to assure depth buffer working correctly on Wii
    mgdl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if (drawOpenGL)
    {
        glPushMatrix();
            Camera_Apply(glCamera);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE); //  is this needed?

            // This is the other way around on Wii, but
            // hopefully OpenGX handles it
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glShadeModel(GL_FLAT);
                BuildRender_Draw3D(&player, map, &renderGL);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
        glPopMatrix();
    }



    // example.Draw();
    ///////////////////
    if (drawTopdown)
    {
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
        glTranslatef(0.375f, 0.375f, 0.0f);
        BuildRender_DrawTopDown(&player, map, &renderGL, &render2D);
        glPopMatrix();
    }

    mgdl_InitOrthoProjection();

    if (showMenu) { mapMenu->windowHeight = mgdl_GetScreenHeight()-8;} else { mapMenu->windowHeight = 64;}

    Menu_Start(mapMenu, 8, showMenu ?mgdl_GetScreenHeight()-8 : 64, 256);
        Menu_Toggle(mapMenu, showMenu ? "Hide" : "Show", &showMenu);
    if (showMenu)
    {

        if (render2D.movePlayer)
        {
            Menu_TextF(mapMenu, "Player3D: (%.1f %.1f %.1f) Dir: %.0f", player.position.x, player.position.y, player.position.z, Rad2Deg(player.angleRad));
            Menu_TextF(mapMenu, "Player Sector: %s %d", player.sectorNumber >=0 ? "Inside" : "Outside", player.sectorNumber);
        }
        else
        {
            Menu_TextF(mapMenu, "Collision point: (%.1f %.1f) Dir: %.0f", render2D.collisionPoint.x, render2D.collisionPoint.y, render2D.collisionAngleDeg);
            Menu_TextF(mapMenu, "Sector: %s %d", render2D.collisionInsideSector >= 0 ? "Inside" : "Outside", render2D.collisionInsideSector);
        }
    //Menu_Slider(mapMenu, "Speed", 1, 2048.0f, &player.moveSpeed);
    //Menu_Slider(mapMenu, "V Speed", 1, 512.0f, &player.verticalSpeed);
    //Menu_Slider(mapMenu, "R Speed", 45, 720.0f, &player.turnSpeedDegrees);

    Menu_Slider(mapMenu, "Zoom", 0.1f, 6.0f, &mapZoom);
    Menu_Toggle(mapMenu, "Zoom Out", &ZoomOut);
    if (ZoomOut)
    {
        render2D.mapZoom = 1.0f/mapZoom;
    }
    else
    {
        render2D.mapZoom = mapZoom;
    }

    Menu_Toggle(mapMenu, "Rotate on Player", &render2D.rotateMap);
    Menu_Toggle(mapMenu, "Center on Player", &render2D.centerMapToPlayer);

    if (Menu_Button(mapMenu, "Draw Wall -"))
    {
       render2D.drawOneWall--;
    }
    if (Menu_Button(mapMenu, "Draw Wall +"))
    {
       render2D.drawOneWall++;
    }
    Menu_TextF(mapMenu, "Draw wall: %d", render2D.drawOneWall);
    if (Menu_Button(mapMenu, "Draw Sector -"))
    {
       render2D.drawOneSector--;
    }
    if (Menu_Button(mapMenu, "Draw Sector +"))
    {
       render2D.drawOneSector++;
    }
    Menu_TextF(mapMenu, "Draw sect: %d", render2D.drawOneSector);
    Menu_Toggle(mapMenu, "Move player", &render2D.movePlayer);
    Menu_Slider(mapMenu, "Collision L", 1.0f, 1024.0f, &render2D.collisionLength);
    Menu_Slider(mapMenu, "Collision A", 0, 360.0f, &render2D.collisionAngleDeg);

    //Menu_Slider(mapMenu, "Scale XZ", 0.1f, 16.0f, &dukeUnitsPerMetreXZ2D);
    //render2D.scaleXZ =dukeUnitsPerMetreXZ2D;
    //Menu_TextF(mapMenu, "Scale XZ: %.4f", render2D.scaleXZ);
    Menu_Slider(mapMenu, "X", -100.f, 400.0f, &render2D.mapOffset.x);
    Menu_Slider(mapMenu, "Y", -100.f, 400.0f, &render2D.mapOffset.y);
    Menu_Toggle(mapMenu, "Map", &drawTopdown);
    //Menu_Toggle(mapMenu, "Map Y Down", &render2D.mapYDown);

    Menu_Toggle(mapMenu, "OpenGL", &drawOpenGL);

    Menu_Slider(mapMenu, "Player Height", 1024, 16 * 1024, &player.standingHeight);

    //Menu_Slider(mapMenu, "Sprite width", 64, 1024, &renderGL.spriteDefaultWidth);
    //Menu_Slider(mapMenu, "Sprite height", 64, 8024, &renderGL.spriteDefaultHeight);
    //Menu_Slider(mapMenu, "GL Width scaling", 1, 16, &dukeUnitsPerMetreXZ);
    //Menu_Slider(mapMenu, "GL Height scaling", 16, 128, &dukeUnitsPerMetreY);
    // Menu_Slider(mapMenu, "GL Texture scale", 16, 128, &texCoordPerMetre);
    //Menu_TextF(mapMenu, "Scale XZ: %.2f Y: %.2f", renderGL.scaleXZ, renderGL.scaleY);
    renderGL.scaleXZ = 1.0f/dukeUnitsPerMetreXZ;
    renderGL.scaleY = 1.0f/dukeUnitsPerMetreY;
    renderGL.textureScale = 1.0/texCoordPerMetre;

    //Menu_Text(mapMenu, "Camera");
    //Menu_Slider(mapMenu, "FOV ", 45, 90, &glCamera->fovY);
    Menu_Slider(mapMenu, "Far Z ", 100, 1000, &glCamera->farZ);
    if (Menu_Button(mapMenu, "ResetPlayer"))
    {
        Map_InitPlayer(map, &player);
        player.position.y += player.standingHeight;
    }
    if (Menu_Button(mapMenu, "Reload map"))
    {
        LoadMap();
    }
    /*
    for(int i = 0; i< 10; i++)
    {
        Menu_TextF(mapMenu, "Wall %d x: %d -> %d", i, wallXPoints[i*2], wallXPoints[i*2+1]);
    }
    */
    }
    Menu_DrawCursor(mapMenu);

}

void quit()
{
    // Called before program exits
    // Use this to free any resources and disconnect rocket
    //example.Quit();
}

int main()
{
    mgdl_InitSystem("Duke Test",
        ScreenAspect::Screen4x3,
            init,
            frame,
            quit,
        FlagNone
         //| FlagFullScreen
         // | FlagSplashScreen
         // | FlagPauseUntilA
    );

    return 0;
}
