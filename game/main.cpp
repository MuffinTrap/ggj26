#include <mgdl.h>
#include "example.h"
#include "dukemapreader.h"
#include "dukemath.h"
#include "build-render.h"
#include "player.h"
#include "gameplay.h"
#include "main.h"
#include "mainmenu.h"
#include "map-play.h"

static GameState currentState;

// Debug Menu
void init()
{
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    MainMenu_Init();
    MapPlay_Init();
    Gameplay_Init();


    currentState = Game_MainMenu;

    Color4f c = Palette_GetColor4f(Palette_GetDefault(), 0);
    mgdl_glClearColor4f(&c);
}


void frame()
{
    // NOTE Use the mgdl_glClear to assure depth buffer working correctly on Wii
    mgdl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    switch(currentState)
    {
        case Game_MainMenu:
        {
            MainMenuResult r = MainMenu_Frame();
            if (r == MainMenuStartGame)
            {
                currentState = Game_MapPlay;
                MapPlay_StartMap(MainMenu_GetSelectedMapIndex(), MainMenu_GetSelectedPlayerAmount());
            }
        }
            break;
        case Game_MapPlay:
        {
            MapPlayResult r = MapPlay_Frame();
            // TODO end
            if (r == MapPlayEndMap)
            {
                currentState = Game_MainMenu;
            }
        }
            break;

        case Game_MapLobby:
            // NOP
            break;
        case Game_MapOver:
           // TODO
            break;
    }
}

void quit()
{
    // Called before program exits
    // Use this to free any resources and disconnect rocket
    //example.Quit();
}

int main()
{
    mgdl_InitSystem("O ALASA E LEN WAWA",
        ScreenAspect::Screen4x3,
            init,
            frame,
            quit,
        FlagNone
        // | FlagGameHandlesHOME
         | FlagFullScreen
         // | FlagSplashScreen
         // | FlagPauseUntilA
    );

    return 0;
}
