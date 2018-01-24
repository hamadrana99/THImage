#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/jpeg.h"
#else

#ifndef _LIBJPEG_ERROR_STRUCTS_
#define _LIBJPEG_ERROR_STRUCTS_
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */

  char msg[JMSG_LENGTH_MAX]; /* last error message */
};

typedef struct my_error_mgr * my_error_ptr;
#endif

int libjpeg_(Main_size)(const char *filename, int *channels, int *height, int *width);
THTensor* libjpeg_(Main_load)(const char *filename, THByteTensor *src);
int libjpeg_(Main_save)(const char *filename, THTensor *tensor,	int quality, THByteTensor* tensor_dest);

#endif
