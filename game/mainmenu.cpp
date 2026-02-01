#include "mainmenu.h"
#include <mgdl.h>
#include <mgdl/mgdl-main.h>

    static int playerAmount = 1;
    static int mapIndex = 0;
    static Menu* mainMenu;
    static Menu* mapMenu;
    static Font* menuFont;

    static Texture* betaText;

void MainMenu_Init()
{


    betaText = mgdl_LoadTexture("assets/betamaze_36x36.png", Nearest);

    menuFont =  Font_LoadSelective(betaText, 36, 36, 7, " \"(),.0123456789:;ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    mainMenu = Menu_Create(menuFont, 2, 1.0f);
    //mapMenu = Menu_Create(menuFont, 2, 1.2f);
    // TODO set colors etc
    // color ffbe03
    Color4f orange = Color_HexToFloats(0xffbe03ff);
    Color4f black = Color_HexToFloats(0x020202ff);
    Color4f white = Color_HexToFloats(0xfffcfcfc);
    mainMenu->bg = orange;
    mainMenu->text = black;
    mainMenu->highlight = white;

}

MainMenuResult MainMenu_Frame()
{
    Color4f orange = Color_HexToFloats(0xffbe03ff);
    Color4f black = Color_HexToFloats(0x020202ff);
    Color4f white = Color_HexToFloats(0xfffcfcfc);
    Color4f grey = Color_HexToFloats(0xff4c4c4c);
    mgdl_glClearColor4f(&orange);
    mgdl_InitOrthoProjection();
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());
    Menu_Start(mainMenu, 36, mgdl_GetScreenHeight()-36, 36 * 2 * 5);

    mainMenu->text = grey;
    Menu_TextF(mainMenu, "JAN_%d", playerAmount);

    mainMenu->text = black;
    if (Menu_Button(mainMenu, "0   "))
    {
        playerAmount = 1;
    }
   if (Menu_Button(mainMenu, "00  "))
    {
        playerAmount = 2;
    }
    if (Menu_Button(mainMenu, "000 "))
    {
        playerAmount = 3;
    }
    if (Menu_Button(mainMenu, "0000"))
    {
        playerAmount = 4;
    }

    if (Menu_Button(mainMenu, "ALASA"))
    {
        return MainMenuStartGame;
    }

    Menu_DrawCursor(mainMenu);

    return MainMenuLoop;
}

int MainMenu_GetSelectedPlayerAmount()
{
    return playerAmount;
}

int MainMenu_GetSelectedMapIndex()
{
    return 0;
}
