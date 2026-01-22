#include "binaryreader.h"
#include <mgdl/mgdl-util.h>
#include <mgdl/mgdl-logger.h>
#include <stdio.h>

static FILE* openFile = nullptr;
static s8 buffer[4];

bool OpenBinary(const char* filename)
{
    openFile = fopen(filename, "rb");
    return (openFile != nullptr);
}
bool CloseBinary()
{
    if (openFile != nullptr)
    {
        fclose(openFile);
        return true;
    }
    return false;
}

s16 ReadInt16()
{
    s16 out;
    if (fread(&out, sizeof(s16), 1, openFile) == 1)
    {
        return out;
    }
    else
    {
        Log_Error("Failed to read Int16\n");
        return 0;
    }
#   ifdef GEKKO
    RevBytes(buffer, sizeof(s16));
#   endif
    //s16 out = *(s16*)(buffer);
    //return out;
}
u16 ReadUInt16()
{
    u16 out;
    fread(&out, sizeof(u16), 1, openFile);
#   ifdef GEKKO
    RevBytes(buffer, sizeof(u16));
#   endif
    //u16 out = *(u16*)(buffer);
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
