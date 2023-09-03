#ifndef KERNEL_H
#define KERNEL_H

#include "k_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "Screen_Base.h"

#ifndef NULL
#define NULL 0
#endif

#define AM_MEM_MCU_MRAM                0x00018000
#define AM_MEM_MCU_TCM                 0x10000000
#define AM_MEM_SHARED_SRAM             0x10060000

#define CHECKPOINT_EXCLUDE_DATA __attribute__((section(".checkpoint_exclude_data")))
#define CHECKPOINT_EXCLUDE_BSS  __attribute__((section(".checkpoint_exclude_bss")))
#define CHECKPOINT_INCLUDE_DATA __attribute__((section(".checkpoint_include_data")))
#define CHECKPOINT_INCLUDE_BSS  __attribute__((section(".checkpoint_include_bss")))

typedef enum{
    Int32,
    Uint32,
    Bool,
    Action //Internal use only
}SettingsType;

typedef struct k_app{
    char name[32];
    void (*entryPoint)(struct k_app*);
    void (*onCheckpointRestore)(struct k_app*);
    uint8_t* appIcon; //A 50x50px icon. Use http://javl.github.io/image2cpp/ to get the code
    char appIconImageName[20]; //Name of the icon, it will be loaded form storage
    k_image appIconImage; //A icon, should be 50x50px
    uint32_t arg; //Optional arguments, the app decides what to use this for
    uint32_t requestedStorageBytes;
} k_app;


//Kernel entry point, never returns
int kernel_main(void);

//Registers an app with the kernel, should only be called from k_registerApps in k_apps.c
//The k_app object should always be valid, even after exiting this function
void k_registerApp(k_app* app);


//Gets an app from the registered list by name (or null if not found)
k_app* k_getAppByName(char* appName);

//Gets the list of registered apps
k_app** k_getAppList(uint32_t* appCountOut);

//Gets the next app scheduled to be run
k_app* k_getNextApp();

//Gets the app currently running
k_app* k_getCurrentApp();

//Sleeps (in low power mode) until a certain time (in ms)
void k_lowPowerSleepUntil(int64_t sleepUntil);

//Sets the next up scheduled to be run
void k_setNextApp(k_app* _nextAppToRun);

void k_setShouldCurrentAppExit(bool newState);
bool k_getShouldCurrentAppExit();

uint32_t k_getCurrentFrame();

bool k_isRecoveringFromACheckpoint();

void k_ForceRestart();

bool k_IsBeingCranked();
bool k_GetCrankDirection(); //True = Clockwise, False = Counter Clockwise

//APP STORAGE //////////////////////////////////////////////////////////////////////////////

//Writes a chunk of bytes to the app storage of a certain app.
void k_WriteToAppStorage(k_app* app, uint32_t storageOffset, uint8_t* data,  uint32_t size);

//Gets a READ-ONLY pointer to the app storage of a certain app. Returns NULL if not available
uint8_t* k_GetStorageReadOnlyAddressForApp(k_app* app, uint32_t* sizeOut);

//END OF APP STORAGE   /////////////////////////////////////////////////////////////////////


//METRICS /////////////////////////////////////////////////////////////////////////////////
#ifdef KERNEL_ENABLE_METRICS
void IncreaseMetricsInternal(const char *name, int value); //Increments a metric by a certain amount, the final value will be calculated as a rate per second
#define IncreaseMetrics(name, value) IncreaseMetricsInternal(name, value)
#else //If metrics are disabled, just ignore the call
#define IncreaseMetrics(name, value)
#endif
void SetMetricDirect(const char *name, int value); //Sets a metric to a certain value
//END OF METRICS ///////////////////////////////////////////////////////////////////////////



//APP HEAP /////////////////////////////////////////////////////////////////////////////////

uint8_t* k_getHeapPtr();
//Any active app FULLY and EXCLUSIVELY controls this contiguous piece of memory
uint32_t k_getHeapSize();

//END OF APP HEAP //////////////////////////////////////////////////////////////////////////


//INPUT HANDLING ///////////////////////////////////////////////////////////////////////////

typedef struct k_input_state{
    bool buttonA, buttonB, buttonX, buttonY;
    bool buttonPower;//, buttonJoystick;
    bool triggerLeft, triggerRight;
    float joystickX, joystickY; //-1 to 1, center at 0
} k_input_state;

void k_GetInput(k_input_state* state, bool includeJoysticks);

//END OF INPUT HANDLING ////////////////////////////////////////////////////////////////////



//SCREEN RENDERING /////////////////////////////////////////////////////////////////////////

void k_BeginScreenUpdate(); //To be called before starting to write a new frame to the screen

//Use the k_draw_XXXX APIs to write to the screen here. Defined in Kernel_Draw.h

void k_EndScreenUpdate(bool checkpointsAllowed, bool showOverlays); //To be called after writing a new frame to the screen. Usually outputs the frame


//END OF SCREEN RENDERING //////////////////////////////////////////////////////////////////



//MISC /////////////////////////////////////////////////////////////////////////////////////

void k_DelayMS(int ms);
uint32_t k_GetTimeMS();
float k_GetVBat();
uint32_t k_getLastFPS();

//END OF MISC //////////////////////////////////////////////////////////////////////////////


// SETTTINGS ///////////////////////////////////////////////////////////////////////////////

bool k_GetSettingGeneric(char* path, SettingsType type, void* dataOut, uint32_t bytesToCopy);
uint32_t k_GetSettingUInt32(char* path, uint32_t defaultValue);
int32_t k_GetSettingInt32(char* path, int32_t defaultValue);
bool k_GetSettingBool(char* path, bool defaultValue);

bool k_SetSettingGeneric(char* path, SettingsType type, void* dataIn, uint32_t bytesToCopy);
uint32_t k_SetSettingUInt32(char* path, uint32_t valueToSet);
int32_t k_SetSettingInt32(char* path, int32_t valueToSet);
bool k_SetSettingBool(char* path, bool valueToSet);

void k_GetMaximumSettingLength(uint32_t* maxNameLengthOut, uint32_t* maxContentSizeOut);

//Given a filter, gets the next matching setting. Input back the returning value until NULL. Start with NULL
void* k_EnumerateNextSettingEntry(char* beginningFilter, void* lastTokenOrNULL, char* nameOut, SettingsType* typeOut);

//END OF SETTINGS //////////////////////////////////////////////////////////////////////////


//MEM_WATCHER //////////////////////////////////////////////////////////////////////////////

void k_MemWatcher_ResetRegions();

typedef struct k_memRegions{
    uint8_t regionModifiedStatus[8]; //Each bit (256 regions) is whether a region was modified or not
} k_memRegions;
void k_MemWatcher_GetModifiedRegions(k_memRegions* k_memRegions);

//END OF MEM_WATCHER ///////////////////////////////////////////////////////////////////////


//LOGGING///////////////////////////////////////////////////////////////////////////////////
void UART_LOG_SendCommand(char* toSend);
void UART_LOG_StartExperiment(int experimentNumber);
void UART_LOG_EndExperiment();
void UART_LOG_SendEvent(int eventNumber);
void UART_LOG_SetChannelData(int channelNumber,int channelData);

#define LOG_CHANNEL_APP_NAME 0
#define LOG_CHANNEL_LIGHT_SENSOR 1
#define LOG_CHANNEL_SLEEP_PERCENT 2
//END OF LOGGING/////////////////////////////////////////////////////////////////////////////////////


//CRANK///////////////////////////////////////////////////////////////////////////////////
void k_CrankInitialize();
float k_GetCrank();
//END OF CRANK/////////////////////////////////////////////////////////////////////////////////////

#endif
