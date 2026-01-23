#include "player.h"
#include "build-render.h"

void Player_UpdateMove(Player* player, WiiController* controller, RenderSettings2D* settings2D, RenderSettingsOpenGL* settingsGL)
{
	float turnSpeed = M_PI;
	float moveSpeed = player->moveSpeed;
	float moveSpeed2D = (float)moveSpeed/1024.0f * settings2D->widthPerDukeK;
	float moveSpeed3D = (float)moveSpeed/1024.0f * settingsGL->widthPerDukeK;
	float verticalSpeed = player->verticalSpeed;
	float verticalSpeed2D = (float)verticalSpeed/1024.0f * settings2D->heightPerDukeK;
	float verticalSpeed3D = (float)verticalSpeed/1024.0f * settingsGL->heightPerDukeK;
	float dt = mgdl_GetDeltaTime();

	vec2 jdir =  WiiController_GetNunchukJoystickDirection(controller);
	player->angleRad += jdir.x * turnSpeed * dt;

	mat3x3 rotation;
	mat3x3Identity(rotation);
	mat3x3RotateZ(rotation, player->angleRad);

	vec3 forward = vec3New(0.0f, 1.0f,0.0f);
	vec3 dir = mat3x3MultiplyVector(rotation, forward);

	player->position2D =  vec3Add(player->position2D, vec3Multiply(dir, -jdir.y * moveSpeed2D * dt));
	player->positionOpenGL =  vec3Add(player->positionOpenGL, vec3Multiply(dir, -jdir.y * moveSpeed3D* dt));

	if (WiiController_ButtonHeld(controller, Button1))
	{
		player->position2D.z += verticalSpeed2D * dt;
		player->positionOpenGL.z += verticalSpeed3D * dt;
	}
	if (WiiController_ButtonHeld(controller, Button2))
	{
		player->position2D.z -= verticalSpeed2D* dt;
		player->positionOpenGL.z -= verticalSpeed3D* dt;
	}
}
