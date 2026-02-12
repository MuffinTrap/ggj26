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
static Sound* music;
bool musicStarted = false;

// Debug Menu
void init()
{
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    OpenGLRender_Init();
    MainMenu_Init();
    MapPlay_Init();
    Gameplay_Init();

    // Angle test
    /*
    vec3 angles[4];
    angles[0] = vec3New(0, 0, 1);
    angles[1] = vec3New(1, 0, 0);
    angles[2] = vec3New(0, 0, -1);
    angles[3] = vec3New(-1, 0, 0);
    vec3 rotatedForward;
    Log_InfoF("Matrix3x3 rotations\n");
    for (int angle = 0; angle < 2048; angle += 512)
    {
        mat3x3 rotateMatrix;
        mat3x3Identity(rotateMatrix);
        mat3x3RotateY(rotateMatrix, Math_DukeAngleToRad(angle));
        rotatedForward = mat3x3MultiplyVector(rotateMatrix, WORLD_FORWARD);
        vec3 expect= angles[angle/512];
        Log_InfoF("Rotate: Duke angle %d (%.2f degrees): is (%.2f, %.2f, %.2f): should be (%.1f, %.1f, %.1f)\n",
                  angle, Rad2Deg(Math_DukeAngleToRad(angle)),
                  rotatedForward.x, rotatedForward.y, rotatedForward.z,
                  expect.x, expect.y, expect.z);
    }

    Log_InfoF("Vec2XZRotateY rotations\n");
    vec2 f2 = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
    for (int angle = 0; angle < 2048; angle += 512)
    {
        vec2 rotated2 = Vec2XZRotateY(f2, Math_DukeAngleToRad(angle));
        vec3 expect= angles[angle/512];
        Log_InfoF("Rotate: Duke angle %d (%.2f degrees): is (%.2f, %.2f, %.2f): should be (%.1f, %.1f, %.1f)\n",
                  angle, Rad2Deg(Math_DukeAngleToRad(angle)),
                  rotated2.x, 0.0f, rotated2.y,
                  expect.x, expect.y, expect.z);
    }
    */

    music = mgdl_LoadSoundMp3("assets/music.mp3");
    currentState = Game_MainMenu;

    Color4f c = Palette_GetColor4f(Palette_GetDefault(), 0);
    mgdl_glClearColor4f(&c);


}


void frame()
{
    if (musicStarted == false)
    {
        Audio_PlaySound(music);
        musicStarted = true;
    }
    // NOTE Use the mgdl_glClear to assure depth buffer working correctly on Wii
    mgdl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    switch(currentState)
    {
        case Game_MainMenu:
        {
            MainMenuResult r = MainMenu_Frame();
            if (r == MainMenuStartGame)
            {
                Audio_StopSound(music);
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
         //| FlagFullScreen
         | FlagSplashScreen
         // | FlagPauseUntilA
    );

    return 0;
}
