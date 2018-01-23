#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/lua_png.h"
#else

/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 * Clement: modified for Torch7.
 */

static int libpng_(Main_load)(const char *file_name,
                              THByteTensor *src,	                    //source tensor, set to NULL if reading from file
                              THTensor *tensor,	                      //destination tensor
                              int *bit_depth_to_return);              

static int libpng_(Main_save)(const char *file_name,
                              THByteTensor* tensor_dest,			        //destination tensor, set to NULL if saving to file
                              THTensor* tensor)	;                     //source tensor
                              
static int libpng_(Main_size)(const char *filename, 
                              int *depth_to_return, 
                              int *height_to_return, 
                              int *width_to_return );

#endif
