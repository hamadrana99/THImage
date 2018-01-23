#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/ppm.h"
#else

static int libppm_(Main_size)(const char *filename, int *channels, int *height, int *width);

static int libppm_(Main_load)(const char *filename, THTensor *tensor);

int libppm_(Main_save)(const char *filename, THTensor *tensor);
#endif
