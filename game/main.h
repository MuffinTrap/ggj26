#pragma once

// This has game wide defines and things

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PLAYERS 4 // How many players can the game have at maximum

enum GameState
{
    Game_MainMenu,
    Game_MapLobby,
    Game_MapPlay,
    Game_MapOver
};
typedef enum GameState GameState;

#ifdef __cplusplus
}
#endif

