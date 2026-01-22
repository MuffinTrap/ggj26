#include "dukemapreader.h"
#include "binaryreader.h"
#include <mgdl.h>
#include <stdio.h>

DukeMap* ReadMapFromFile(const char* mapfilename)
{
    Log_InfoF("Reading map file %s\n", mapfilename);
    if (!OpenBinary(mapfilename))
    {
        Log_ErrorF("No map file %s\n", mapfilename);

        return nullptr;
    }
    DukeMap* mapPtr = (DukeMap*)malloc(sizeof(DukeMap));
    DukeMap m = {0};
    m.version = ReadInt32();
    s32 x = ReadInt32();
    s32 y = ReadInt32();
    s32 z = ReadInt32();
    m.startPosition = (vec3){{(float)x, (float)y, (float)z}};
    m.startAngle = ReadInt16();
    m.startingSector = ReadInt16();

    m.sectorAmount = ReadUInt16();
    m.sectors = (Sector*)malloc(sizeof(Sector)*m.sectorAmount);
    for (int i = 0; i < m.sectorAmount; i++)
    {
        Sector* s = &m.sectors[i];
        s->wallptr = ReadInt16();
        s->wallnum = ReadInt16();
        s->ceilingz = ReadInt32();
        s->floorz = ReadInt32();
        s->ceilingstat = ReadInt16();
        s->floorstat = ReadInt16();
        s->ceilingpicnum = ReadInt16();
        s->ceilingheinum = ReadInt16();
        s->ceilingshade = ReadSByte();
        s->ceilingpal = ReadByte();
        s->ceilingxpanning = ReadByte();
        s->ceilingypanning = ReadByte();
        s->floorpicnum = ReadInt16();
        s->floorheinum = ReadInt16();
        s->floorshade = ReadSByte();
        s->floorpal = ReadByte();
        s->floorxpanning = ReadByte();
        s->floorypanning = ReadByte();
        s->visibility = ReadByte();
        s->filler = ReadByte();
        s->lotag = ReadInt16();
        s->hitag = ReadInt16();
        s->extra = ReadInt16();
    }
    m.wallAmount = ReadUInt16();
    m.walls = (Wall*)malloc(sizeof(Wall) * m.wallAmount);
    for (int s = 0; s < m.wallAmount; s++)
    {
        Wall* w = &m.walls[s];
        w->x = ReadInt32();
        w->y = ReadInt32();
        w->point2 = ReadInt16();
        w->nextwall = ReadInt16();
        w->nextsector = ReadInt16();
        w->cstat = ReadInt16();
        w->picnum = ReadInt16();
        w->overpicnum = ReadInt16();
        w->shade = ReadSByte();
        w->pal = ReadByte();
        w->xrepeat = ReadByte();
        w->yrepeat = ReadByte();
        w->xpanning = ReadByte();
        w->ypanning = ReadByte();
        w->lotag = ReadInt16();
        w->hitag = ReadInt16();
        w->extra = ReadInt16();
    }
    m.spriteAmount = ReadUInt16();
    m.sprites = (DSprite*)malloc(sizeof(DSprite) * m.spriteAmount);
    for (int i = 0; i < m.spriteAmount; i++)
    {
        DSprite* s = &m.sprites[i];
        s->x = ReadInt32();
        s->y = ReadInt32();
        s->z = ReadInt32();
        s->cstat = ReadInt16();
        s->picnum = ReadInt16();
        s->shade = ReadSByte();
        s->pal = ReadByte();
        s->clipdist = ReadByte();
        s->filler = ReadByte();
        s->xrepeat = ReadByte();
        s->yrepeat = ReadByte();
        s->xoffset = ReadSByte();
        s->yoffset = ReadSByte();
        s->sectnum = ReadInt16();
        s->statnum = ReadInt16();
        s->ang = ReadInt16();
        s->owner = ReadInt16();
        s->xvel = ReadInt16();
        s->yvel = ReadInt16();
        s->zvel = ReadInt16();
        s->lotag = ReadInt16();
        s->hitag = ReadInt16();
        s->extra = ReadInt16();
    }

    CloseBinary();
    (*mapPtr) = m;
    return mapPtr;
}

