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
	player->angleRad += jdir.x * turnSpeed * dt;

	mat3x3 rotation;
	mat3x3Identity(rotation);
	mat3x3RotateZ(rotation, player->angleRad);

	vec3 forward = vec3New(0.0f, 1.0f,0.0f);
	player->direction = mat3x3MultiplyVector(rotation, forward);

	player->prevPositionOpenGL = player->positionOpenGL; // Store old
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

	// Test if collides with a wall of this sector that is not a portal
        // Get the sector info from map

        // Keep doing this check until player is inside

		vec2 point = player->prevPositionOpenGL.xy;
		vec2 endpoint = player->positionOpenGL.xy;
		vec2 cross;
		Sector* sector = Map_GetSector(map, player->sectorNumber);

		// Keep testing until player is back inside again
		// TODO Ring buffer of previous positions
			player->insideSector =  Map_IsPointInsideSectorOG(map, endpoint, player->sectorNumber);
			if (player->insideSector == true)
			{
				// TODO does player hit head
				float ceilingY = sector->ceilingz * -1;
				float floorY = sector->floorz * -1;
				player->positionOpenGL.z = floorY + player->standingHeight;
				if (player->positionOpenGL.z  > ceilingY)
				{
					player->positionOpenGL.z = ceilingY - player->standingHeight - 1;
				}
			}
			else
			{
				bool foundPortal = false;
				bool foundWall = false;
				for (s16 wi = 0; wi < sector->wallnum; wi++)
				{
					Wall* w = Map_GetWallInSector(map, player->sectorNumber, wi);
					// Did player cross this wall
					if (Map_FindIntersectionWithWall(point, endpoint, w, &cross))
					{
						// Yes
						// Is it a portal?
						if (w->nextsector >= 0)
						{
							player->sectorNumber = w->nextsector;
							foundPortal = true;
							player->positionOpenGL.x = endpoint.x;
							player->positionOpenGL.y = endpoint.y;
							break;
						}
						else
						{
							foundWall = true;
							vec2 normal = Wall_GetNormal(w);
							// Push player back
							player->positionOpenGL.x = cross.x + normal.x;
							player->positionOpenGL.y = cross.y + normal.y;
							// TODO Slide along the wall
							vec2 move = Vec2Project( vec2Subtract(endpoint, point), vec2Subtract(w->end, w->start));
							player->positionOpenGL.x += move.x;
							player->positionOpenGL.y += move.y;
							break;
						}
					}
				}
				if (foundPortal == false && foundWall == false)
				{
					// DANGER Player has escaped: return to original position and reverse
					player->positionOpenGL = player->prevPositionOpenGL;
					vec2 reversed = vec2Add(player->positionOpenGL.xy, vec2Multiply(player->direction.xy, -moveSpeed3D * dt));
					player->positionOpenGL.x = reversed.x;
					player->positionOpenGL.y = reversed.y;
				}
			}
}
