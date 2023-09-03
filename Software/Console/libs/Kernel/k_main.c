#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "string.h"

#include "Helpers/Misc.h"
#include "Helpers/PinDefinitions.h"
#include "Screen/Screen.h"
#include "Helpers/Flash.h"
#include "Helpers/TimeKeeper.h"
#include "Helpers/RNG.h"
#include "Helpers/UART.h"
#include "Helpers/MemWatcher.h"
#include "Kernel.h"
#include "SettingsHandler.h"
#include "PowerDownRecovery.h"
#include "FileSystem.h"

#include "SEGGER_RTT.h"

//IMP: CACHE_ALL_COLORMAP_TO_RAM

k_app* apps[MAX_NUMBER_OF_APPS] = { 0 };

CHECKPOINT_INCLUDE_DATA k_app* nextAppToRun = NULL;
CHECKPOINT_INCLUDE_DATA k_app* currentApp = NULL;
CHECKPOINT_INCLUDE_DATA bool shouldCurrentAppExit = false;
CHECKPOINT_INCLUDE_DATA uint32_t frameCounter=0;
CHECKPOINT_EXCLUDE_DATA uint32_t k_lastFrameMillis=0;
CHECKPOINT_EXCLUDE_DATA uint32_t frameCounter_NotCheckpointed = 0;


//Any active app FULLY and EXCLUSIVELY controls this
__attribute__ ((section(".displayData_bss"), aligned (8192)))  uint8_t app_heap[APP_HEAP_SIZE];


void k_init();
void k_registerApps();
void k_tryRecoverFromPowerDown();
void k_run();
uint32_t k_getLastTimeInputSeen();

//External functions, but should only be used by this file
void k_InitializeStorageForApps();



int kernel_main(void)
{    
    //Preinitialization of the kernel (HALs...)
    k_init();    

    //Register apps here
    k_registerApps();

    k_InitializeStorageForApps();

    k_input_state state;
    k_GetInput(&state, false);
    if(state.buttonY && state.triggerLeft && state.triggerRight) k_ResetAllSetings();
    k_InitializeSettings();

    k_tryRecoverFromPowerDown(); //This won't return if successful

    //If we couldn't recover from a power down (or there wasn't anything to do), just select the normal menu
    //k_setNextApp(k_getAppByName("Doom")); 
   // k_setNextApp(k_getAppByName("GameBoy")); 
     k_setNextApp(k_getAppByName("Main menu")); 
 //      k_setNextApp(k_getAppByName("Download mode")); 
   // k_setNextApp(k_getAppByName("GameBoy")); 
      


    //Let the kernel run, this should never return
    k_run();

    return 0;
}



void k_init(){


    

    //Enable the FPU
    am_hal_sysctrl_fpu_enable();
    am_hal_sysctrl_fpu_stacking_enable(true);

    const am_hal_pwrctrl_mcu_memory_config_t    g_DefaultMcuMemCfg1 =
    {
        .eCacheCfg          = AM_HAL_PWRCTRL_CACHE_ALL,
        .bRetainCache       = true,
        .eDTCMCfg           = AM_HAL_PWRCTRL_DTCM_384K,
        .eRetainDTCM        = AM_HAL_PWRCTRL_DTCM_384K,
        .bEnableNVM0        = true,
        .bRetainNVM0        = true
    };
   // am_hal_pwrctrl_mcu_memory_config((am_hal_pwrctrl_mcu_memory_config_t *)&g_DefaultMcuMemCfg1);

    const am_hal_pwrctrl_sram_memcfg_t          g_DefaultSRAMCfg =
    {
        .eSRAMCfg           = AM_HAL_PWRCTRL_SRAM_ALL,
        .eActiveWithMCU     = AM_HAL_PWRCTRL_SRAM_ALL,
        .eActiveWithGFX     = AM_HAL_PWRCTRL_SRAM_NONE,
        .eActiveWithDISP    = AM_HAL_PWRCTRL_SRAM_NONE,
        .eActiveWithDSP     = AM_HAL_PWRCTRL_SRAM_NONE,
        .eSRAMRetain        = AM_HAL_PWRCTRL_SRAM_ALL
    };
    am_hal_pwrctrl_sram_config((am_hal_pwrctrl_sram_memcfg_t *)&g_DefaultSRAMCfg);


    am_hal_pwrctrl_dsp_memory_config_t ExtSRAMMemCfg =
    {
    .bEnableICache      = false,  //Should always be "false"
    .bRetainCache       = false,  //Should always be "false"
    .bEnableRAM         = true,   //Controls Extended RAM power when MCU awake
    .bActiveRAM         = false,  //Should be "false"
    .bRetainRAM         = true       //true configures Extended RAM to be retained in deep sleep
    };
    am_hal_pwrctrl_dsp_memory_config(AM_HAL_DSP0, &ExtSRAMMemCfg);
    am_hal_pwrctrl_dsp_memory_config(AM_HAL_DSP1, &ExtSRAMMemCfg);  //1mW more power consumption when enabled  

    //Setup all the IO
    SetupIO();     

    //Initialize the memWatcher
    //MemWatcher_Initialize();    

    //Enable interrupts globaly
    am_hal_interrupt_master_enable();
        
    // Set the default cache configuration    
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();
    
    // Configure the board for low power operation.    
    am_bsp_low_power_init();

    // Initialize the printf interface and UART
    SetupUART();

    // Initialize the SEGGER_RTT backend
    SEGGER_RTT_Init();

    // Print the banner.    
    LOG_I("App restarted");  
    
    InitRNG();
    Timer_Init();

    k_CrankInitialize();

    Flash_Initialize();
    Screen_Initialize(); 
    LightSensor_Initialize();  
    
    LOG_I("Kernel initalization complete");
}


