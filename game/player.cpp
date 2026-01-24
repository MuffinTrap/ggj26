#include "player.h"
#include "build-render.h"

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL)
{
	float turnSpeed = Deg2Rad(player->turnSpeedDegrees);
	float moveSpeed = player->moveSpeed;
	float moveSpeed3D = moveSpeed;
	float verticalSpeed = player->verticalSpeed;
	float verticalSpeed3D = verticalSpeed;
	float dt = mgdl_GetDeltaTime();

	vec2 jdir =  WiiController_GetNunchukJoystickDirection(controller);
	player->angleRad += jdir.x * turnSpeed * dt;

	mat3x3 rotation;
	mat3x3Identity(rotation);
	mat3x3RotateZ(rotation, player->angleRad);

	vec3 forward = vec3New(0.0f, 1.0f,0.0f);
	player->direction = mat3x3MultiplyVector(rotation, forward);

	// player->position2D =  vec3Add(player->position2D, vec3Multiply(dir, -jdir.y * moveSpeed2D * dt));
	player->positionOpenGL =  vec3Add(player->positionOpenGL, vec3Multiply(player->direction, -jdir.y * moveSpeed3D* dt));

	if (WiiController_ButtonHeld(controller, Button1))
	{
		//player->position2D.z += verticalSpeed2D * dt;
		player->positionOpenGL.z += verticalSpeed3D * dt;
	}
	if (WiiController_ButtonHeld(controller, Button2))
	{
		//player->position2D.z -= verticalSpeed2D* dt;
		player->positionOpenGL.z -= verticalSpeed3D* dt;
	}
}
