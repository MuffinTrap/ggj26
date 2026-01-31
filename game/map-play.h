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
        MapPlayEndMap
    };
    typedef enum MapPlayResult MapPlayResult;

    void MapPlay_Init();

    MapPlayResult MapPlay_Frame();
    void MapPlay_StartMap(int mapIndex, int playerAmount);
    Rect MapPlay_GetPlayerScreenRect(int playerIndex, int amountPlayers);

    void MapPlay_ReloadActiveMap();
    DukeMap* MapPlay_LoadMap(const char* mapfile);


#ifdef __cplusplus
}
#endif

