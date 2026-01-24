#include <mgdl.h>
#include "example.h"
#include "dukemapreader.h"
#include "build-render.h"
#include "player.h"

static Example example;
static DukeMap* map;
static Player player;

// Map menu
static Menu* mapMenu;
static float mapZoom;
static bool ZoomOut;
static bool drawTopdown;
static bool drawOpenGL;

static RenderSettings2D render2D;
static RenderSettingsOpenGL renderGL;
static float dukeUnitsPerMetreXZ;
static float dukeUnitsPerMetreY;
static Camera* glCamera;

void init()
{
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //example.Init();
    map = ReadMapFromFile("assets/Maps/tonnitesti.map");
    Map_PrintInfo(map);
    Map_InitPlayer(map, &player);
    BuildRender_Init(map);

    player.moveSpeed = 2048.0f; // NOTE Set
    player.verticalSpeed = 400.0f;
    player.standingHeight = 2048.0f; // NOTE Set
    player.kneelingHeight = 4000.0f;
    player.turnSpeedDegrees = 150; // NOTE set
    player.positionOpenGL.z += player.standingHeight;

    mapMenu = Menu_CreateWindowed(DefaultFont_GetDefaultFont(), 1.0f, 1.5f, 256, 128, "Map menu");
    drawTopdown = true;
    drawOpenGL = true;

    render2D.mapOffset = vec2New(0,0);
    render2D.mapZoom = 1.0f;
    render2D.scaleXZ = 1.0f;
    mapZoom = 6.0f;
    ZoomOut = true;
    render2D.mapYDown = true;


    // 3D settings

    dukeUnitsPerMetreXZ = 7.3f;
    dukeUnitsPerMetreY = 109.6;
    renderGL.scaleXZ = 1.0f/dukeUnitsPerMetreXZ;
    renderGL.scaleY = 1.0f/dukeUnitsPerMetreY;

    glCamera = Camera_CreateDefault();
    glCamera->nearZ = 0.0001f;
    glCamera->fovY = 77.7f;
    Camera_SetMode(glCamera, CameraDirection);
}

void frame()
{
    //example.Update();
    Player_UpdateMove(&player, mgdl_GetController(0), &render2D, &renderGL);

    // Move camera
    //NOTE Z is up
    Camera_SetPosition(glCamera,
                       player.positionOpenGL.x * renderGL.scaleXZ,
                       player.positionOpenGL.z * renderGL.scaleY,
                       player.positionOpenGL.y * renderGL.scaleXZ);
    Camera_SetRotations(glCamera, player.Pitch * M_PI, Rad2Deg(-player.angleRad), 0.0f);

    Color4f c = Palette_GetColor4f(Palette_GetDefault(), 1);
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
            //glEnable(GL_CULL_FACE);
            //glCullFace(GL_BACK);
            glShadeModel(GL_FLAT);
                BuildRender_DrawOpenGL(&player, map, &renderGL);
            glDisable(GL_DEPTH_TEST);
            //glDisable(GL_CULL_FACE);
        glPopMatrix();
    }



    // example.Draw();
    ///////////////////
    if (drawTopdown)
    {
        glPushMatrix();
        if (render2D.mapYDown)
        {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            /////////// 2D drawing mode
            // Y increases down :
            gluOrtho2D(0.0, (double)mgdl_GetScreenWidth(), (double)mgdl_GetScreenHeight(), 0.0);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // NOTE: This is from the OpenGL red book. The purpose is to have the vertices
            // in the middle of the screen pixels
            }
        else
        {
            mgdl_InitOrthoProjection();
        }
        glTranslatef(0.375f, 0.375f, 0.0f);
        BuildRender_DrawTopDown(&player, map, &renderGL, &render2D);
        glPopMatrix();
    }

    mgdl_InitOrthoProjection();

    Menu_Start(mapMenu, 8, mgdl_GetScreenHeight()-8, 256);
    Menu_TextF(mapMenu, "Player3D: (%.1f %.1f %.1f) Dir: %.0f", player.positionOpenGL.x, player.positionOpenGL.y, player.positionOpenGL.z, Rad2Deg(player.angleRad));
    Menu_Slider(mapMenu, "Speed", 1, 2048.0f, &player.moveSpeed);
    Menu_Slider(mapMenu, "V Speed", 1, 512.0f, &player.verticalSpeed);
    Menu_Slider(mapMenu, "R Speed", 45, 720.0f, &player.turnSpeedDegrees);

    Menu_Slider(mapMenu, "Zoom", 1.0f, 32.0f, &mapZoom);
    Menu_Toggle(mapMenu, "Zoom Out", &ZoomOut);
    if (ZoomOut)
    {
        render2D.mapZoom = 1.0f/mapZoom;
    }
    else
    {
        render2D.mapZoom = mapZoom;
    }

    //Menu_Slider(mapMenu, "Scale XZ", 0.1f, 16.0f, &dukeUnitsPerMetreXZ2D);
    //render2D.scaleXZ =dukeUnitsPerMetreXZ2D;
    //Menu_TextF(mapMenu, "Scale XZ: %.4f", render2D.scaleXZ);
    Menu_Slider(mapMenu, "X", -100.f, 100.0f, &render2D.mapOffset.x);
    Menu_Slider(mapMenu, "Y", -100.f, 100.0f, &render2D.mapOffset.y);
    Menu_Toggle(mapMenu, "Map", &drawTopdown);
    Menu_Toggle(mapMenu, "Map Y Down", &render2D.mapYDown);

    Menu_Toggle(mapMenu, "OpenGL", &drawOpenGL);

    Menu_Slider(mapMenu, "GL Width scaling", 1, 16, &dukeUnitsPerMetreXZ);
    Menu_Slider(mapMenu, "GL Height scaling", 16, 128, &dukeUnitsPerMetreY);
    Menu_TextF(mapMenu, "Scale XZ: %.2f Y: %.2f", renderGL.scaleXZ, renderGL.scaleY);
    renderGL.scaleXZ = 1.0f/dukeUnitsPerMetreXZ;
    renderGL.scaleY = 1.0f/dukeUnitsPerMetreY;

    Menu_Text(mapMenu, "Camera");
    Menu_Slider(mapMenu, "FOV ", 45, 90, &glCamera->fovY);
    Menu_Slider(mapMenu, "Far Z ", 100, 1000, &glCamera->farZ);
    if (Menu_Button(mapMenu, "ResetPlayer"))
    {
        player.positionOpenGL = map->startPosition;
        player.positionOpenGL.z += player.standingHeight;
    }
    /*
    for(int i = 0; i< 10; i++)
    {
        Menu_TextF(mapMenu, "Wall %d x: %d -> %d", i, wallXPoints[i*2], wallXPoints[i*2+1]);
    }
    */
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
