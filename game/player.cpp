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

	float dt = mgdl_GetDeltaTime();

	float turnInput = 0.0f;
	// Use right joystick for turning on Windows
#if defined (MGDL_PLATFORM_WINDOWS) || defined (MGDL_PLATFORM_LINUX)|| defined (MGDL_PLATFORM_MAC)
	turnInput = WiiController_GetRoll(controller);
	// NOTE Roll is in radians
	if (abs(turnInput) < CONTROLLER_DEADZONE * M_PI) turnInput = 0.0f;

#else
	// Use dpad for turning on Wii
	if (WiiController_ButtonHeld(controller, ButtonRight)) turnInput = 1.0f;
	else if (WiiController_ButtonHeld(controller, ButtonLeft)) turnInput = -1.0f;
#endif
	
	// NOTE turning left around Y is positive
	// Forward is 0
	// Left is 90
	// right is -90
	// So when joystick is turned right: it is positive : decrease rotation
	if (turnInput == 0.0f)
	{
		player->turnSpeedDegrees = 0.0f; // Stop turning
	}
	else
	{
		player->turnSpeedDegrees -= turnInput * player->turnAcceleration * dt;
		if (player->turnSpeedDegrees > player->maxTurnSpeedDegrees)
		{
			player->turnSpeedDegrees = player->maxTurnSpeedDegrees;
		}
		else if (player->turnSpeedDegrees < -player->maxTurnSpeedDegrees)
		{
			player->turnSpeedDegrees = -player->maxTurnSpeedDegrees;
		}
	}
	player->angleRad += Deg2Rad(player->turnSpeedDegrees) * dt;

	// Moving forward and backward
	vec2 forward = vec2New(WORLD_FORWARD.x, WORLD_FORWARD.z);
	player->direction = Vec2XZRotateY(forward, player->angleRad);
	vec2 strafeDirection = Vec2XZRotateY(forward, player->angleRad - Deg2Rad(90.0f));

	float forwardInput = 0.0f;
	float strafeInput = 0.0f;

	vec2 jdir = WiiController_GetNunchukJoystickDirection(controller);
	if (abs(jdir.x) < CONTROLLER_DEADZONE) jdir.x = 0.0f;
	if (abs(jdir.y) < CONTROLLER_DEADZONE) jdir.y = 0.0f;

	// NOTE  Joystick dir -Y is forward, +Y is backwards
	forwardInput = -jdir.y;
	strafeInput = jdir.x;

	if (forwardInput == 0.0f && strafeInput == 0.0f)
	{
		player->moveVelocity.x = 0.0f;
		player->moveVelocity.y = 0.0f;
	}
	else
	{
		vec2 moveXZ = vec2Multiply(player->direction, forwardInput * player->moveAcceleration);
		vec2 strafe = vec2Multiply(strafeDirection, strafeInput * player->moveAcceleration);

		player->moveVelocity = vec2Add(player->moveVelocity, vec2Multiply(moveXZ, dt));
		player->moveVelocity = vec2Add(player->moveVelocity, vec2Multiply(strafe, dt));
		if (vec2Length(player->moveVelocity) > player->maxMoveSpeed)
		{
			player->moveVelocity = vec2Multiply( vec2Normalize(player->moveVelocity), player->maxMoveSpeed);
		}
	}

	// Store old
	player->prevPosition = player->position;

	// Apply move
	player->position.x += player->moveVelocity.x * dt;
	player->position.z += player->moveVelocity.y * dt;


	if (player->targetHeight < player->position.y)
	{
		player->position.y -= player->fallingSpeed * dt;
		if (player->targetHeight > player->position.y)
		{
			player->position.y = player->targetHeight;
		}
	}
	else if (player->targetHeight > player->position.y)
	{
		player->position.y += player->verticalSpeed * dt;
		if (player->position.y > player->targetHeight )
		{
			player->position.y = player->targetHeight;
		}
	}


	// Test if collides with a wall of this sector that is not a portal
        // Get the sector info from map

        // Keep doing this check until player is inside

	vec2 point = vec2New(player->prevPosition.x, player->prevPosition.z);
	vec2 endpoint = vec2New(player->position.x, player->position.z);

	vec2 pointOut;
	s16 sectorOut;
	MoveResult result = Map_MovePointInMap(map, point, endpoint, player->sectorNumber, player->radius, &pointOut, &sectorOut);
	if (result == MoveResult::Move_Cancel)
	{
		player->position = player->prevPosition;
	}
	else
	{
		if (sectorOut != player->sectorNumber)
		{
			// new sector!
			// check if player can climb
			float newFloor = Map_GetSectorFloorHeight(map, sectorOut);
			// If the new floor is lower than third of height
			if (newFloor < player->position.y + player->standingHeight/3.0f)
			{
				// Keep player on floor and under the ceiling
				player->targetHeight = newFloor + player->standingHeight;
				player->sectorNumber = sectorOut;
				player->position = vec3New(pointOut.x, player->position.y, pointOut.y);;
			}
			else
			{
				// Cannot climb
				player->position = player->prevPosition;
			}
			// player->position.y = minF(player->position.y, Map_GetSectorCeilingHeight(map, player->sectorNumber));
		}
		else
		{
			// Moving inside same sector
			player->position = vec3New(pointOut.x, player->position.y, pointOut.y);;
		}
	}


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


				// Initial direction, without cursor addon
				vec3 bulletDir = vec3New(player->direction.x, 0.0f, player->direction.y);

#if defined(MGDL_PLATFORM_WII)
				// Calculate the cursor position on the near plane
				// and calculate direction in the world
				//Log_InfoF("CURSOR X: %.2f Y: %.2f\n", cursorPosition.x, cursorPosition.y);
				vec3 cursorWorld = CalculateCursorWorldPos(cursorPosition, screenRect, settingsGL);
				vec3 playerGLpos = Vec3DukePosToOpenGL(player->position, settingsGL);
				bulletDir = vec3Normalize( vec3Subtract(cursorWorld, playerGLpos));
				//Log_InfoF("Bullet dir %.2f, %.2f, %.2f\n", bulletDir.x, bulletDir.y, bulletDir.z);
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
