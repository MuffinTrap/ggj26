#include "gameplay.h"

void Gameplay_Init()
{
	for (int i = 0; i < MAX_BULLET_AMOUNT; ++i)
	{
		bullets[i].sectorNumber = 0;
		bullets[i].alive = false;
		bullets[i].timeAlive = 0.0f;
		bullets[i].position = vec3();
		bullets[i].direction = vec3();
		bullets[i].radius = 32.0f;
	}

	// TODO: Get treasure spawn from the map
	treasure.render = true;
	treasure.position = vec3New(0.0f, 0.0f, 0.0f);
	treasure.sectorNumber = 0;
	treasure.radius = 32.0f;

	// TODO: Get exit spawn from the map
	treasureExit.render = false;
	treasureExit.position = vec3New(0.0f, 0.0f, 0.0f);
	treasureExit.sectorNumber = 0;
	treasureExit.radius = 32.0f;
}

void Gameplay_Update(Player* player, DukeMap* map)
{
	float dt = mgdl_GetDeltaTime();

	// Update treasure
	if (!player->hasTreasure)
	{
		// Pick up treasure
		if (SphereToSphereCollision(player->position, player->radius, treasure.position, treasure.radius))
		{
			player->hasTreasure = true;
			treasure.render = false;
		}
	}
	else // Return treasure check
	{
		// TODO: Check if you are in an exit sector
		if (SphereToSphereCollision(player->position, player->radius, treasureExit.position, treasureExit.radius))
		{
			// TODO: Game ends! This player wins!
		}
	}
	
	if (treasure.render)
	{
		// TODO: Draw treasure
	}

	// Update bullets
	for (int i = 0; i < MAX_BULLET_AMOUNT; ++i)
	{
		if (!bullets[i].alive)
		{
			// Spawn new bullet, if shot this frame
			if (player->shotThisFrame)
			{
				bullets[i].alive = true;
				bullets[i].timeAlive = 0.0f;
				bullets[i].position = player->shotOrigin;
				bullets[i].direction = player->shotDirection;
				bullets[i].sectorNumber = player->sectorNumber;
				// TODO: Get ID of the player who shot the bullet
				bullets[i].playerNumber = 0;
				player->shotThisFrame = false;
				Log_Info("BULLET SPAWNED\n");
			}
		}
		else
		{
			// TODO: Draw alive bullets

			// Update bullets alive (-z forwads, thus negative speed)
			vec3 movementEnd = vec3Multiply(bullets[i].direction, -BULLET_SPEED * dt);
			movementEnd = vec3Add(bullets[i].position, movementEnd);

			vec2 point = vec2New(bullets[i].position.x, bullets[i].position.z);
			vec2 endpoint = vec2New(movementEnd.x, movementEnd.z);

			vec2 pointOut;
			s16 sectorOut;
			MoveResult result = Map_MovePointInMap(map, point, endpoint, bullets[i].sectorNumber, &pointOut, &sectorOut);
			bullets[i].position = vec3New(pointOut.x, bullets[i].position.y, pointOut.y);;
			bullets[i].sectorNumber = sectorOut;
			Log_InfoF("BULLET UPDATE SECTOR: %i X: %.2f Y: %.2f Z: %.2f TIME: %.2f \n", bullets[i].sectorNumber, bullets[i].position.x, bullets[i].position.y, bullets[i].position.z, bullets[i].timeAlive);

			// Silently destroy out of bounds bullets
			bullets[i].timeAlive += dt;
			if (bullets[i].timeAlive > BULLET_MAX_TIME_ALIVE)
			{
				Log_Info("BULLET OUT OF BOUNDS\n");
				bullets[i].alive = false;
			}

			// Destory bullet when it hits wall
			if (result == Move_HitWall)
			{
				Log_Info("BULLET HIT WALL\n");
				bullets[i].alive = false;
				// TODO: Play wall hit sfx
			}

			// TODO: Check bullet to player collisions
			// TODO: Update player amount
			for (int j = 0; j < 1; ++j)
			{
				if (bullets[i].playerNumber != j)
				{
					if (SphereToSphereCollision(bullets[i].position, bullets[i].radius, player->position, player->radius))
					{
						bullets[i].alive = false;

						if (player->hasTreasure)
						{
							// TODO: Bullet hit player with treasure sfx
							// TODO: Player drops treasure
							treasure.render = true;
							treasure.sectorNumber = true;
							treasure.position = player->position;
							treasure.render = true;
							player[j].hasTreasure = false;
						}
						else
						{
							// TODO: Bullet hit player sfx
						}
						// TODO: Stun player
					}
				}
			}
		}
	}
}

bool SphereToSphereCollision(vec3 pos1, float radius1, vec3 pos2, float radius2)
{
	float distanceSquared =
		pow(pos1.x - pos2.x, 2) +
		pow(pos1.y - pos2.y, 2) +
		pow(pos1.z - pos2.z, 2);
	float radiusSum = radius1 + radius2;
	return distanceSquared <= (radiusSum * radiusSum);
}