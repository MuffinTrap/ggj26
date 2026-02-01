#include "gameplay.h"

DSprite tempSprites[TEMP_SPRITE_AMOUNT];
int winnerPlayerIndex = -1;

int shootSfxIndex = 0;
Sound* sfxShoot[8];
int wallSfxIndex = 0;
Sound* sfxHitWall[8];
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
			tempSprites[i].cstat = Flag_Unset(tempSprites[i].cstat, 1 << SPRITE_PIVOT_BIT);
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

	sfxShoot[0] = mgdl_LoadSoundWav("assets/shot0.wav");
	sfxShoot[1] = mgdl_LoadSoundWav("assets/shot1.wav");
	sfxShoot[2] = mgdl_LoadSoundWav("assets/shot2.wav");
	sfxShoot[3] = mgdl_LoadSoundWav("assets/shot3.wav");
	sfxShoot[4] = mgdl_LoadSoundWav("assets/shot4.wav");
	sfxShoot[5] = mgdl_LoadSoundWav("assets/shot5.wav");
	sfxShoot[6] = mgdl_LoadSoundWav("assets/shot6.wav");
	sfxShoot[7] = mgdl_LoadSoundWav("assets/shot7.wav");
	sfxHitWall[0] = mgdl_LoadSoundWav("assets/hitWall0.wav");
	sfxHitWall[1] = mgdl_LoadSoundWav("assets/hitWall1.wav");
	sfxHitWall[2] = mgdl_LoadSoundWav("assets/hitWall2.wav");
	sfxHitWall[3] = mgdl_LoadSoundWav("assets/hitWall3.wav");
	sfxHitWall[4] = mgdl_LoadSoundWav("assets/hitWall4.wav");
	sfxHitWall[5] = mgdl_LoadSoundWav("assets/hitWall5.wav");
	sfxHitWall[6] = mgdl_LoadSoundWav("assets/hitWall6.wav");
	sfxHitWall[7] = mgdl_LoadSoundWav("assets/hitWall7.wav");
	sfxHitPlayer = mgdl_LoadSoundWav("assets/hit.wav");
	sfxHitPlayerWithMask = mgdl_LoadSoundWav("assets/drop.wav");
	sfxPickupMask = mgdl_LoadSoundWav("assets/getMask.wav");
}

void Gameplay_StartMap(DukeMap* map)
{
	winnerPlayerIndex = -1;

	DSprite* treasureSprite = Map_FindSprite(map, TREASURE_LOTAG, 0);
	if (treasureSprite)
	{
		treasureSprite->cstat = Flag_Set(treasureSprite->cstat, 1 << CSTAT_SPRITE_INVISIBLE);
		treasure.render = true;
		treasure.position = treasureSprite->position;
		treasure.sectorNumber = treasureSprite->sectnum;
		treasure.radius = 800.0f;
	}

	for (int i = 0; i < MAX_BULLET_AMOUNT; ++i)
	{
		bullets[i].sectorNumber = 0;
		bullets[i].alive = false;
		bullets[i].timeAlive = 0.0f;
		bullets[i].position = vec3();
		bullets[i].direction = vec3();
		bullets[i].radius = 128.0f;
	}

	SetDark(false);
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
	else if (mgdl_GetElapsedSeconds() < player->shootTimer - player->shootRate * 0.25f)
	{
		tempSprites[index].picnum = player->hasTreasure ? PICNUM_PLAYER_SHOOT_WITH_MASK : PICNUM_PLAYER_SHOOT;
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
			SetDark(true);
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
					Audio_PlaySound(sfxShoot[shootSfxIndex]);
					shootSfxIndex++;
					if (shootSfxIndex >= 8) shootSfxIndex = 0;
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
				Audio_PlaySound(sfxHitWall[wallSfxIndex]);
				wallSfxIndex++;
				if (wallSfxIndex >= 8) wallSfxIndex = 0;
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
