#ifndef PRINTFF
#define PTINTFF

#include "stdint.h"
#include "stdbool.h"
#include "am_util_stdio.h"
#include <stdarg.h>

#define printf am_util_stdio_printf
#define sprintf am_util_stdio_sprintf

#define snprintf am_util_stdio_snprintf
#define vprintf am_util_stdio_vprintf
#define vsnprintf am_util_stdio_vsnprintf

#endif