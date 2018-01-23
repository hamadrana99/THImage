#ifndef TH_PNG_INC
#define TH_PNG_INC
#include <TH.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PNG_DEBUG 3
#include <png.h>

#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define libpng_(NAME) TH_CONCAT_3(libpng_, Real, NAME)

/*
 * Bookkeeping struct for reading png data from memory
 */
typedef struct {
  unsigned char* buffer;
  png_size_t offset;
  png_size_t length;
} libpng_inmem_buffer;

/*
 * Call back for reading png data from memory
 */
static void
libpng_userReadData(png_structp pngPtrSrc, png_bytep dest, png_size_t length);


struct libpng_inmem_write_struct
{
  unsigned char *inmem;  /* destination memory (if saving to memory) */
  unsigned long inmem_size;  /* destination memory size (bytes) */
};

/*
 * Call back for writing png data to memory
 */
static void libpng_userWriteData(png_structp  png_ptr, png_bytep data, png_size_t length) ;

/*
 * Error message wrapper (single member struct to preserve `str` size info)
 */
typedef struct {
  char str[256];
} libpng_errmsg;

/*
 * Custom error handling function (see `png_set_error_fn`)
 */
static void
libpng_error_fn(png_structp png_ptr, png_const_charp error_msg);

#include "generic/lua_png.h"
#include "THGenerateAllTypes.h"
#endif
