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
    s16 sectorNumber;

    // Position in duke units
    vec3 position;
    vec3 prevPosition;
    // Direction as a normal vector
    vec2 direction;

    float angleRad;
    float pitchRad; // looking up and down

    float turnSpeedDegrees;
    // These are in dukes
    float moveSpeed;
    float verticalSpeed;
    float fallingSpeed;
    float standingHeight; ///< How much above ground when standing
    float kneelingHeight; ///< How much above ground when kneeling/crouching

    float radius;

    // Shooting
    float shootTimer;
    float shootRate;
    bool shotThisFrame;
    vec3 shotOrigin;
    vec3 shotDirection;
};
typedef struct Player Player;

const float CONTROLLER_DEADZONE = 0.4f;

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL, DukeMap* map);
