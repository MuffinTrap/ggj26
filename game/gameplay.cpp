#include "gameplay.h"

void Gameplay_Init()
{
	for (int i = 0; i < TEMP_SPRITE_AMOUNT; ++i)
	{
		tempSprites[i].position = vec3(); // SET ON UPDATE
		tempSprites[i].cstat = CSTAT_SPRITE_INVISIBLE;
		if (i < MAX_BULLET_AMOUNT)
		{
			tempSprites[i].picnum = 0; // SET ON SPAWN, BULLET
		}
		else if (i >= MAX_BULLET_AMOUNT)
		{
			tempSprites[i].picnum = 0; // SET ON SPAWN, TREASURE
		}
		tempSprites[i].shade = 0;
		tempSprites[i].pal = 0;
		tempSprites[i].clipdist = 0;
		tempSprites[i].filler = 0;
		tempSprites[i].xrepeat = 64; // SET ON SPAWN
		tempSprites[i].yrepeat = 64; // SET ON SPAWN
		tempSprites[i].xoffset = 0;
		tempSprites[i].yoffset = 0;
		tempSprites[i].sectnum = 0; // SET ON UPDATE
		tempSprites[i].statnum = 0;
		tempSprites[i].ang = 0;
		tempSprites[i].owner = 0;
		tempSprites[i].xvel = 0;
		tempSprites[i].yvel = 0;
		tempSprites[i].zvel = 0;
		tempSprites[i].lotag = 0;
		tempSprites[i].hitag = 0;
		tempSprites[i].extra = 0;
	}

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
		if (SphereToSphereCollision(player->position, player->radius, treasureExit.position, treasureExit.radius))
		{
			// TODO: Game ends! This player wins!
		}
	}
	
	if (treasure.render)
	{
		tempSprites[MAX_BULLET_AMOUNT].cstat = CSTAT_SPRITE_NO_FLAGS;
		tempSprites[MAX_BULLET_AMOUNT].position = treasure.position;
		tempSprites[MAX_BULLET_AMOUNT].sectnum = treasure.sectorNumber;
	}
	else
	{
		tempSprites[MAX_BULLET_AMOUNT].cstat = CSTAT_SPRITE_INVISIBLE;
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
			tempSprites[i].cstat = CSTAT_SPRITE_INVISIBLE;
		}
		else
		{
			tempSprites[i].cstat = CSTAT_SPRITE_NO_FLAGS;
			tempSprites[i].position = bullets[i].position;
			tempSprites[i].sectnum = bullets[i].sectorNumber;

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
							treasure.render = true;
							treasure.sectorNumber = true;
							treasure.position = player->position;
							treasure.render = true;
							player[j].hasTreasure = false;
							player->stunTimer = mgdl_GetElapsedSeconds() + PLAYER_STUN_DURATION_WITH_TREASURE;
							// TODO: Bullet hit player with treasure sfx
						}
						else
						{
							player->stunTimer = mgdl_GetElapsedSeconds() + PLAYER_STUN_DURATION;
							// TODO: Bullet hit player sfx
						}
						
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