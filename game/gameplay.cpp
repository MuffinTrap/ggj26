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
	}
}

void Gameplay_Update(Player* player, DukeMap* map)
{
	float dt = mgdl_GetDeltaTime();

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
			//for (int i = 0; i < playerAmount; ++j)
			//{
			//	if (aabbCollision(bullets[i].position, player[j]->position))
			//	{
			//		bullets[i].alive = false;
			//
			//		if (player[j]->hasTreasure)
			//		{
			//			// TODO: Bullet hit player with treasure sfx
			//			// TODO: Player drops treasure
			//		}
			//		else
			//		{
			//			// TODO: Bullet hit player sfx
			//		}
			//		// TODO: Stun player
			//	}
			//}
		}
	}
}