#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/THImage.h"
#else

THTensor* image_(load)(const char* filename, int depth);        //depth is the number of channels
THLongTensor* image_(getSize)(const char* filename);     //returns size tensor
void image_(save)(char* filename, THTensor* src);
void image_(crop)(THTensor* src, THTensor*dst, long startx, long starty, long endx, long endy);
void image_(translate)(THTensor* src, THTensor* dst, long x, long y);
void image_(scale)(THTensor* src, THTensor* dst, long width, long height, const char* mode);
void image_(rotate)(THTensor* src, THTensor* dst, float theta, const char* mode);
void image_(polar)(THTensor* src, THTensor* dst, const char* interpolation, const char* mode);
void image_(logPolar)(THTensor* src, THTensor* dst, const char* interpolation, const char* mode);
void image_(warp)(THTensor* src, THTensor* dst, const char* warp);
void image_(affineTransorm)(THTensor* src, THTensor* dst, THTensor* matrix);
void image_(hflip)(THTensor* src, THTensor* dst);
void image_(vflip)(THTensor* src, THTensor* dst);
void image_(flip)(THTensor* src, THTensor* dst, long flip_dim);
void image_(minMax)(THTensor* src, THTensor* dst, real max, real min);

#endif
