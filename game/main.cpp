#include <mgdl.h>
#include "example.h"
#include "dukemapreader.h"
#include "build-render.h"

static Example example;
static DukeMap* map;
static Player player;

void init()
{
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //example.Init();
    map = ReadMapFromFile("assets/Maps/test_room.map");
    Map_PrintInfo(map);
    player.position = map->startPosition;
    BuildRender_Init(map);
}

void frame()
{
    //example.Update();

    // NOTE Use the mgdl_glClear to assure depth buffer working correctly on Wii
    mgdl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    // example.Draw();
    mgdl_InitOrthoProjection();
    BuildRender_DrawFirstPerson(&player, map);
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
         | FlagSplashScreen
         // | FlagPauseUntilA
    );

    return 0;
}
