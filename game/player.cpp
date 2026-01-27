#include "player.h"
#include "build-render.h"

#include "dukemap.h"
#include "dukemath.h"

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL, DukeMap* map)
{
	float turnSpeed = Deg2Rad(player->turnSpeedDegrees);
	float moveSpeed = player->moveSpeed;
	float moveSpeed3D = moveSpeed;
	float verticalSpeed = player->verticalSpeed;
	float verticalSpeed3D = verticalSpeed;
	float dt = mgdl_GetDeltaTime();

	vec2 jdir =  WiiController_GetNunchukJoystickDirection(controller);
	// NOTE turning left around Y is positive
	// Forward is 0
	// Left is 90
	// right is -90
	// So when joystick is turned right: it is positive : decrease rotation
	player->angleRad += jdir.x * turnSpeed * dt;


	vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
	player->direction = Vec2XZRotateY(forward, player->angleRad);

	player->prevPosition = player->position; // Store old
	// NOTE  Joystick dir -Y is forward, +Y is backwards
	// But -Z is forward so...
	vec2 moveXZ = vec2Multiply(player->direction, jdir.y * moveSpeed3D* dt);
	player->position.x += moveXZ.x;
	player->position.z += moveXZ.y;

	// Jetback controls?
	if (WiiController_ButtonHeld(controller, Button1))
	{
		player->position.y += verticalSpeed3D * dt;
	}
	else
	{
		player->position.y -= player->fallingSpeed * dt;
	}
	if (WiiController_ButtonHeld(controller, Button2))
	{
		player->position.y -= verticalSpeed3D* dt;
	}

	// Test if collides with a wall of this sector that is not a portal
        // Get the sector info from map

        // Keep doing this check until player is inside

	vec2 point = vec2New(player->prevPosition.x, player->prevPosition.z);
	vec2 endpoint = vec2New(player->position.x, player->position.z);

	vec2 pointOut;
	s16 sectorOut;
	MoveResult result = Map_MovePointInMap(map, point, endpoint, player->sectorNumber, &pointOut, &sectorOut);
	// TODO Sector height
	player->position = vec3New(pointOut.x, player->position.y, pointOut.y);;
	player->sectorNumber = sectorOut;

	// Keep player on floor
	player->position.y = maxF(player->position.y, Map_GetSectorFloorHeight(map, player->sectorNumber) + player->standingHeight);
}