#define FPS_SAMPLES_MAX 10
uint32_t firstFPSSample = 0;
uint8_t totalFPSsamplesGathered = 0;
uint32_t lastFPS = 0;
uint32_t lastCheckpointTaken = 0;


void k_DoEndOfFrameStuff(bool checkpointsAllowed){

    k_input_state inputState;
    k_GetInput(&inputState, false);    

    //If the shortcut to go to menu is pressed, do it
    if(inputState.triggerLeft && inputState.triggerRight && inputState.buttonPower){
        k_setNextApp(k_getAppByName("Main menu"));
        k_setShouldCurrentAppExit(true);
    }

    //FPS stuff
    if(totalFPSsamplesGathered==0){
        firstFPSSample = k_GetTimeMS();
        totalFPSsamplesGathered++;
    }else if(totalFPSsamplesGathered==FPS_SAMPLES_MAX){
        totalFPSsamplesGathered = 0;
        LOG_I("FPS: %d",lastFPS);
        lastFPS = (int)(1000.0f/((k_GetTimeMS()-firstFPSSample)/(float)FPS_SAMPLES_MAX));
    }else{
        totalFPSsamplesGathered++;
    }    

    if(checkpointsAllowed && k_GetSettingBool("/Other/Intermittency/Checkpoint creation enabled", true)){
        if((millis()-lastCheckpointTaken) > k_GetSettingUInt32("/Other/Intermittency/Checkpoint interval (ms)",1000)){
            if(k_GetSettingBool("/Other/Intermittency/Only checkpoint while active",true)){
                if(millis()-k_getLastTimeInputSeen() < k_GetSettingUInt32("/Intermittency/Active timeout",5000)){ //Only do checkpoints if there are any inputs
                    k_storeCurrentState();
                }
            }else{
                k_storeCurrentState();
            }
            lastCheckpointTaken = millis();

        }
    }    

    //The first frame after a checkpoint is invalid, so don't set the backlight on yet
    if(frameCounter_NotCheckpointed > 1){
        float lightLevel = LightSensor_GetValue();
        if(k_GetSettingBool("/Other/Energy/Automatic brightness", false)){            
            int backlightLevel = sqrt(lightLevel) *5;
            if(backlightLevel < 5) backlightLevel = 5;
            if(backlightLevel > 100) backlightLevel = 100;
            //LOG_I("Setting backlight to %d",backlightLevel);
            ChangeBacklightPWMValue(backlightLevel);           
        }else{
            ChangeBacklightPWMValue(k_GetSettingUInt32("/Other/Energy/Screen brightness", 3)*10);
        }
        if(frameCounter_NotCheckpointed % 10 == 0){
            UART_LOG_SetChannelData(LOG_CHANNEL_LIGHT_SENSOR, lightLevel);
        }
    }

    

    //am_hal_pwrctrl_mcu_mode_select already caches the mode and checks if it is already set inside    
    if (am_hal_pwrctrl_mcu_mode_select(k_GetSettingBool("/Other/Energy/Fast CPU", false) ? AM_HAL_PWRCTRL_MCU_MODE_HIGH_PERFORMANCE : AM_HAL_PWRCTRL_MCU_MODE_LOW_POWER) != AM_HAL_STATUS_SUCCESS)
    {
        LOG_E("Failed to set high performance mode!");
    }

    //am_hal_clkgen_clkout_enable already caches the mode and checks if it is already set inside    
    if (am_hal_clkgen_clkout_enable(true, k_GetSettingBool("/Other/Energy/Fast LCD refresh", false) ? CLKGEN_CLKOUT_CKSEL_HFRC2_24MHz : CLKGEN_CLKOUT_CKSEL_HFRC2_12MHz) != AM_HAL_STATUS_SUCCESS)
    {
        LOG_E("Failed to set LCD refresh rate!");
    }

   

    //Power stuff
    float vBat = k_GetVBat();
    if(vBat < 1.0f){
        PowerEverythingDown(true);
    }
    //End of power stuff

     GPIO_SetLevel(60,k_GetSettingBool("/Other/Energy/Infinite battery",false) && vBat > 3.0f); //Turn on the infinite battery if the setting is enabled and the battery is above 3.0V

    int targetFrameTime = 1000/k_GetSettingUInt32("/Other/Energy/Target refresh rate (Hz)", 20);
    int64_t toSleepUntil = (int64_t)k_lastFrameMillis + (targetFrameTime)-1;

    if(frameCounter%10 == 0){
        int percentSleep = (millis()-k_lastFrameMillis) * 100 / targetFrameTime;
        if(percentSleep > 100) percentSleep = 100;
        if(percentSleep < 0) percentSleep = 0;
        UART_LOG_SetChannelData(LOG_CHANNEL_SLEEP_PERCENT, percentSleep);
    }
   


    k_lowPowerSleepUntil(toSleepUntil); //Sleep until the next frame
    k_lastFrameMillis = millis();


    frameCounter++;
    frameCounter_NotCheckpointed++;
}

