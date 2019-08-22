#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/THImage.h"
#else

THTensor* image_(load)(const char* filename, int depth);        
THLongTensor* image_(getSize)(const char* filename);    
void image_(save)(char* filename, THTensor* src);
THTensor* image_(specificCrop)(THTensor* src,
                               THTensor* dst, 
                               //c for center, tl for top left,
                               //tr for top right, bl for bottom
                               //left, br for bottom right
                               const char *crop_type,     
                               long height, 
                               long width);
THTensor* image_(crop)(THTensor* src,
                       THTensor* dst, 
                       long startx, 
                       long starty, 
                       long endx, 
                       long endy);
THTensor* image_(translate)(THTensor* src, long x, long y);
THTensor* image_(scale)(THTensor* src, 
                        long height, 
                        long width, 
                        //simple, bilinear or bicubic
                        const char* mode);
THTensor* image_(rotate)(THTensor* src, 
                         float theta, 
                         //simple or bilinear
                         const char* mode);
THTensor* image_(polar)(THTensor* src,
                        //simple or bilinear 
                        const char* interpolation, 
                        //valid or full
                        const char* mode);
THTensor* image_(logPolar)(THTensor* src, 
                           //simple or bilinear
                           const char* interpolation, 
                           //valid or full
                           const char* mode);
THTensor* image_(warp)(THTensor* src, 
                       THTensor* flow_field,
                       //simple, bilinear, bicubic or lanczos 
                       const char* mode, 
                       //1 or 0 to add (x,y) to flow field
                       int offset_mode, 
                       //clamp or pad
                       const char* clamp_mode, 
                       real pad_value);
THTensor* image_(hflip)(THTensor* src, THTensor* dst);
THTensor* image_(vflip)(THTensor* src);
THTensor* image_(flip)(THTensor* src, long flip_dim);
THTensor* image_(minMax)(THTensor* src, real min, real max);
THTensor* image_(rgb2y)(THTensor*src);
THTensor* image_(rgb2hsv)(THTensor* src);
THTensor* image_(hsv2rgb)(THTensor* src);
THTensor* image_(rgb2hsl)(THTensor* src);
THTensor* image_(hsl2rgb)(THTensor* src);
THTensor* image_(rgb2lab)(THTensor* src);
THTensor* image_(lab2rgb)(THTensor* src);
void image_(drawText)(THTensor* src, 
                      const char* text, 
                      long x, 
                      long y, 
                      long size,
                      int color_r, 
                      int color_g, 
                      int color_b,
                      //1 or 0 to wrap around lines 
                      int wrap); 
void image_(drawRect)(THTensor* src, 
                      long startx, 
                      long starty, 
                      long endx, 
                      long endy, 
                      int line_width,
                      int color_r, 
                      int color_g, 
                      int color_b); 


THTensor*image_(affineTransorm)(THTensor* src,
                                THTensor* matrix);
THTensor* image_(convolve)(THTensor* src,
                           THTensor* kernel,
                           //full, valid or same
                           const char* mode);


#endif
