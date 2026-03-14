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
    int playerNumber;

    s16 sectorNumber;

    // Position in duke units
    vec3 position;
    vec3 prevPosition;
    // Direction as a normal vector
    vec2 direction;

    float angleRad;
    float pitchRad; // looking up and down

    float turnSpeedDegrees; ///< Current turnspeed
    float maxTurnSpeedDegrees;
    float turnAcceleration; ///< In degrees per second
    // These are in dukes
    vec2 moveVelocity;
    float moveAcceleration; ///< In dukes per second
    float maxMoveSpeed;
    float verticalSpeed;
    float fallingSpeed;
    float targetHeight; ///< What height the player is trying to reach
    float standingHeight; ///< How much above ground when standing

    float radius; ///< How far from wall the player is pushed

    // Shooting
    float shootTimer;
    float shootRate;
    bool shotThisFrame;
    vec3 shotOrigin;
    vec3 shotDirection;

    // Treasure
    bool hasTreasure;

    // Stun
    float stunTimer;
};
typedef struct Player Player;

const float CONTROLLER_DEADZONE = 0.4f;

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL, DukeMap* map, int amountPlayers);
bool Player_IsStunned(Player* player);
bool IsPointInsideRect(Rect rect, vec2 point);