void k_run(){
    while(true){
        k_app* appToRunNow = k_getNextApp();
        if(appToRunNow != NULL){
            LOG_I("Starting to run app: %s",appToRunNow->name);
            UART_LOG_SetChannelData(LOG_CHANNEL_APP_NAME, appToRunNow->name[0]);
            k_setNextApp(NULL);
            k_setShouldCurrentAppExit(false);
            frameCounter=0;
            k_MemWatcher_ResetRegions();
            currentApp = appToRunNow;
            appToRunNow->entryPoint(appToRunNow);
            currentApp = NULL;
            if(k_getNextApp()==NULL) k_setNextApp(k_getAppByName("Main menu")); //If no app was selected to be run, that problably means a clean exit, so just show the menu
            LOG_I("Exited app: %s",appToRunNow->name);
        }else{
            LOG_W("No app to run!");
            DelayMS(1000);
        }
    }
}


k_app* k_getAppByName(char* appName){   
    for(int i=0;i<MAX_NUMBER_OF_APPS;i++){
        if(apps[i] != NULL && strcmp(apps[i]->name,appName)==0){
            return apps[i];
        }
    }
    LOG_W("Failed to find app by name: %s",appName);
    return NULL;
}

//This is not perfect (specially substracting one to sleep after reenabling the FLASH, but it works in most cases, and where it is not perfect, it doesn't really matter)
void k_lowPowerSleepUntil(int64_t sleepUntil){
    //You need to wait 
    sleepUntil--; //Subtract 1ms because that will be slept in the last loop (after waking up the flash)

    if(sleepUntil <= 0) return;
    if(sleepUntil > 0xFFFFFFFF) return;  //SleepUntil is a 32 bit value, so we can't sleep for more than 49 days

    

    //Disable FLASH while sleeping
    GPIO_SetLevel(PIN_ENABLE_MEM_VCC,0);

    while(sleepUntil > millis()){
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
    }

    //Enable FLASH after waking up
    GPIO_SetLevel(PIN_ENABLE_MEM_VCC,1);
    //Wait for 1 more ms after re-enabling the FLASH
    while(sleepUntil+1 > millis()){
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
    }
}


//Helper functions
k_app** k_getAppList(uint32_t* appCountOut){
    uint32_t appCount=0;
    for(appCount=0;appCount<MAX_NUMBER_OF_APPS;appCount++){
        if(apps[appCount] == NULL){
            break;
        }
    }

    //Skip the first app (usually the main menu)
    *appCountOut = appCount - 1;
    return apps + 1;
}

k_app* k_getNextApp(){
    return nextAppToRun;
}

void k_setNextApp(k_app* _nextAppToRun){
    nextAppToRun = _nextAppToRun;
    if(_nextAppToRun==NULL){
        LOG_I("Selected new app to run: NONE");
    }else{
        LOG_I("Selected new app to run: %s",_nextAppToRun -> name);
    }
}

void k_registerApp(k_app* app){
    LOG_DEBUG("Registering app: %s",app->name);

    if(app->appIconImageName){
        LOG_DEBUG("Loading icon for app: %s",app->name);
        app->appIconImage = GetImageByName(app->appIconImageName);
    }

    bool emptySpaceFound = false;
    for(int i=0;i<MAX_NUMBER_OF_APPS;i++){
        if(apps[i] == NULL){
            apps[i] = app;
            emptySpaceFound=true;
            break;
        }
    }

    if(!emptySpaceFound){
        LOG_E("Failed to register app %s. Not enough app slots", app->name);
    }    
}

void k_setShouldCurrentAppExit(bool newState){
    shouldCurrentAppExit = newState;
}
bool k_getShouldCurrentAppExit(){
    return shouldCurrentAppExit;
}

k_app* k_getCurrentApp(){
    return currentApp;
}

void k_DelayMS(int ms){
    DelayMS(ms);
}

uint32_t k_GetTimeMS(){
    return millis();
}

uint8_t* k_getHeapPtr(){
    return app_heap;
}

uint32_t k_getLastFPS(){
    return lastFPS;
}

uint32_t k_getHeapSize(){
    LOG_I("Heap size: %d",sizeof(app_heap));
    return sizeof(app_heap);
}

uint32_t k_getCurrentFrame(){
    return frameCounter;
}

void k_ForceRestart(){
    am_hal_sysctrl_aircr_reset();
}

bool k_IsBeingCranked(){
    float crankVal = k_GetCrank();
    return (crankVal > 0.05f || crankVal < -0.05f);
}

bool k_GetCrankDirection(){
    float crankVal = k_GetCrank();
    return (crankVal > 0.4f);

}