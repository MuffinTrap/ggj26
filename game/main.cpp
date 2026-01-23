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
static bool draw2D;
static bool drawTopdown;
static bool drawOpenGL;


static RenderSettings2D render2D;
static RenderSettingsOpenGL renderGL;
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

    player.moveSpeed = 1024;
    player.verticalSpeed = 586;
    player.standingHeight = 8148; // Duke Units
    player.kneelingHeight = 2048; // Duke Units
    player.positionOpenGL.z = 16.5;

    mapMenu = Menu_CreateWindowed(DefaultFont_GetDefaultFont(), 1.0f, 1.1f, 256, 128, "Map menu");
    draw2D = true;
    drawTopdown = true;
    drawOpenGL = false;

    render2D.floor0Level = 0;
    render2D.mapOffset = vec2New(0,0);
    render2D.mapZoom = 1.0f;
    render2D.heightPerDukeK = 12;
    render2D.widthPerDukeK = 400;

    renderGL.floor0Level = -30.0f;
    renderGL.heightPerDukeK = 12.0f;
    renderGL.widthPerDukeK = 92.0f;

    glCamera = Camera_CreateDefault();
    Camera_SetMode(glCamera, CameraDirection);
}

void frame()
{
    //example.Update();
    Player_UpdateMove(&player, mgdl_GetController(0), &render2D, &renderGL);

    // Move camera
    //NOTE Z is up
    Camera_SetPosition(glCamera, player.positionOpenGL.x, player.positionOpenGL.z, player.positionOpenGL.y);
    Camera_SetRotations(glCamera, player.Pitch * M_PI, Rad2Deg(-player.angleRad), 0.0f);

    Color4f c = Palette_GetColor4f(Palette_GetDefault(), 1);
    mgdl_glClearColor4f(&c);

    // NOTE Use the mgdl_glClear to assure depth buffer working correctly on Wii
    mgdl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if (drawOpenGL)
    {
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
    }


    // example.Draw();
    mgdl_InitOrthoProjection();
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
    ///////////////////
    if (draw2D)
    {
        BuildRender_DrawFirstPerson(&player, map, &render2D);
    }
    if (drawTopdown)
    {
        BuildRender_DrawTopDown(&player, map, &render2D);
    }

    mgdl_InitOrthoProjection();

    Menu_Start(mapMenu, 8, mgdl_GetScreenHeight()-8, 256);
    Menu_TextF(mapMenu, "Player: (%.1f %.1f %.1f) Dir: %.0f", player.position2D.x, player.position2D.y, player.position2D.z, Rad2Deg(player.angleRad));
    Menu_Slider(mapMenu, "Speed", 128, 4096.0f, &player.moveSpeed);
    Menu_Slider(mapMenu, "V Speed", 16, 1024.0f, &player.verticalSpeed);
    Menu_Slider(mapMenu, "Zoom", 0.01f, 1.0f, &render2D.mapZoom);
    Menu_Slider(mapMenu, "X", -1000.f, 1000.0f, &render2D.mapOffset.x);
    Menu_Slider(mapMenu, "Y", -1000.f, 1000.0f, &render2D.mapOffset.y);
    Menu_Toggle(mapMenu, "2D", &draw2D);
    Menu_Toggle(mapMenu, "Map", &drawTopdown);
    Menu_Slider(mapMenu, "2D Floor Level", -32, 32.0f, &render2D.floor0Level);
    Menu_Slider(mapMenu, "2D Height scaling", 8, 128, &render2D.heightPerDukeK);
    Menu_Slider(mapMenu, "2D Width scaling", 64, 1024, &render2D.widthPerDukeK);
    Menu_Toggle(mapMenu, "OpenGL", &drawOpenGL);
    Menu_TextF(mapMenu, "Player3D: (%.1f %.1f %.1f) Dir: %.0f", player.positionOpenGL.x, player.positionOpenGL.y, player.positionOpenGL.z, Rad2Deg(player.angleRad));
    Menu_Slider(mapMenu, "GL Floor Level", -32, 32.0f, &renderGL.floor0Level);
    Menu_Slider(mapMenu, "GL Height scaling", 1, 64, &renderGL.heightPerDukeK);
    Menu_Slider(mapMenu, "GL Width scaling", 8, 128, &renderGL.widthPerDukeK);
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
