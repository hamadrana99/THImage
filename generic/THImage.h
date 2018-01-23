#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/THImage.h"
#else

#include "jpeg.h"
#include "lua_png.h"
#include "ppm.h"
#include "image.h"
#include <TH.h>

THTensor* image_(load)(THTensor* src, int depth);        //depth is the number of channels
THLongTensor* image_(getSize)(const char* filename);     //returns size tensor
void image_(save)(const char* filename, THTensor* src);

#endif
