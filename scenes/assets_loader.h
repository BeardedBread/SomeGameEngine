#ifndef __ASSETS_LOADER_H
#define __ASSETS_LOADER_H

#include "assets.h"


bool load_from_infofile(const char* file, Assets_t* assets);
bool load_from_rres(const char* file, Assets_t* assets);

#endif // __ASSETS_LOADER_H
