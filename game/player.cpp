#include "player.h"
#include "build-render.h"

#include "dukemap.h"
#include "dukemath.h"
#include "math.h"
#include "map-play.h"

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL, DukeMap* map, int amountPlayers)
{
	// Skip movement if stunned
	if (Player_IsStunned(player))
	{
		return;
	}

	float turnSpeed = Deg2Rad(player->turnSpeedDegrees);
	float moveSpeed = player->moveSpeed;
	float moveSpeed3D = moveSpeed;
	float verticalSpeed = player->verticalSpeed;
	float verticalSpeed3D = verticalSpeed;
	float dt = mgdl_GetDeltaTime();

	// Use right joystick for turning on Windows
#if defined (MGDL_PLATFORM_WINDOWS) || defined (MGDL_PLATFORM_LINUX)
	float turn = WiiController_GetRoll(controller);
	if (abs(turn) < CONTROLLER_DEADZONE) turn = 0.0f;

	// Use dpad for turning on Wii
#else
	float turn = 0.0f;
	if (WiiController_ButtonHeld(controller, ButtonRight)) turn = 1.0f;
	else if (WiiController_ButtonHeld(controller, ButtonLeft)) turn = -1.0f;
#endif
	
	// NOTE turning left around Y is positive
	// Forward is 0
	// Left is 90
	// right is -90
	// So when joystick is turned right: it is positive : decrease rotation
	player->angleRad -= turn * turnSpeed * dt;

	vec2 jdir = WiiController_GetNunchukJoystickDirection(controller);
	if (abs(jdir.x) < CONTROLLER_DEADZONE) jdir.x = 0.0f;
	if (abs(jdir.y) < CONTROLLER_DEADZONE) jdir.y = 0.0f;

	vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
	player->direction = Vec2XZRotateY(forward, player->angleRad);
	vec2 strafeDirection = Vec2XZRotateY(forward, player->angleRad - Deg2Rad(90.0f));

	// NOTE  Joystick dir -Y is forward, +Y is backwards
	// But -Z is forward so...
	vec2 moveXZ = vec2Multiply(player->direction, -jdir.y * moveSpeed3D * dt);
	vec2 strafe = vec2Multiply(strafeDirection, jdir.x * moveSpeed3D * dt);

	// Store old
	player->prevPosition = player->position;

	// Apply move
	player->position.x += moveXZ.x + strafe.x;
	player->position.z += moveXZ.y + strafe.y;

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

	player->position = vec3New(pointOut.x, player->position.y, pointOut.y);;
	player->sectorNumber = sectorOut;

	// Keep player on floor and under the ceiling
	player->position.y = maxF(player->position.y, Map_GetSectorFloorHeight(map, player->sectorNumber) + player->standingHeight);
	player->position.y = minF(player->position.y, Map_GetSectorCeilingHeight(map, player->sectorNumber));

	// Shooting, when trigger is pressed, cursor is within viewport and shoot timer is ok
	vec2 cursorPosition = WiiController_GetCursorPosition(controller);
	Rect screenRect = MapPlay_GetPlayerScreenRect(player->playerNumber, amountPlayers);
#ifdef MGDL_PLATFORM_WII
	if (IsPointInsideRect(screenRect, cursorPosition))
	{
#endif
		if (WiiController_ButtonHeld(controller, ButtonB))
		{
			if (mgdl_GetElapsedSeconds() > player->shootTimer)
			{
				// Bullet spawning will clear this
				player->shotThisFrame = true;

				// Limit shooting speed
				player->shootTimer = mgdl_GetElapsedSeconds() + player->shootRate;

				// Data for bullet initialization
				player->shotOrigin = player->position;

				// Add cursor position to shooting direction
				// 0,0 is the center of the screen
				// x right & y up is positive

				vec2 relativeScreenPosition = vec2New((cursorPosition.x - screenRect.x) / screenRect.w, (cursorPosition.y - screenRect.y) / screenRect.h);

				//vec2 relativeScreenPosition = vec2New(screenPosition.x / mgdl_GetScreenWidth(), screenPosition.y / mgdl_GetScreenHeight());
				relativeScreenPosition.x -= 0.5f;
				relativeScreenPosition.y -= 0.5f;
				Log_InfoF("CURSOR X: %.2f Y: %.2f\n", relativeScreenPosition.x, relativeScreenPosition.y);

				// Initial direction, without cursor addon
				vec3 bulletDir = vec3New(player->direction.x, 0.0f, player->direction.y);

#ifdef MGDL_PLATFORM_WII
				// Horizontal addon
				vec3 bulletRight = vec3CrossProduct(WORLD_UP, vec3Normalize(bulletDir));
				bulletRight = vec3Multiply(bulletRight, -relativeScreenPosition.x * 2.0f);
				bulletDir = vec3Add(bulletDir, bulletRight);

				// Vertical addon
				bulletDir.y -= relativeScreenPosition.y * 20.0f;
#endif
				// Final direction
				player->shotDirection = bulletDir;
				Log_Info("BULLET SHOT\n");
			}
		}
#ifdef MGDL_PLATFORM_WII
	}
#endif
}

bool Player_IsStunned(Player* player)
{
	return (player->stunTimer > mgdl_GetElapsedSeconds());
}

bool IsPointInsideRect(Rect rect, vec2 point)
{
	return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}
