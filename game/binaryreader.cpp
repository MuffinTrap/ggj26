#include "binaryreader.h"
#include <mgdl/mgdl-util.h>
#include <stdio.h>

static FILE* openFile = nullptr;
static s8 buffer[4];

bool OpenBinary(const char* filename)
{
    openFile = fopen(filename, "rb");
}
bool CloseBinary()
{
    fclose(openFile);
}

s16 ReadInt16()
{
    fread(buffer, sizeof(s16), 1, openFile);
#   ifdef GEKKO
    RevBytes(buffer, sizeof(s16));
#   endif
    s16 out = *(s16*)(buffer);
    return out;
}
u16 ReadUInt16()
{
    fread(buffer, sizeof(u16), 1, openFile);
#   ifdef GEKKO
    RevBytes(buffer, sizeof(u16));
#   endif
    u16 out = *(u16*)(buffer);
    return out;
}
s32 ReadInt32()
{
    fread(buffer, sizeof(s32), 1, openFile);
#   ifdef GEKKO
    RevBytes(buffer, sizeof(s32));
#   endif
    s32 out = *(s32*)(buffer);
    return out;
}
s8  ReadSByte()
{
    fread(buffer, sizeof(u8), 1, openFile);
    u8 out = *(u8*)(buffer);
    return out;
}
u8  ReadByte()
{
    fread(buffer, sizeof(u8), 1, openFile);
    u8 out = *(u8*)(buffer);
    return out;
}
