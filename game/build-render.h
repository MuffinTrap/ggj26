#pragma once
#include "dukemap.h"

struct SectorRender
{
    int number;
    int leftX;
    int rightX;
};
typedef struct SectorRender SectorRender;

// TODO move this
struct Player
{
    int sectorNumber;
    vec3 position; //NOTE Z is up
    float angleRad;
    float Pitch;
};
typedef struct Player Player;


void BuildRender_Init(DukeMap* map);
void BuildRender_DrawFirstPerson(Player* player, DukeMap* map);
