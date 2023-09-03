#include <stdint.h>
#include "Kernel.h"

#define SETTINGS_COOKIE 0xC0FFEBAD




typedef struct {
    char name[SETTINGS_MAX_NAME_LENGHT];
    uint8_t data[SETTINGS_MAX_DATA_LENGHT];
    SettingsType dataType;
    uint32_t cookie;
} k_settingsItem; //100B per settings item


bool k_AddSettingIfNotExistent(char* path, SettingsType type, void* defaultValue, uint32_t dataSize);
void k_ResetAllSetings();
void k_InitializeSettings(); //Defined in k_apps.c