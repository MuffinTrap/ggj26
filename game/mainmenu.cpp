#include "mainmenu.h"
#include <mgdl.h>
#include <mgdl/mgdl-main.h>

    static int playerAmount = 1;
    static int mapIndex = 0;
    static Menu* mainMenu;
    static Menu* mapMenu;
    static Font* menuFont;

    static Texture* betaText;

    Color4f orange = Color_HexToFloats(0xffbe03ff);
    Color4f black = Color_HexToFloats(0x020202ff);
    Color4f white = Color_HexToFloats(0xfffcfcfc);
    Color4f grey = Color_HexToFloats(0xff4c4c4c);
    Color4f transpa = Color_HexToFloats(0x00000000);

void MainMenu_Init()
{


    betaText = mgdl_LoadTexture("assets/betamaze_36x36.png", Nearest);

    menuFont =  Font_LoadSelective(betaText, 36, 36, 7, " \"(),.0123456789:;ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    mainMenu = Menu_Create(menuFont, 2, 1.0f);
    mapMenu = Menu_Create(menuFont, 2, 1.2f);
    // TODO set colors etc
    // color ffbe03
    mainMenu->bg = transpa;
    mainMenu->text = black;
    mainMenu->highlight = white;
}

MainMenuResult MainMenu_Frame()
{
    mgdl_glClearColor4f(&orange);
    mgdl_InitOrthoProjection();
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());
    mgdl_glSetTransparency(true);
    int fontsize = 32;

    mainMenu->font = menuFont;
    Menu_Start(mainMenu, fontsize, mgdl_GetScreenHeight()-8, fontsize * 2 * 6);

    //mainMenu->text = grey;
    int olddx = mainMenu->drawx;
    int olddy = mainMenu->drawy;

    // Draw player amount to the side
    mainMenu->drawx = mgdl_GetScreenWidth() - 36 * 3;

    Menu_TextF(mainMenu, "J\nA\nN\n_\n_\n%d", playerAmount);

    mainMenu->drawx = olddx;
    mainMenu->drawy = olddy;
    //mainMenu->text = black;
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

    Menu_Skip(mainMenu, 36 * 2);
    if (Menu_Button(mainMenu, "ALASA"))
    {
        return MainMenuStartGame;
    }

    Menu_Start(mapMenu, mgdl_GetScreenWidth()/2 + 36, mgdl_GetScreenHeight(), 36 * 2 * 4);

    if (Menu_Button(mapMenu, "M 1"))
    {
        mapIndex = 0;
    }
   if (Menu_Button(mapMenu, "M 2"))
    {
        mapIndex = 1;
    }
    if (Menu_Button(mapMenu, "M 3"))
    {
        mapIndex = 2;
    }

    mainMenu->font = DefaultFont_GetDefaultFont();
    Menu_DrawCursor(mainMenu);

    mgdl_glSetTransparency(false);
    return MainMenuLoop;
}

int MainMenu_GetSelectedPlayerAmount()
{
    return playerAmount;
}

int MainMenu_GetSelectedMapIndex()
{
    return mapIndex;
}
