#include "mainmenu.h"
#include <mgdl.h>

    static int playerAmount = 1;
    static int mapIndex = 0;
    static Menu* mainMenu;
    static Menu* mapMenu;
    static Font* menuFont;


void MainMenu_Init()
{
    // menuFont = mgdl_LoadFont()
    menuFont = DefaultFont_GetDefaultFont();
    mainMenu = Menu_Create(menuFont, 2, 1.2f);
    mapMenu = Menu_Create(menuFont, 2, 1.2f);
    // TODO set colors etc
    // color ffbe03

}

MainMenuResult MainMenu_Frame()
{
    mgdl_InitOrthoProjection();
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());
    // TODO Draw menu
    Menu_Start(mainMenu, 0, mgdl_GetScreenHeight(), mgdl_GetScreenWidth()/2);
    Menu_Text(mainMenu, "MAIN MENU");

    if (Menu_Button(mainMenu, "1 PLAYER"))
    {
        playerAmount = 1;
    }
   if (Menu_Button(mainMenu, "2 PLAYERS"))
    {
        playerAmount = 2;
    }
    if (Menu_Button(mainMenu, "3 PLAYERS"))
    {
        playerAmount = 3;
    }
    if (Menu_Button(mainMenu, "4 PLAYERS"))
    {
        playerAmount = 4;
    }
    Menu_TextF(mainMenu, "SELECTED %d", playerAmount);

    if (Menu_Button(mainMenu, "START"))
    {
        return MainMenuStartGame;
    }

    Menu_Start(mapMenu, mgdl_GetScreenWidth()/2, mgdl_GetScreenHeight(), mgdl_GetScreenWidth()/2);
    if (Menu_Button(mapMenu, "Map 1"))
    {
        mapIndex = 0;
    }
    if (Menu_Button(mapMenu, "Map 2"))
    {
        mapIndex = 1;
    }
    if (Menu_Button(mapMenu, "Map 3"))
    {
        mapIndex = 2;
    }
    Menu_TextF(mapMenu, "SELECTED %d", mapIndex + 1);

    Menu_DrawCursor(mainMenu);

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
