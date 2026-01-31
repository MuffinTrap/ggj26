#pragma once
#include <mgdl.h>
#include <mgdl/ccVector/ccVector.h>
#include "player.h"
#include "dukemap.h"

const int MAX_BULLET_AMOUNT = 32;
const float BULLET_SPEED = 2048.0f;
const float BULLET_MAX_TIME_ALIVE = 5.0f;

struct Bullet
{
    s16 sectorNumber;
    bool alive;
    float timeAlive;
    vec3 position;
    vec3 direction;
};
typedef struct Bullet Bullet;

static Bullet bullets[MAX_BULLET_AMOUNT];

void Gameplay_Init();
void Gameplay_Update(Player* player, DukeMap* map);