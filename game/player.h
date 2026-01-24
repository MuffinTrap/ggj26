#pragma once
#include <mgdl.h>
#include <mgdl/ccVector/ccVector.h>

struct DukeMap;
struct RenderSettings2D;
struct RenderSettingsOpenGL;
/**
 * @brief This is the player data.
 * @details It starts with the info loaded from the map
 * and then comes game specific stuff
 */
struct Player
{
    int sectorNumber;
    vec3 positionOpenGL; //NOTE Z is up
    vec3 prevPositionOpenGL;
    vec3 direction;
    float angleRad;
    float Pitch;

    float turnSpeedDegrees;
    // These are in dukes
    float moveSpeed;
    float verticalSpeed;
    float standingHeight; ///< How much above ground when standing
    float kneelingHeight; ///< How much above ground when kneeling/crouching

    float radius;

    // Collision info
    bool insideSector;
};
typedef struct Player Player;

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL, DukeMap* map);
