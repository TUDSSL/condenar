#ifndef KCONFIG_H
#define KCONFIG_H

#define MAX_NUMBER_OF_APPS 16
#define MAX_APPNAME_LENGTH 32

#define SETTINGS_MAX_NAME_LENGHT 85
#define SETTINGS_MAX_DATA_LENGHT 10

#define SEND_SCREEN_OVER_RTT

#define APP_HEAP_SIZE (100 * 1000) //Large chunk of contiguous memory that apps can use
#define APP_STACK_SIZE 16384

#define FLASH_CACHING_ENABLED

//#define KERNEL_ENABLE_METRICS


#endif