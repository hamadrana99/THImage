#ifndef TH_PPM_INC
#define TH_PPM_INC
#include <TH.h>

#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define libppm_(NAME) TH_CONCAT_3(libppm_, Real, NAME)

/* Get the next character in the file, skipping over comments, which
 * start with a # and continue to the end of the line. 
 */
static char ppm_getc(FILE *fp);

/* Get the next integer, skipping whitespace and comments. */
static long ppm_get_long(FILE *fp);

#include "generic/ppm.h"
#include "THGenerateAllTypes.h"
#endif
