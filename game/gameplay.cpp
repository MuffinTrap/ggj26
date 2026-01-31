#include "gameplay.h"

DSprite tempSprites[TEMP_SPRITE_AMOUNT];

void Gameplay_Init()
{
	for (int i = 0; i < TEMP_SPRITE_AMOUNT; ++i)
	{
		tempSprites[i].position = vec3(); // SET ON UPDATE
		tempSprites[i].cstat = Flag_Set(tempSprites[i].cstat, 1 << SPRITE_PIVOT_BIT); // SET ON UPDATE
		tempSprites[i].cstat = Flag_Set(tempSprites[i].cstat, 1 << CSTAT_SPRITE_INVISIBLE); // SET ON UPDATE
		if (i < MAX_BULLET_AMOUNT)
		{
			tempSprites[i].picnum = PICNUM_BULLET;
			tempSprites[i].extra = 150.0f;
		}
		else if (i == MAX_BULLET_AMOUNT)
		{
			tempSprites[i].picnum = PICNUM_TREASURE;
			tempSprites[i].extra = 400.0f;
		}
		else
		{
			tempSprites[i].picnum = PICNUM_PLAYER;
			tempSprites[i].extra = 400.0f;
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
		tempSprites[i].xvel = 0;
		tempSprites[i].yvel = 0;
		tempSprites[i].zvel = 0;
		tempSprites[i].lotag = 0;
		tempSprites[i].hitag = 0;
		tempSprites[i].owner = 9;
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
	treasure.position = vec3New(1789.2f, 0.0f, 525.3f);
	treasure.sectorNumber = 3;
	treasure.radius = 32.0f;

	// TODO: Get exit spawn from the map
	treasureExit.render = false;
	treasureExit.position = vec3New(-256.0f, 2048.0f, -127.0f);
	treasureExit.sectorNumber = 0;
	treasureExit.radius = 32.0f;
}

void Gameplay_Update(Player* player, DukeMap* map)
{
	float dt = mgdl_GetDeltaTime();

	// Update player model
	int index = MAX_BULLET_AMOUNT + 1 + player->playerNumber;
	tempSprites[index].position = player->position;
	tempSprites[index].owner = player->playerNumber;
	tempSprites[index].cstat = Flag_Unset(tempSprites[index].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
	tempSprites[index].picnum = player->hasTreasure ? PICNUM_PLAYER_WITH_MASK : PICNUM_PLAYER;

	// Update treasure
	if (!player->hasTreasure)
	{
		// Pick up treasure
		if (Gameplay_SphereToSphereCollision(player->position, player->radius, treasure.position, treasure.radius))
		{
			Log_Info("PICK UP TREASURE\n");
			player->hasTreasure = true;
			treasure.render = false;
		}
	}
	else // Return treasure check
	{
		if (Gameplay_SphereToSphereCollision(player->position, player->radius, treasureExit.position, treasureExit.radius))
		{
			Log_Info("GAME OVER!\n");
			// TODO: Game ends! This player wins!
		}
	}
	
	if (treasure.render)
	{
		tempSprites[MAX_BULLET_AMOUNT].cstat = Flag_Unset(tempSprites[MAX_BULLET_AMOUNT].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
		tempSprites[MAX_BULLET_AMOUNT].position = treasure.position;
		tempSprites[MAX_BULLET_AMOUNT].sectnum = treasure.sectorNumber;
	}
	else
	{
		tempSprites[MAX_BULLET_AMOUNT].cstat = Flag_Set(tempSprites[MAX_BULLET_AMOUNT].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
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
				bullets[i].playerNumber = player->playerNumber;
				player->shotThisFrame = false;
				Log_Info("BULLET SPAWNED\n");
			}
			tempSprites[i].cstat = Flag_Set(tempSprites[i].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
		}
		else
		{
			tempSprites[i].cstat = Flag_Unset(tempSprites[i].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
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
			bullets[i].position = vec3New(pointOut.x, movementEnd.y, pointOut.y);;
			bullets[i].sectorNumber = sectorOut;
			//Log_InfoF("BULLET UPDATE SECTOR: %i X: %.2f Y: %.2f Z: %.2f TIME: %.2f \n", bullets[i].sectorNumber, bullets[i].position.x, bullets[i].position.y, bullets[i].position.z, bullets[i].timeAlive);
			bool hitFloor = bullets[i].position.y < Map_GetSectorFloorHeight(map, bullets[i].sectorNumber);
			bool hitCeiling = bullets[i].position.y > Map_GetSectorCeilingHeight(map, bullets[i].sectorNumber);

			// Silently destroy out of bounds bullets
			bullets[i].timeAlive += dt;
			if (bullets[i].timeAlive > BULLET_MAX_TIME_ALIVE)
			{
				Log_Info("BULLET OUT OF BOUNDS\n");
				bullets[i].alive = false;
			}

			// Destory bullet when it hits wall
			if (result == Move_HitWall || hitFloor || hitCeiling)
			{
				Log_InfoF("BULLET HIT %s%s%s\n", (result == Move_HitWall) ? "WALL" : "", hitFloor ? "FLOOR" : "", hitCeiling ? "CEILING" : "");
				bullets[i].alive = false;
				// TODO: Play wall hit sfx
			}

			if (bullets[i].playerNumber != player->playerNumber)
			{
				if (Gameplay_SphereToSphereCollision(bullets[i].position, bullets[i].radius, player->position, player->radius))
				{
					bullets[i].alive = false;

					if (player->hasTreasure)
					{
						Log_Info("DROP TREASURE\n");
						treasure.render = true;
						treasure.sectorNumber = true;
						treasure.position = player->position;
						treasure.render = true;
						player->hasTreasure = false;
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

bool Gameplay_SphereToSphereCollision(vec3 pos1, float radius1, vec3 pos2, float radius2)
{
	float distanceSquared =
		pow(pos1.x - pos2.x, 2) +
		pow(pos1.y - pos2.y, 2) +
		pow(pos1.z - pos2.z, 2);
	float radiusSum = radius1 + radius2;
	return distanceSquared <= (radiusSum * radiusSum);
}