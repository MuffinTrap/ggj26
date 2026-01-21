#pragma once
#include "dukemap.h"
#include <mgdl/mgdl-types.h>

// Reads binary info from a file
#ifdef __cplusplus
extern "C" {
#endif


bool OpenBinary(const char* filename);

s16 ReadInt16();
u16 ReadUInt16();
s32 ReadInt32();
s8  ReadSByte();
u8  ReadByte();

bool CloseBinary();

#ifdef __cplusplus
}
#endif
