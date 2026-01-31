#include "gameplay.h"

DSprite tempSprites[TEMP_SPRITE_AMOUNT];
int winnerPlayerIndex = -1;

Sound* sfxShoot;
Sound* sfxHitWall;
Sound* sfxHitPlayer;
Sound* sfxHitPlayerWithMask;
Sound* sfxPickupMask;

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
			tempSprites[i].extra = 900.0f;
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

	sfxShoot = mgdl_LoadSoundWav("assets/blipSelect.wav");
	sfxHitWall = mgdl_LoadSoundWav("assets/blipSelect.wav");
	sfxHitPlayer = mgdl_LoadSoundWav("assets/blipSelect.wav");
	sfxHitPlayerWithMask = mgdl_LoadSoundWav("assets/blipSelect.wav");
	sfxPickupMask = mgdl_LoadSoundWav("assets/blipSelect.wav");

	Gameplay_Reset();
}

void Gameplay_Reset()
{
	winnerPlayerIndex = -1;

	// TODO: Get treasure spawn from the map
	treasure.render = true;
	treasure.position = vec3New(1789.2f, -2048.0f, 525.3f);
	treasure.sectorNumber = 3;
	treasure.radius = 32.0f;

	for (int i = 0; i < MAX_BULLET_AMOUNT; ++i)
	{
		bullets[i].sectorNumber = 0;
		bullets[i].alive = false;
		bullets[i].timeAlive = 0.0f;
		bullets[i].position = vec3();
		bullets[i].direction = vec3();
		bullets[i].radius = 128.0f;
	}
}

void Gameplay_Update(Player* player, DukeMap* map)
{
	float dt = mgdl_GetDeltaTime();

	// Update player model
	int index = MAX_BULLET_AMOUNT + 1 + player->playerNumber;
	tempSprites[index].position = player->position;
	tempSprites[index].owner = player->playerNumber;
	tempSprites[index].cstat = Flag_Unset(tempSprites[index].cstat, 1 << CSTAT_SPRITE_INVISIBLE);
	if (Player_IsStunned(player))
	{
		tempSprites[index].picnum = PICNUM_PLAYER_SHOCK;
	}
	else
	{
		tempSprites[index].picnum = player->hasTreasure ? PICNUM_PLAYER_WITH_MASK : PICNUM_PLAYER;
	}

	// Update treasure
	if (!player->hasTreasure)
	{
		// Pick up treasure
		if (treasure.render
			&& Gameplay_SphereToSphereCollision(player->position, player->radius, treasure.position, treasure.radius)
			&& !Player_IsStunned(player))
		{
			Log_Info("PICK UP TREASURE\n");
			player->hasTreasure = true;
			treasure.render = false;
			Audio_PlaySound(sfxPickupMask);
		}
	}
	else // Return treasure check
	{
		if (Map_GetSector(map, player->sectorNumber)->lotag == LEVEL_END_LOTAG)
		{
			winnerPlayerIndex = player->playerNumber;
			Log_InfoF("WINNER: %i\n", winnerPlayerIndex);
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
}

void Gameplay_UpdateBullets(Player* players, int playerAmount, DukeMap* map)
{
	float dt = mgdl_GetDeltaTime();

	for (int i = 0; i < MAX_BULLET_AMOUNT; ++i)
	{
		if (!bullets[i].alive)
		{
			// Spawn new bullet, if shot this frame
			for (int j = 0; j < playerAmount; ++j)
			{
				if (players[j].shotThisFrame)
				{
					bullets[i].alive = true;
					bullets[i].timeAlive = 0.0f;
					bullets[i].position = players[j].shotOrigin;
					bullets[i].direction = players[j].shotDirection;
					bullets[i].sectorNumber = players[j].sectorNumber;
					bullets[i].playerNumber = players[j].playerNumber;
					players[j].shotThisFrame = false;
					Audio_PlaySound(sfxShoot);
					Log_Info("BULLET SPAWNED\n");
				}
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

			// Destroy bullet when it hits wall
			if (result == Move_HitWall || hitFloor || hitCeiling)
			{
				Log_InfoF("BULLET HIT %s%s%s\n", (result == Move_HitWall) ? "WALL" : "", hitFloor ? "FLOOR" : "", hitCeiling ? "CEILING" : "");
				bullets[i].alive = false;
				Audio_PlaySound(sfxHitWall);
			}

			// Check bullet to player collisions
			for (int j = 0; j < playerAmount; ++j)
			{
				if (bullets[i].playerNumber != players[j].playerNumber)
				{
					if (Gameplay_SphereToSphereCollision(bullets[i].position, bullets[i].radius, players[j].position, players[j].radius))
					{
						bullets[i].alive = false;

						if (players[j].hasTreasure)
						{
							Log_Info("DROP TREASURE\n");
							treasure.render = true;
							treasure.sectorNumber = true;
							treasure.position = players[j].position;
							treasure.render = true;
							players[j].hasTreasure = false;
							players[j].stunTimer = mgdl_GetElapsedSeconds() + PLAYER_STUN_DURATION_WITH_TREASURE;
							Audio_PlaySound(sfxHitPlayerWithMask);
						}
						else
						{
							players[j].stunTimer = mgdl_GetElapsedSeconds() + PLAYER_STUN_DURATION;
							Audio_PlaySound(sfxHitPlayer);
						}
						Log_InfoF("BULLET HIT PLAYER %i\n", players[j].playerNumber);
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

/// Get index of winning player.
/// -1 means, no winner currently
int Gameplay_GetWinner()
{
	return winnerPlayerIndex;
}
