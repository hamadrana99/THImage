
#include "lua_png.h"

static void
libpng_userReadData(png_structp pngPtrSrc, png_bytep dest, png_size_t length)
{
  libpng_inmem_buffer* src = png_get_io_ptr(pngPtrSrc);
  assert(src->offset+length <= src->length);
  memcpy(dest, src->buffer + src->offset, length);
  src->offset += length;
}


/*
 * Call back for writing png data to memory
 */
static void libpng_userWriteData(png_structp  png_ptr, png_bytep data, png_size_t length) {
    struct libpng_inmem_write_struct *p = (struct libpng_inmem_write_struct*)png_get_io_ptr(png_ptr);
    p->inmem=realloc(p->inmem,p->inmem_size+length);
    memmove(p->inmem+p->inmem_size,data,length);
    p->inmem_size+=length;
}

/*
 * Custom error handling function (see `png_set_error_fn`)
 */
static void
libpng_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
  libpng_errmsg *errmsg = png_get_error_ptr(png_ptr);
  int max = sizeof(errmsg->str) - 1;
  strncpy(errmsg->str, error_msg, max);
  errmsg->str[max] = '\0';
  longjmp(png_jmpbuf(png_ptr), 1);
}

#include "generic/lua_png.c"
#include "THGenerateAllTypes.h"

