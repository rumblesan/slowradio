#ifndef __SLOW_RADIO_FILE_UTILS__
#define __SLOW_RADIO_FILE_UTILS__

#include <stdbool.h>

#include "bclib/bstrlib.h"

bool is_regular_file(const char *path);

bstring get_random_file(bstring pattern);

#endif
