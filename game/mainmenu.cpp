#include "mainmenu.h"
#include <mgdl.h>

    static int playerAmount = 1;
    static Menu* mainMenu;
    static Font* menuFont;


void MainMenu_Init()
{
    // menuFont = mgdl_LoadFont()
    menuFont = DefaultFont_GetDefaultFont();
    mainMenu = Menu_Create(menuFont, 4, 1.2f);
    // TODO set colors etc

}

MainMenuResult MainMenu_Frame()
{
    mgdl_InitOrthoProjection();
    glViewport(0, 0, mgdl_GetScreenWidth(), mgdl_GetScreenHeight());
    // TODO Draw menu
    Menu_Start(mainMenu, 0, mgdl_GetScreenHeight(), mgdl_GetScreenWidth());
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
