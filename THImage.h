#ifndef TH_IM_INC
#include "lua_png.h"
#include "jpeg.h"
#include "ppm.h"
#include "image.h"
#include <TH.h>
#include <stdio.h>
#include <string.h>


#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define image_(NAME) TH_CONCAT_3(image_, Real, NAME)

#include "generic/THImage.h"
#include "THGenerateAllTypes.h"
#endif
