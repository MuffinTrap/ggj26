#pragma once
#include "dukemap.h"

// Reads duke nukem maps made with mapster32
#ifdef __cplusplus
extern "C" {
#endif
    DukeMap* ReadFromFile(const char* mapfilename);
#ifdef __cplusplus
}
#endif
