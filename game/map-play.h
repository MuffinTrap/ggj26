#pragma once
#include <mgdl/mgdl-types.h>

// This code handles the maps and players and gameplay
struct DukeMap;

#ifdef __cplusplus
extern "C" {
#endif

    enum MapPlayResult
    {
        MapPlayLoop,
        MapPlayEndMap,
        MapPlayReturnToMain
    };
    typedef enum MapPlayResult MapPlayResult;

    void MapPlay_Init();
    void MapPlay_ResetPlayers();

    MapPlayResult MapPlay_Frame();
    void MapPlay_StartMap(int mapIndex, int playerAmount);
    RectF MapPlay_GetPlayerScreenRect(int playerIndex, int amountPlayers);

    void MapPlay_ReloadActiveMap();
    DukeMap* MapPlay_LoadMap(const char* mapfile);

    MapPlayResult MapPlay_DrawMapPreview(float timerPercentage);
    int MapPlay_GetWinner();


#ifdef __cplusplus
}
#endif

