#pragma once
#include <mgdl.h>
#include <mgdl/ccVector/ccVector.h>
#include "player.h"
#include "dukemap.h"
#include "math.h"

#define CSTAT_SPRITE_INVISIBLE 15

#define PICNUM_BULLET 99
#define PICNUM_TREASURE 98
#define PICNUM_PLAYER 97
#define PICNUM_PLAYER_WITH_MASK 96

const int MAX_BULLET_AMOUNT = 32;
const float BULLET_SPEED = 2048.0f;
const float BULLET_MAX_TIME_ALIVE = 5.0f;
const float PLAYER_STUN_DURATION = 3.0f;
const float PLAYER_STUN_DURATION_WITH_TREASURE = 6.0f;

// 0-31 bullets
// 32 treasure
// 33 player 0
// 34 player 1
// 35 player 2
// 36 player 3
#define TEMP_SPRITE_AMOUNT 37
extern DSprite tempSprites[TEMP_SPRITE_AMOUNT];

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
bool Gameplay_SphereToSphereCollision(vec3 pos1, float radius1, vec3 pos2, float radius2);
int Gameplay_GetWinner();