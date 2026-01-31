#pragma once
#include <mgdl.h>
#include <mgdl/ccVector/ccVector.h>
#include "player.h"
#include "dukemap.h"
#include "math.h"

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
    float radius;
    int playerNumber;
};
typedef struct Bullet Bullet;

struct WorldObject
{
    bool render;
    s16 sectorNumber;
    vec3 position;
    float radius;
};
typedef struct WorldObject WorldObject;

static Bullet bullets[MAX_BULLET_AMOUNT];
static WorldObject treasure;
static WorldObject treasureExit;

void Gameplay_Init();
void Gameplay_Update(Player* player, DukeMap* map);
bool SphereToSphereCollision(vec3 pos1, float radius1, vec3 pos2, float radius2);