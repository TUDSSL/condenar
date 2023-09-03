
#ifndef PLATFORMS_PLATFORM_H_
#define PLATFORMS_PLATFORM_H_

#include "am_mcu_apollo.h"

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define CATSTR(A, B) A B

#if HWREV == 1
#include "int-console-v1-0.h"
#elif HWREV == 2
#include "int-console-v1-1.h"
#elif HWREV == 3
#include "int-console-v1-2.h"
#else
#error "Please define a platform the hardware revision."
#endif


#endif /* PLATFORMS_PLATFORM_H_ */
