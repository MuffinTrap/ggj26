#pragma once

// This code handles how many players are in game and what map is played

#ifdef __cplusplus
extern "C" {
#endif

    enum MainMenuResult
    {
        MainMenuLoop,
        MainMenuStartGame
    };
    typedef enum MainMenuResult MainMenuResult;

void MainMenu_Init();
MainMenuResult MainMenu_Frame();
int MainMenu_GetSelectedPlayerAmount();
int MainMenu_GetSelectedMapIndex();


#ifdef __cplusplus
}
#endif
