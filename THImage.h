#ifndef TH_IM_INC
#define TH_IM_INC
#include <TH.h>

#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor TH_CONCAT_STRING_3(torch., Real, Tensor)
#define image_(NAME) TH_CONCAT_3(image_, Real, NAME)

#include "generic/THImage.h"
#include "THGenerateAllTypes.h"
#endif
